#include "video_export.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include <libavutil/opt.h>
}

#include "framework/mesh.hpp"

namespace sn {

////////////////////////////////////////////////////////////////////////////////
// audio mixing
////////////////////////////////////////////////////////////////////////////////

// Decodes any audio file libavformat can read into 48kHz stereo interleaved
// float, the format the mixer works in. The two assets we have differ in both
// sample rate and channel count (48k stereo music, 44.1k mono explosion), so
// resampling here is what lets them be added together sample by sample.
static bool decode_audio_file(const char *path, std::vector<float> &out) {
  AVFormatContext *fmt = nullptr;
  if (avformat_open_input(&fmt, path, nullptr, nullptr) < 0) {
    fprintf(stderr, "[export] could not open audio file '%s'\n", path);
    return false;
  }
  if (avformat_find_stream_info(fmt, nullptr) < 0) {
    avformat_close_input(&fmt);
    return false;
  }

  const AVCodec *dec = nullptr;
  int idx = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
  if (idx < 0 || !dec) {
    avformat_close_input(&fmt);
    return false;
  }

  AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);
  avcodec_parameters_to_context(dec_ctx, fmt->streams[idx]->codecpar);
  if (avcodec_open2(dec_ctx, dec, nullptr) < 0) {
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt);
    return false;
  }

  AVChannelLayout out_layout;
  av_channel_layout_default(&out_layout, AudioMixer::kChannels);
  SwrContext *swr = nullptr;
  swr_alloc_set_opts2(&swr, &out_layout, AV_SAMPLE_FMT_FLT,
                      AudioMixer::kSampleRate, &dec_ctx->ch_layout,
                      dec_ctx->sample_fmt, dec_ctx->sample_rate, 0, nullptr);
  swr_init(swr);

  AVPacket *pkt = av_packet_alloc();
  AVFrame *frm = av_frame_alloc();
  std::vector<float> chunk;

  auto drain = [&](AVFrame *in) {
    int in_samples = in ? in->nb_samples : 0;
    int max_out = swr_get_out_samples(swr, in_samples);
    if (max_out <= 0)
      return;
    chunk.resize((size_t)max_out * AudioMixer::kChannels);
    uint8_t *dst = (uint8_t *)chunk.data();
    int got = swr_convert(swr, &dst, max_out,
                          in ? (const uint8_t **)in->extended_data : nullptr,
                          in_samples);
    if (got > 0)
      out.insert(out.end(), chunk.begin(),
                 chunk.begin() + (size_t)got * AudioMixer::kChannels);
  };

  while (av_read_frame(fmt, pkt) >= 0) {
    if (pkt->stream_index == idx && avcodec_send_packet(dec_ctx, pkt) >= 0) {
      while (avcodec_receive_frame(dec_ctx, frm) >= 0)
        drain(frm);
    }
    av_packet_unref(pkt);
  }
  // flush the decoder, then whatever the resampler is still holding
  avcodec_send_packet(dec_ctx, nullptr);
  while (avcodec_receive_frame(dec_ctx, frm) >= 0)
    drain(frm);
  drain(nullptr);

  av_frame_free(&frm);
  av_packet_free(&pkt);
  swr_free(&swr);
  av_channel_layout_uninit(&out_layout);
  avcodec_free_context(&dec_ctx);
  avformat_close_input(&fmt);
  return !out.empty();
}

void AudioMixer::set_music(const char *path, float volume) {
  this->music_volume = volume;
  this->music.clear();
  if (path && !decode_audio_file(path, this->music))
    fprintf(stderr, "[export] no music track, exporting SFX only\n");
}

void AudioMixer::ensure(size_t frames) {
  if (frames <= this->music_filled)
    return;

  this->mix.resize(frames * kChannels, 0.0f);
  const size_t music_frames = this->music.size() / kChannels;
  for (size_t f = this->music_filled; f < frames; f++) {
    if (music_frames == 0)
      break;
    // the score loops for as long as the cinematic runs
    const size_t src = (f % music_frames) * kChannels;
    for (uint32_t c = 0; c < kChannels; c++)
      this->mix[f * kChannels + c] = this->music[src + c] * this->music_volume;
  }
  this->music_filled = frames;
}

void AudioMixer::add_sfx(const char *path, double at_seconds, float volume) {
  auto it = this->sfx_cache.find(path);
  if (it == this->sfx_cache.end()) {
    std::vector<float> samples;
    decode_audio_file(path, samples);
    it = this->sfx_cache.emplace(path, std::move(samples)).first;
  }
  const std::vector<float> &samples = it->second;
  if (samples.empty())
    return;

  const size_t start = (size_t)(at_seconds * (double)kSampleRate);
  const size_t frames = samples.size() / kChannels;
  // the music bed has to exist under the effect before it can be added on top
  this->ensure(start + frames);
  for (size_t i = 0; i < samples.size(); i++)
    this->mix[start * kChannels + i] += samples[i] * volume;
}

////////////////////////////////////////////////////////////////////////////////
// video / muxing
////////////////////////////////////////////////////////////////////////////////

VideoEncoder &VideoEncoder::init(const char *filename, const uint32_t width,
                                 const uint32_t height, const uint32_t fps,
                                 const char *music_path, float music_volume) {
  this->width = width;
  this->height = height;
  this->fps = fps;
  this->frame_count = 0;
  this->audio_pos = 0;

  this->fmt_ctx = nullptr;
  avformat_alloc_output_context2(&this->fmt_ctx, NULL, "mp4", filename);
  assert(this->fmt_ctx);

  this->video_stream = avformat_new_stream(this->fmt_ctx, NULL);
  assert(this->video_stream);
  this->video_stream->time_base = AVRational{1, (int)fps};

  // libx264 rather than the generic H.264 lookup so we can drive it with
  // preset/crf below; avcodec_find_encoder is the fallback for builds
  // without x264.
  auto codec = avcodec_find_encoder_by_name("libx264");
  if (!codec)
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  assert(codec);
  this->codec_ctx = avcodec_alloc_context3(codec);
  assert(this->codec_ctx);

  // 3. Configure Encoder Context
  this->codec_ctx->width = width;
  this->codec_ctx->height = height;
  this->codec_ctx->time_base = AVRational{1, (int)fps};
  this->codec_ctx->framerate = AVRational{(int)fps, 1};
  this->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  this->codec_ctx->gop_size = (int)fps * 2;
  this->codec_ctx->max_b_frames = 2;
  // Constant quality instead of a fixed bitrate: at 2160p a 4 Mbps cap would
  // smear every explosion, and the file is a one-off deliverable rather than
  // something that has to hit a bandwidth budget.
  av_opt_set(this->codec_ctx->priv_data, "preset", "medium", 0);
  av_opt_set(this->codec_ctx->priv_data, "crf", "18", 0);
  // mp4 keeps the codec extradata in the header, not in the stream
  if (this->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    this->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  // Open the encoder
  int ret = avcodec_open2(this->codec_ctx, codec, NULL);
  assert(ret >= 0);

  ret = avcodec_parameters_from_context(this->video_stream->codecpar,
                                        this->codec_ctx);
  assert(ret >= 0);

  // Audio stream: AAC, fed from the mixer during encode(). Has to be set up
  // before the header is written, since mp4 records both streams there.
  this->mixer.set_music(music_path, music_volume);
  auto audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
  assert(audio_codec);
  this->audio_stream = avformat_new_stream(this->fmt_ctx, NULL);
  assert(this->audio_stream);
  this->audio_stream->time_base = AVRational{1, (int)AudioMixer::kSampleRate};
  this->audio_ctx = avcodec_alloc_context3(audio_codec);
  assert(this->audio_ctx);
  this->audio_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
  this->audio_ctx->sample_rate = (int)AudioMixer::kSampleRate;
  this->audio_ctx->bit_rate = 192000;
  this->audio_ctx->time_base = AVRational{1, (int)AudioMixer::kSampleRate};
  av_channel_layout_default(&this->audio_ctx->ch_layout, AudioMixer::kChannels);
  if (this->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    this->audio_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  ret = avcodec_open2(this->audio_ctx, audio_codec, NULL);
  assert(ret >= 0);
  ret = avcodec_parameters_from_context(this->audio_stream->codecpar,
                                        this->audio_ctx);
  assert(ret >= 0);

  this->audio_frame = av_frame_alloc();
  assert(this->audio_frame);
  this->audio_frame->format = this->audio_ctx->sample_fmt;
  this->audio_frame->sample_rate = this->audio_ctx->sample_rate;
  this->audio_frame->nb_samples = this->audio_ctx->frame_size;
  av_channel_layout_copy(&this->audio_frame->ch_layout,
                         &this->audio_ctx->ch_layout);
  ret = av_frame_get_buffer(this->audio_frame, 0);
  assert(ret >= 0);

  // Open output file (if not using standardavio)
  ret = avio_open(&this->fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
  assert(ret >= 0);

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

VideoEncoder &VideoEncoder::encode(GLubyte *pixels) {
  // OpenGL renders upside down relative to standard video coordinate systems,
  // so we configure linesize pointers inversely for vertical flipping.
  const int src_linesize[4] = {(int)(this->width * 4), 0, 0, 0};
  // Point to the last row for bottom-up processing
  const uint8_t *src_slice[4] = {pixels + (this->height - 1) * src_linesize[0],
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

  // Keep the audio track level with the picture. Everything up to the frame
  // we just wrote is final (sound effects for it were mixed in while it was
  // being rendered), so it is safe to encode.
  this->encode_audio_until(this->time());

  return *this;
}

void VideoEncoder::encode_audio_until(double seconds) {
  const int frame_size = this->audio_ctx->frame_size;
  const int64_t target = (int64_t)(seconds * (double)AudioMixer::kSampleRate);

  while (this->audio_pos + frame_size <= target) {
    this->mixer.ensure((size_t)(this->audio_pos + frame_size));

    int ret = av_frame_make_writable(this->audio_frame);
    assert(ret >= 0);
    // AAC wants planar samples, the mixer stores them interleaved
    float *left = (float *)this->audio_frame->data[0];
    float *right = (float *)this->audio_frame->data[1];
    const float *src =
        this->mixer.mix.data() + this->audio_pos * AudioMixer::kChannels;
    for (int i = 0; i < frame_size; i++) {
      // the score and a full-volume explosion can sum past full scale; clamp
      // rather than letting the encoder wrap it into distortion
      left[i] = fminf(fmaxf(src[i * 2], -1.0f), 1.0f);
      right[i] = fminf(fmaxf(src[i * 2 + 1], -1.0f), 1.0f);
    }
    this->audio_frame->pts = this->audio_pos;
    this->audio_pos += frame_size;

    ret = avcodec_send_frame(this->audio_ctx, this->audio_frame);
    while (ret >= 0) {
      ret = avcodec_receive_packet(this->audio_ctx, this->packet);
      if (ret < 0)
        break;
      av_packet_rescale_ts(this->packet, this->audio_ctx->time_base,
                           this->audio_stream->time_base);
      this->packet->stream_index = this->audio_stream->index;
      av_interleaved_write_frame(this->fmt_ctx, this->packet);
      av_packet_unref(this->packet);
    }
  }
}

void VideoEncoder::flush_audio() {
  // pad out to the last video frame, then drain the encoder
  this->encode_audio_until(this->time() + 1.0 / (double)this->fps);
  avcodec_send_frame(this->audio_ctx, nullptr);
  while (true) {
    int ret = avcodec_receive_packet(this->audio_ctx, this->packet);
    if (ret < 0)
      break;
    av_packet_rescale_ts(this->packet, this->audio_ctx->time_base,
                         this->audio_stream->time_base);
    this->packet->stream_index = this->audio_stream->index;
    av_interleaved_write_frame(this->fmt_ctx, this->packet);
    av_packet_unref(this->packet);
  }
}

void VideoEncoder::deinit() {
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

  this->flush_audio();

  // Write trailer
  av_write_trailer(this->fmt_ctx);

  // Free resources
  avio_closep(&this->fmt_ctx->pb);

  avformat_free_context(this->fmt_ctx);
  avcodec_free_context(&this->codec_ctx);
  avcodec_free_context(&this->audio_ctx);
  av_frame_free(&this->frame);
  av_frame_free(&this->audio_frame);
  av_packet_free(&this->packet);
  sws_freeContext(this->sws_ctx);
}

////////////////////////////////////////////////////////////////////////////////

VideoExport &VideoExport::init(const char *filename, const uint32_t width,
                               const uint32_t height, const uint32_t fps,
                               const char *music_path, float music_volume) {
  this->recording = false;
  this->active = true;

  this->width = width;
  this->height = height;
  this->fps = fps;

  // init opengl pixel buffer
  size_t frame_size = this->height * this->width * 4; // 4 because of rgba
  glGenBuffers(1, &this->pbo_id);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_id);
  glBufferData(GL_PIXEL_PACK_BUFFER, frame_size, NULL, GL_STREAM_READ);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); // unbind buffer

  // Offscreen target at the full export resolution. The color attachment is
  // sRGB because the framework enables GL_FRAMEBUFFER_SRGB globally - with a
  // plain RGBA8 texture the recorded frames would come out darker than what
  // the window shows.
  glGenTextures(1, &this->color_tex);
  glBindTexture(GL_TEXTURE_2D, this->color_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  glGenRenderbuffers(1, &this->depth_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, this->depth_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  glGenFramebuffers(1, &this->fbo_id);
  glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         this->color_tex, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, this->depth_rbo);
  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  this->encoder.init(filename, width, height, fps, music_path, music_volume);

  return *this;
}

VideoExport &VideoExport::start() {
  this->recording = true;
  return *this;
}
VideoExport &VideoExport::stop() {
  if (this->recording) {
    // begin_frame() may have left the offscreen target bound for a frame we
    // are no longer going to record
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(this->saved_viewport[0], this->saved_viewport[1],
               this->saved_viewport[2], this->saved_viewport[3]);
  }
  this->recording = false;
  return *this;
}

VideoExport &VideoExport::begin_frame() {
  if (!this->recording)
    return *this;

  // everything the frame draws goes into the 2160p target instead of the
  // (much smaller) window; the window's own viewport is restored in update()
  // so ImGui still draws correctly afterwards
  glGetIntegerv(GL_VIEWPORT, this->saved_viewport);
  glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
  glViewport(0, 0, this->width, this->height);

  return *this;
}

void VideoExport::record_sfx(const std::string &path, float volume) {
  if (!this->recording)
    return;
  // The timestamp is the frame currently being rendered, not wall clock -
  // that is what keeps the effect on the same instant as the explosion that
  // triggered it, however long the offline render takes per frame.
  this->encoder.mixer.add_sfx(path.c_str(), this->encoder.time(), volume);
}

VideoExport &VideoExport::update() {
  if (!this->recording)
    return *this;

  size_t frame_size = this->height * this->width * 4; // 4 because of rgba

  // read buffer
  glBindFramebuffer(GL_READ_FRAMEBUFFER, this->fbo_id);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, this->pbo_id);
  glReadPixels(0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  GLubyte *pixels = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

  // if there are any pixels, process them
  if (pixels) {
    this->encoder.encode(pixels);
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  }

  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); // unbind buffer
  // hand the window back its framebuffer and viewport, so ImGui and a normal
  // on-screen frame still show up
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(this->saved_viewport[0], this->saved_viewport[1],
             this->saved_viewport[2], this->saved_viewport[3]);

  return *this;
}

void VideoExport::deinit() {
  if (!this->active)
    return;
  this->active = false;

  this->encoder.deinit();
  glDeleteBuffers(1, &this->pbo_id);
  glDeleteFramebuffers(1, &this->fbo_id);
  glDeleteTextures(1, &this->color_tex);
  glDeleteRenderbuffers(1, &this->depth_rbo);
}

}; // namespace sn
