#include "video_export.h"

#include <cassert>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>

#include "framework/mesh.hpp"

namespace sn {

VideoExport &VideoExport::init(const char *filename, const uint32_t width,
                               const uint32_t height, const uint32_t fps) {
  this->recording = false;

  this->width = width;
  this->height = height;
  this->fps = fps;

  // open out_file for writing
  this->out_file = fopen(filename, "wb");
  assert(this->out_file); // panic on error

  // init opengl pixel buffer
  size_t frame_size = this->height * this->width * 4; // 4 because of rgba
  glGenBuffers(1, &this->pbo_id);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_id);
  glBufferData(GL_PIXEL_PACK_BUFFER, frame_size, NULL, GL_STREAM_READ);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); // unbind buffer

  // init ffmpeg stuff
  this->fmt_ctx = nullptr;
  this->video_stream = nullptr;
  this->sws_ctx = nullptr;

  avformat_alloc_output_context2(&this->fmt_ctx, nullptr, "mp4", filename);
  assert(this->fmt_ctx);

  // 2. Find MPEG-4 Encoder
  this->codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
  assert(this->codec);

  this->video_stream = avformat_new_stream(this->fmt_ctx, this->codec);
  assert(this->video_stream);

  this->codec_ctx = avcodec_alloc_context3(this->codec);
  assert(this->codec_ctx);

  // 3. Configure Encoder Context
  this->codec_ctx->width = width;
  this->codec_ctx->height = height;
  this->codec_ctx->time_base = (AVRational){1, (int)fps};
  this->codec_ctx->framerate = (AVRational){(int)fps, 1};
  this->codec_ctx->bit_rate = 4000000; // 4 Mbps target bitrate
  this->codec_ctx->gop_size = 12;      // Emit intra frame every 12 frames
  this->codec_ctx->max_b_frames = 1;
  this->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P; // MPEG-4 standard pixel format

  // Some formats want global headers if applicable
  if (this->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
    this->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  // Open the encoder
  int ret = avcodec_open2(this->codec_ctx, this->codec, nullptr);
  assert(ret >= 0);

  ret = avcodec_parameters_from_context(this->video_stream->codecpar,
                                        this->codec_ctx);
  assert(ret >= 0);

  this->video_stream->time_base = this->codec_ctx->time_base;

  // Open output file (if not using standardavio)
  if (!(this->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    ret = avio_open(&this->fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
    assert(ret >= 0);
  }

  // Write file header
  ret = avformat_write_header(this->fmt_ctx, nullptr);
  assert(ret >= 0);

  // 4. Allocate Frames and Packets
  this->frame = av_frame_alloc();
  assert(this->frame);
  this->frame->format = this->codec_ctx->pix_fmt;
  this->frame->width = this->codec_ctx->width;
  this->frame->height = this->codec_ctx->height;
  ret = av_frame_get_buffer(this->frame, 32);
  assert(ret >= 0);

  this->packet = av_packet_alloc();
  assert(this->packet);

  // 5. Initialize SwsContext for RGBA -> YUV420P conversion
  this->sws_ctx = sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height,
                                 AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr,
                                 nullptr, nullptr);
  assert(this->sws_ctx);

  return *this;
}

VideoExport &VideoExport::start() {
  this->recording = true;
  return *this;
}
VideoExport &VideoExport::stop() {
  this->recording = false;
  return *this;
}

VideoExport &VideoExport::update() {
  if (!this->recording)
    return *this;

  size_t frame_size = this->height * this->width * 4; // 4 because of rgba

  // read buffer
  glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_id);
  glReadPixels(0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  GLubyte *pixels = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

  // if there are any pixels, process them
  if (pixels) {
    // OpenGL renders upside down relative to standard video coordinate systems,
    // so we configure linesize pointers inversely for vertical flipping.
    const int src_linesize[4] = {(int)(this->width * 4), 0, 0, 0};
    // Point to the last row for bottom-up processing
    const uint8_t *src_slice[4] = {pixels +
                                       (this->height - 1) * src_linesize[0],
                                   nullptr, nullptr, nullptr};
    int inverted_src_linesize[4] = {-src_linesize[0], 0, 0, 0};

    // Convert RGBA -> YUV420P and handle vertical flip
    sws_scale(this->sws_ctx, src_slice, inverted_src_linesize, 0, this->height,
              this->frame->data, this->frame->linesize);

    this->frame->pts = this->frame_count++;

    // Send frame to encoder
    int ret = avcodec_send_frame(this->codec_ctx, this->frame);
    if (ret >= 0) {
      while (ret >= 0) {
        ret = avcodec_receive_packet(this->codec_ctx, this->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
          break;
        else if (ret < 0)
          break;

        av_packet_rescale_ts(this->packet, this->codec_ctx->time_base,
                             this->video_stream->time_base);
        this->packet->stream_index = this->video_stream->index;

        av_interleaved_write_frame(this->fmt_ctx, this->packet);
        av_packet_unref(this->packet);
      }
    }

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  }

  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); // unbind buffer

  return *this;
}

void VideoExport::deinit() {
  if (!this->recording && this->frame_count == 0) {
    // Clean up safely if never started/initialized fully
    return;
  }

  // Flush remaining frames from encoder
  avcodec_send_frame(this->codec_ctx, nullptr);
  int ret;
  while (true) {
    ret = avcodec_receive_packet(this->codec_ctx, this->packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    else if (ret < 0)
      break;

    av_packet_rescale_ts(this->packet, this->codec_ctx->time_base,
                         this->video_stream->time_base);
    this->packet->stream_index = this->video_stream->index;
    av_interleaved_write_frame(this->fmt_ctx, this->packet);
    av_packet_unref(this->packet);
  }

  // Write trailer
  av_write_trailer(this->fmt_ctx);

  // Free resources
  if (this->fmt_ctx && !(this->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    avio_closep(&this->fmt_ctx->pb);
  }

  avformat_free_context(this->fmt_ctx);
  avcodec_free_context(&this->codec_ctx);
  av_frame_free(&this->frame);
  av_packet_free(&this->packet);
  sws_freeContext(this->sws_ctx);

  glDeleteBuffers(1, &this->pbo_id);
  fclose(this->out_file);
}

}; // namespace sn
