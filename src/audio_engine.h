#pragma once

#include <functional>
#include <string>
#include <vector>

#include "audio/miniaudio.h"

/** A single scheduled event on the cinematic timeline: once `update()` is
 * called with a timelineSeconds >= time, `action` fires exactly once. Call
 * resetSchedule() when the timeline restarts (e.g. autocam looping back to
 * the start) to rearm every event. */
struct TimedAudioEvent {
  float time;
  std::function<void()> action;
  bool triggered = false;
};

/** Thin wrapper around miniaudio's engine: one persistent looping music
 * voice, fire-and-forget one-shot SFX, and a timeline of scheduled events
 * (music cues, sound effects) driven by the cinematic director's clock. */
class AudioEngine {
public:
  bool init();
  void shutdown();
  bool isAvailable() const { return _available; }

  /** (Re)starts the background track, looping, at the given volume. */
  void playMusic(const std::string &path, float volume = 1.0f);
  void stopMusic();
  void setMusicVolume(float volume);

  /** Fire-and-forget sound effect; the engine manages its own lifetime. */
  void playSFX(const std::string &path);

  /** Schedules `action` to run once when the timeline reaches `time`. */
  void schedule(float time, std::function<void()> action);
  /** Convenience: schedules a one-shot SFX at `time`. */
  void scheduleSFX(float time, const std::string &path);
  /** Rearms all scheduled events so they can fire again on the next loop. */
  void resetSchedule();
  /** Fires any scheduled events whose time has been reached. Call every
   * frame with the current position on the cinematic timeline. */
  void update(float timelineSeconds);

private:
  ma_engine _engine{};
  ma_sound _music{};
  bool _available = false;
  bool _musicLoaded = false;
  std::vector<TimedAudioEvent> _schedule;
};
