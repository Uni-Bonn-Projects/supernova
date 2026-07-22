#pragma once

#include <cstdint>

#include "framework/mesh.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace sn {

struct VideoEncoder {
  uint32_t width;
  uint32_t height;

  int64_t frame_count;
  AVFormatContext *fmt_ctx;
  AVCodecContext *codec_ctx;
  AVStream *video_stream;
  AVFrame *frame;
  AVPacket *packet;
  SwsContext *sws_ctx;

  VideoEncoder &init(const char *filename, const uint32_t width,
                     const uint32_t height, const uint32_t fps);

  VideoEncoder &encode(GLubyte *pixels);

  void deinit();
};

struct VideoExport {
  VideoEncoder encoder;

  // whether or not an export is currently running
  bool recording;

  // video vars
  uint32_t width;
  uint32_t height;
  uint32_t fps;

  // open stuff
  GLuint pbo_id;

  VideoExport &init(const char *filename, const uint32_t width,
                    const uint32_t height, const uint32_t fps);

  VideoExport &start();
  VideoExport &stop();

  VideoExport &update();

  void deinit();
};

}; // namespace sn
