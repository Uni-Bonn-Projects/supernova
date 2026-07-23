#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "framework/mesh.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace sn {

// Builds the whole soundtrack of the export in memory, as 48kHz stereo
// interleaved float. The music is laid down as a looping bed and every sound
// effect is mixed on top at the exact second the animation triggered it - so
// the audio can't drift against the picture no matter how slowly the offline
// render actually runs.
struct AudioMixer {
  static constexpr uint32_t kSampleRate = 48000;
  static constexpr uint32_t kChannels = 2;

  // interleaved stereo, 2 floats per sample frame
  std::vector<float> mix;
  // decoded music bed, looped into `mix` as it grows
  std::vector<float> music;
  float music_volume = 1.0f;
  // sample frames of `mix` that already have the music bed written into them
  size_t music_filled = 0;
  // decoded SFX, keyed by path (the explosion is played a dozen times)
  std::unordered_map<std::string, std::vector<float>> sfx_cache;

  void set_music(const char *path, float volume);

  // Mixes `path` into the buffer starting at `at_seconds` on the video's
  // timeline.
  void add_sfx(const char *path, double at_seconds, float volume);

  // Grows the buffer to hold `frames` sample frames, filling anything new
  // with the (looping) music bed.
  void ensure(size_t frames);
};

struct VideoEncoder {
  uint32_t width;
  uint32_t height;
  uint32_t fps;

  int64_t frame_count;
  AVFormatContext *fmt_ctx;
  AVCodecContext *codec_ctx;
  AVStream *video_stream;
  AVFrame *frame;
  AVPacket *packet;
  SwsContext *sws_ctx;

  // audio side: an AAC stream in the same file, fed from `mixer`
  AudioMixer mixer;
  AVCodecContext *audio_ctx;
  AVStream *audio_stream;
  AVFrame *audio_frame;
  // sample frames of the mix that have already been handed to the encoder
  int64_t audio_pos;

  VideoEncoder &init(const char *filename, const uint32_t width,
                     const uint32_t height, const uint32_t fps,
                     const char *music_path, float music_volume);

  VideoEncoder &encode(GLubyte *pixels);

  // Current position on the video timeline, i.e. the timestamp a sound
  // effect triggered by the frame being rendered right now must land on.
  double time() const { return (double)this->frame_count / (double)this->fps; }

  void deinit();

  // Encodes everything of the mix that lies before `seconds`, so audio and
  // video packets end up interleaved in the file instead of the whole audio
  // track being appended after the last frame.
  void encode_audio_until(double seconds);
  void flush_audio();
};

struct VideoExport {
  VideoEncoder encoder;

  // whether or not an export is currently running
  bool recording;
  // whether init() ran - deinit() is called unconditionally on shutdown
  bool active = false;

  // video vars
  uint32_t width;
  uint32_t height;
  uint32_t fps;

  // open stuff
  GLuint pbo_id;
  // Offscreen render target: the export resolution (2160p) is much larger
  // than the window, so the frames can't be read from the default
  // framebuffer.
  GLuint fbo_id;
  GLuint color_tex;
  GLuint depth_rbo;
  // window viewport, saved across the offscreen frame
  GLint saved_viewport[4];

  VideoExport &init(const char *filename, const uint32_t width,
                    const uint32_t height, const uint32_t fps,
                    const char *music_path = nullptr,
                    float music_volume = 1.0f);

  VideoExport &start();
  VideoExport &stop();

  // Redirects rendering into the offscreen target. Call at the very top of
  // render(), before the frame's first draw call.
  VideoExport &begin_frame();

  // Small enough to stay well inside the i915 watchdog on this project's
  // integrated GPU; see draw_tiled().
  static constexpr uint32_t kDefaultTileSize = 256;

  // Edge length of one raymarching tile, see draw_tiled(). 0 disables tiling
  // entirely. Set from `--tile N`, because the right value is a property of
  // the GPU the export runs on, not of the project.
  uint32_t tile_size = kDefaultTileSize;

  // Runs `draw` once per tile of the frame, with the scissor box limiting it
  // to that tile.
  //
  // A 3840x2160 fullscreen raymarch is a single draw call that shades 8.3
  // million pixels at up to uSteps iterations each. On an integrated GPU
  // that takes many seconds, which is far past the i915 preemption timeout
  // (~640ms): the driver declares a GPU hang, resets the context, and every
  // frame afterwards reads back pure black. Splitting the frame into tiles
  // keeps each submission short enough to complete normally.
  //
  // The viewport stays at the full export resolution and only the scissor
  // box moves, so gl_FragCoord (and with it everything the raytracer derives
  // from uResolution) is identical to an untiled render - the tiles are
  // invisible in the result, they only change how the work is submitted.
  //
  // A GPU fast enough to draw a whole frame inside its driver's watchdog
  // doesn't need this, and the glFinish() per tile costs it real time, so
  // `--tile` can widen the tiles or switch them off (see main()).
  template <typename Draw> void draw_tiled(Draw &&draw) {
    if (!this->recording || this->tile_size == 0) {
      draw();
      return;
    }
    glEnable(GL_SCISSOR_TEST);
    for (uint32_t y = 0; y < this->height; y += this->tile_size) {
      for (uint32_t x = 0; x < this->width; x += this->tile_size) {
        // the last row/column may reach past the framebuffer; GL clips the
        // scissor box, so tile sizes that don't divide the frame are fine
        glScissor(x, y, this->tile_size, this->tile_size);
        draw();
        // force this tile to finish before the next is queued, otherwise the
        // driver merges them back into one long-running batch
        glFinish();
      }
    }
    glDisable(GL_SCISSOR_TEST);
  }

  VideoExport &update();

  // Mixes a sound effect into the exported audio track at the timestamp of
  // the frame currently being rendered.
  void record_sfx(const std::string &path, float volume);

  void deinit();
};

}; // namespace sn
