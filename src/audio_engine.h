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

  /** Fire-and-forget sound effect; the engine manages its own lifetime.
   * Routed through a dedicated SFX group so its volume can be set
   * independently of the music via setSFXVolume(). */
  void playSFX(const std::string &path);

  /** Tap on playSFX(), called with the effect's path and its current group
   * volume before the sound is played. The video export uses this to mix
   * every effect into the exported audio track at the timestamp of the frame
   * that triggered it, without any of the callers having to know an export
   * is running. */
  std::function<void(const std::string &path, float volume)> sfxSink;

  /** Suppresses live playback (but not sfxSink). Set while exporting: an
   * offline render takes far longer than real time, so playing the
   * cinematic out of the speakers alongside it is just noise. */
  bool silent = false;
  /** Sets the volume of all SFX (1.0 = unchanged, 0.0 = muted, >1 boosts).
   * Independent of the music volume. */
  void setSFXVolume(float volume);

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
  ma_sound_group _sfxGroup{}; // all SFX mix through here for a shared volume
  bool _available = false;
  bool _musicLoaded = false;
  bool _sfxGroupInit = false;
  float _sfxVolume = 1.0f; // mirrors the SFX group volume, for sfxSink
  std::vector<TimedAudioEvent> _schedule;
};
