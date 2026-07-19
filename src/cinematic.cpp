#include <filesystem>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

#include <imgui.h>

#include <framework/app.hpp>
#include <framework/camera.hpp>
#include <framework/gl/program.hpp>
#include <framework/imguiutil.hpp>
#include <framework/mesh.hpp>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct Object {
  std::string name;
  vec3 position;
  bool isActive;
  float scale;
};

class AssetManager {
private:
  // list of all objects
  std::unordered_map<std::string, Object> registry;

public:
  void registerObject(const std::string &name) {
    registry[name] = {name, vec3(0.0f), false, 1.0f};
  }

  void spawn(const std::string &name, const vec3 &position,
             float scale = 1.0f) {
    if (registry.find(name) != registry.end()) {
      registry[name].position = position;
      registry[name].isActive = true;
      registry[name].scale = scale;
    }
  }

  void despawn(const std::string &name) {
    if (registry.find(name) != registry.end()) {
      registry[name].isActive = false;
    }
  }

  const Object &getObject(const std::string &name) { return registry[name]; }

  void updateShaderUniforms(Program &program) {
    for (const auto &[name, object] : registry) {
      program.set("u_" + name + "_pos", object.position);
      program.set("u_" + name + "_active", object.isActive ? 1.0f : 0.0f);
      program.set("u_" + name + "_scale", object.scale);
    }
  }
};

struct CameraKeyframe {
  float time;
  vec3 position;
  vec3 target;
};

class CinematicSpline {
public:
  // mathematic way for smooth curves for camera
  // Catmull-Rom-Spline from p1 to p2
  // use p0 und p3 for smooth curve
  static vec3 interpolateCatmullRom(const vec3 &p0, const vec3 &p1,
                                    const vec3 &p2, const vec3 &p3, float t) {
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t * t +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t * t * t);
  }

  // uses interpolateCattmullRom on the keyframes to create the actuell
  // positions
  static vec3
  getInterpolatedPosition(const std::vector<CameraKeyframe> &keyframes,
                          float time, bool getTarget = false) {
    // no keyframe
    if (keyframes.empty())
      return vec3(0.0f);
    // just one postion
    if (keyframes.size() == 1)
      return getTarget ? keyframes[0].target : keyframes[0].position;

    // find current szene
    size_t idx = 0;
    while (idx < keyframes.size() - 1 && keyframes[idx + 1].time < time) {
      idx++;
    }
    // if no more keyframe in time get last keyframe
    if (idx >= keyframes.size() - 1) {
      return getTarget ? keyframes.back().target : keyframes.back().position;
    }

    // time in szene
    float startTime = keyframes[idx].time;
    float endTime = keyframes[idx + 1].time;
    float t = (time - startTime) / (endTime - startTime);

    // get 4 controllpoints
    size_t i0 = (idx == 0) ? idx : idx - 1;
    size_t i1 = idx;
    size_t i2 = idx + 1;
    size_t i3 = (idx + 2 >= keyframes.size()) ? i2 : idx + 2;

    if (getTarget) {
      return interpolateCatmullRom(keyframes[i0].target, keyframes[i1].target,
                                   keyframes[i2].target, keyframes[i3].target,
                                   t);
    } else {
      return interpolateCatmullRom(
          keyframes[i0].position, keyframes[i1].position,
          keyframes[i2].position, keyframes[i3].position, t);
    }
  }
};

class CinematicDirector {
public:
  std::vector<CameraKeyframe> timelineKeyframes;
  float currentTimelineEnd = 0.0f;
  // move camera with given speed
  void moveCameraTo(const vec3 &newPos, const vec3 &newTarget, float speed) {
    // last known position or (5,3,5) if empty
    vec3 startPos = timelineKeyframes.empty()
                        ? vec3(5.0f, 3.0f, 5.0f)
                        : timelineKeyframes.back().position;

    float distance = glm::distance(startPos, newPos);
    float duration = (speed > 0.0f) ? (distance / speed) : 1.0f;
    // first keyframe
    if (timelineKeyframes.empty()) {
      timelineKeyframes.push_back({0.0f, startPos, newTarget});
    }
    // grows with timeline
    currentTimelineEnd += duration;
    // add keyframe
    timelineKeyframes.push_back({currentTimelineEnd, newPos, newTarget});
  }
};

// A time-windowed trigger, e.g. "object X is spawned between t=6s and t=25s".
// onActive/onInactive are called EVERY frame (not just on the transition),
// mirroring the old spawn()/despawn() pattern where both were idempotent -
// this way scrubbing/restarting the timeline can never leave stale state.
struct SceneWindowEvent {
  float startTime;
  float endTime;
  std::function<void()> onActive;
  std::function<void()> onInactive;
};

// A one-time trigger, e.g. "play the explosion the first frame we cross
// t=10s". `fired` guards against re-triggering every frame after that; call
// Scene::reset() to clear it when replaying the scene from the start.
struct SceneOneShotEvent {
  float triggerTime;
  std::function<void()> onTrigger;
  bool fired = false;
};

// One cinematic "shot": a camera path (via `director`) plus all the
// spawn/despawn/laser/explosion triggers timed against that same clock.
// Bundling them together means a new shot is just a new Scene instance
// instead of more branches inside MainApp::render().
struct Scene {
  std::string name;
  CinematicDirector director;
  std::vector<SceneWindowEvent> windowEvents;
  std::vector<SceneOneShotEvent> oneShotEvents;

  // Total length of the camera path; the scene is "done" once flight time
  // passes this.
  float duration() const { return director.currentTimelineEnd; }

  // Call once per frame with the current flight time to drive all triggers.
  void update(float flightTime) {
    for (auto &event : windowEvents) {
      if (flightTime >= event.startTime && flightTime < event.endTime) {
        event.onActive();
      } else {
        event.onInactive();
      }
    }
    for (auto &event : oneShotEvents) {
      if (!event.fired && flightTime >= event.triggerTime) {
        event.onTrigger();
        event.fired = true;
      }
    }
  }

  // Clears one-shot "fired" flags so the scene can be played again from t=0.
  void reset() {
    for (auto &event : oneShotEvents) {
      event.fired = false;
    }
  }
};