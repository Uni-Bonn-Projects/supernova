#include <iostream>

#include "audio_engine.h"

bool AudioEngine::init() {
  ma_result result = ma_engine_init(NULL, &_engine);
  _available = result == MA_SUCCESS;
  if (!_available) {
    std::cerr << "Audio engine init failed: " << result << "\n";
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
  ma_engine_uninit(&_engine);
  _available = false;
}

void AudioEngine::playMusic(const std::string &path, float volume) {
  if (!_available)
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
  if (!_available)
    return;
  ma_result result = ma_engine_play_sound(&_engine, path.c_str(), NULL);
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to play SFX '" << path << "': " << result << "\n";
  }
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
