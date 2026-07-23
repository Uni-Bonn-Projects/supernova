#include <iostream>

#include "audio_engine.h"

bool AudioEngine::init() {
  ma_result result = ma_engine_init(NULL, &_engine);
  _available = result == MA_SUCCESS;
  if (!_available) {
    std::cerr << "Audio engine init failed: " << result << "\n";
    return _available;
  }

  // Dedicated mixer node for all SFX, so their volume can be controlled
  // separately from the music. If it fails, SFX just fall back to routing
  // straight to the engine endpoint (see playSFX).
  if (ma_sound_group_init(&_engine, 0, NULL, &_sfxGroup) == MA_SUCCESS) {
    _sfxGroupInit = true;
  } else {
    std::cerr << "SFX group init failed; SFX volume control disabled\n";
  }

  return _available;
}

void AudioEngine::shutdown() {
  if (!_available)
    return;
  if (_musicLoaded) {
    ma_sound_uninit(&_music);
    _musicLoaded = false;
  }
  if (_sfxGroupInit) {
    ma_sound_group_uninit(&_sfxGroup);
    _sfxGroupInit = false;
  }
  ma_engine_uninit(&_engine);
  _available = false;
}

void AudioEngine::playMusic(const std::string &path, float volume) {
  // while exporting the score is muxed into the file instead (see
  // VideoExport::init), so nothing should come out of the speakers here
  if (silent || !_available)
    return;
  if (_musicLoaded) {
    ma_sound_uninit(&_music);
    _musicLoaded = false;
  }
  ma_result result = ma_sound_init_from_file(
      &_engine, path.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, &_music);
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to load music '" << path << "': " << result << "\n";
    return;
  }
  ma_sound_set_looping(&_music, MA_TRUE);
  ma_sound_set_volume(&_music, volume);
  ma_sound_start(&_music);
  _musicLoaded = true;
}

void AudioEngine::stopMusic() {
  if (_musicLoaded)
    ma_sound_stop(&_music);
}

void AudioEngine::setMusicVolume(float volume) {
  if (_musicLoaded)
    ma_sound_set_volume(&_music, volume);
}

void AudioEngine::playSFX(const std::string &path) {
  // The sink runs before the availability check on purpose: an export must
  // still get its sound effects on machines with no working audio device.
  if (sfxSink)
    sfxSink(path, _sfxVolume);
  if (silent || !_available)
    return;
  // Route through the SFX group when available so setSFXVolume() applies;
  // otherwise play straight to the engine endpoint (full volume).
  ma_node *node = _sfxGroupInit ? (ma_node *)&_sfxGroup : NULL;
  ma_result result = ma_engine_play_sound_ex(&_engine, path.c_str(), node, 0);
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to play SFX '" << path << "': " << result << "\n";
  }
}

void AudioEngine::setSFXVolume(float volume) {
  _sfxVolume = volume;
  if (_sfxGroupInit)
    ma_sound_group_set_volume(&_sfxGroup, volume);
}

void AudioEngine::schedule(float time, std::function<void()> action) {
  _schedule.push_back({time, std::move(action), false});
}

void AudioEngine::scheduleSFX(float time, const std::string &path) {
  schedule(time, [this, path]() { playSFX(path); });
}

void AudioEngine::resetSchedule() {
  for (auto &event : _schedule)
    event.triggered = false;
}

void AudioEngine::update(float timelineSeconds) {
  for (auto &event : _schedule) {
    if (!event.triggered && timelineSeconds >= event.time) {
      event.action();
      event.triggered = true;
    }
  }
}
