#include <filesystem>
#include <string>
#include <vector>

#include <imgui.h>

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <framework/app.hpp>
#include <framework/camera.hpp>
#include <framework/gl/program.hpp>
#include <framework/imguiutil.hpp>
#include <framework/mesh.hpp>
#include <framework/objparser.hpp>

#include "audio_engine.h"
#include "explosions.h"
#include "multifile_shaders.h"
#include "oldman.h"
#include "snmesh.h"

#include "cinematic.cpp" // source file import?

using namespace glm;

namespace sn {

struct MainApp : public App {
  Program program;
  Mesh mesh;
  Camera camera;
  AssetManager assetManager;

  // All cinematic shots, chained back-to-back on one global clock
  // (uFlightTime): scene N starts the instant scene N-1's duration()
  // elapses, with no per-scene reset. `currentScene` tracks whichever
  // scene that global time currently falls inside - see render() and
  // sceneStartTime() below. Add more Scene entries in the constructor to
  // script additional shots; push order is playback order.
  std::vector<Scene> scenes;
  int currentScene = 0;
  // Scene-local time of the frame currently being driven; see render().
  float currentSceneTime = 0.0f;

  // Sum of scenes[0..idx)'s durations - i.e. the global flight time at
  // which scenes[idx] starts playing.
  float sceneStartTime(int idx) const {
    float t = 0.0f;
    for (int i = 0; i < idx; i++)
      t += scenes[i].duration();
    return t;
  }

  void resetTimeline() {
    uFlightTime = 0.0f;
    currentScene = 0;
    for (auto &scene : scenes)
      scene.reset();
    audio.resetSchedule();

    // Scenes only unwind runtime state (spawns, uInLinearSpace,
    // uLaserActive, ...) via SceneWindowEvent::onInactive, but several
    // window events (e.g. linearSpace's) only define onActive - so
    // whatever was on when playback stopped stays on. Force everything
    // back to its resting state here instead of relying on every event to
    // pair its own onInactive.
    assetManager.despawnAll();
    uInLinearSpace = false;
    program.set("uInLinearSpace", uInLinearSpace);
    uLaserActive = false;
    program.set("uLaserActive", uLaserActive);
    uAttackerLaserActive = false;
    program.set("uAttackerLaserActive", uAttackerLaserActive);

    uAttackerAliveMask = kAliveMaskAll;
    program.set("uAttackerAliveMask", uAttackerAliveMask);
    uOldmanBeamActive = false;
    program.set("uOldmanBeamActive", uOldmanBeamActive);
    uOldmanBeamTargets = ivec3(-1);
    program.set("uOldmanBeamTargets", uOldmanBeamTargets);
    uAttackerLaserTarget = fightPos;
    program.set("uAttackerLaserTarget", uAttackerLaserTarget);
    uOldmanBeamStart = fightPos + kOldmanMuzzleOffset;
    program.set("uOldmanBeamStart", uOldmanBeamStart);

    // The chase translates the CSG mesh incrementally, so unlike everything
    // else here it can't just be re-set to a value - it has to be moved back
    // by however far it drifted, or a replay starts with oldman displaced.
    if (oldmanWorldPos != fightPos) {
      oldman.move(1.0f, fightPos - oldmanWorldPos);
      oldman.update(program);
      oldmanWorldPos = fightPos;
    }
    attackerSwarmPos = fightPos + vec3(800.0f, 0.0f, 0.0f);
  }
  Explosions explosions;
  AudioEngine audio;
  Oldman oldman;

  // timed explosion tied to the audio cue below, both keyed off uFlightTime
  const float kAttackExplosionTime = 10.0f;

  vec3 uLightDir = normalize(vec3(1.0));
  float uNear = 1.0;
  float uFar = 1'000'000.0;
  int uSteps = 1000;
  float uEpsilon = 0.0001;
  float uNormalEps = 0.1;
  bool uAutoCam = false;
  float uFlightTime = 0.0f;
  float uWarp = 0.75f;
  float uScan = 0.75f;
  vec3 fightPos = vec3(-600.0, -600.0, -600.0);
  bool uInLinearSpace = false;
  float uFocusDistance = 500.0f;
  float uApertureSize = 0.0f;
  int uFocusSamples = 1;

  float uSunSize = 1000.0f;

  // Offset from fightPos to the top of the oldman mesh (top_dome is a
  // radius-100 sphere centered on the mesh's own origin, see oldman.cpp),
  // i.e. roughly the muzzle the laser fires from.
  static constexpr vec3 kOldmanMuzzleOffset = vec3(0.0f, 100.0f, 0.0f);
  static constexpr float kLaserLength = 50000.0f; // km, well past the moon
  bool uLaserActive = false; // not visible before its triggered
  vec3 uLaserStart = fightPos + kOldmanMuzzleOffset;
  vec3 uLaserEnd = uLaserStart;
  float uLaserRadius = 20.0f;
  vec3 uLaserColor = vec3(0.1f, 0.4f, 1.0f); // blue halo
  vec3 uLaserCoreColor = vec3(0.8f, 0.9f, 1.0f);
  float uLaserGlowRadius = 7.5f;
  float uLaserGlowIntensity = 7.5f;
  // Cinematic firing window (seconds into uFlightTime); kept separate from
  // uLaserActive so ImGui can still force it on/off manually while uAutoCam
  // is off.
  float uLaserFireStart = 15.0f;
  float uLaserFireEnd = 20.0f;

  // attacker swarm's return fire: 13 small beams (one per attacker sphere,
  // positions computed in-shader) converging on oldman's muzzle
  bool uAttackerLaserActive = false;
  // Center of the oldman mesh; each beam aims at a different point scattered
  // around it
  vec3 uAttackerLaserTarget = fightPos;
  float uAttackerLaserRadius = 1.5f;
  vec3 uAttackerLaserColor = vec3(0.9f, 0.2f, 0.05f); // red/orange halo
  vec3 uAttackerLaserCoreColor = vec3(1.0f, 0.8f, 0.3f);
  float uAttackerLaserGlowRadius = 1.0f;
  float uAttackerLaserGlowIntensity = 1.0f;

  // Number of attacker spheres; must match attackerAmount in pprocedural
  // raytracing file
  static constexpr int kAttackerAmount = 13;
  static constexpr float kAttackerDistance = 400.0f;
  static constexpr float kAttackerScale = 7.5f;
  static constexpr int kAliveMaskAll = (1 << kAttackerAmount) - 1;

  int uAttackerAliveMask = kAliveMaskAll;
  vec3 uAttackerSwarmPivot = fightPos;

  // oldman's green return fire (up to 3 beams, targets given as indices)
  bool uOldmanBeamActive = false;
  vec3 uOldmanBeamStart = fightPos + kOldmanMuzzleOffset;
  ivec3 uOldmanBeamTargets = ivec3(-1);
  float uOldmanBeamRadius = 1.5f;
  vec3 uOldmanBeamColor = vec3(0.1f, 1.0f, 0.2f); // green halo
  vec3 uOldmanBeamCoreColor = vec3(0.8f, 1.0f, 0.85f);
  float uOldmanBeamGlowRadius = 1.0f;
  float uOldmanBeamGlowIntensity = 1.0f;

  vec3 oldmanWorldPos = fightPos;
  vec3 attackerSwarmPos = fightPos + vec3(800.0f, 0.0f, 0.0f);

  struct AttackRun {
    int attackerIndex;
    float beamStart;
    float killTime;
  };
  std::vector<AttackRun> attackRuns;

  static constexpr float kBeamDwell = 2.5f;   // time under fire before dying
  static constexpr float kBeamReload = 1.5f;  // gap between a slot's kills
  static constexpr float kSlotStagger = 0.8f; // offset between the 3 beams
  static constexpr int kBeamSlots = 3;
  static constexpr int kKillCount = 9; // 9 of 13 die, 4 flee
  static constexpr int kKillRounds = kKillCount / kBeamSlots;
  // The escape begins the instant the last attacker dies. Derived from the
  // constants above rather than hardcoded, so retuning the pacing can't
  // silently desync the flight phase from the final kill:
  // 0.8*2 + 2.5 + 2*4.0 = 12.1s
  static constexpr float kFlightStart =
      kSlotStagger * float(kBeamSlots - 1) + kBeamDwell +
      float(kKillRounds - 1) * (kBeamDwell + kBeamReload);
  static constexpr float kChaseDuration = 8.0f;
  static constexpr vec3 kEscapeVelocity = vec3(-155.0f, 40.0f, 155.0f);

  vec3 attackerWorldPos(int i, vec3 origin) const {
    // same PI literal as math.glsl, so CPU and GPU positions agree exactly
    const float kPi = 3.1415926f;
    float angle =
        float(i - 6) * (360.0f / float(kAttackerAmount)) * (kPi / 180.0f);
    vec3 rel = origin - uAttackerSwarmPivot;
    float c = cos(angle), s = sin(angle);
    vec3 rotated = vec3(c * rel.x - s * rel.z, rel.y, s * rel.x + c * rel.z);
    vec3 anchor = uAttackerSwarmPivot + rotated;

    const float goldenAngle = kPi * (3.0f - sqrt(5.0f));
    float y = 1.0f - (float(i) / float(kAttackerAmount - 1)) * 2.0f;
    float radiusAtY = sqrt(1.0f - y * y);
    float theta = float(i) * goldenAngle;
    return anchor + vec3(cos(theta) * radiusAtY, y, sin(theta) * radiusAtY) *
                        kAttackerDistance;
  }

  // Drives the whole "Old Man attacks" shot, called every frame while that
  // scene is active (via a SceneWindowEvent, whose onActive fires per-frame).
  // Everything here is derived from the scene-local time rather than
  // accumulated, so scrubbing or replaying can't leave stale state - except
  // the mesh translation during the chase, which is genuinely incremental
  // (see oldman.move) and is unwound in resetTimeline().
  void updateOldmanAttack(float t) {
    // --- who is still alive -------------------------------------------
    int aliveMask = kAliveMaskAll;
    for (const auto &run : attackRuns) {
      if (t >= run.killTime) {
        aliveMask &= ~(1 << run.attackerIndex);
      }
    }
    if (aliveMask != uAttackerAliveMask) {
      uAttackerAliveMask = aliveMask;
      program.set("uAttackerAliveMask", uAttackerAliveMask);
    }

    // --- oldman's beams: at most kBeamSlots at once --------------------
    bool inKillPhase = t < kFlightStart;
    ivec3 targets = ivec3(-1);
    if (inKillPhase) {
      int slot = 0;
      for (const auto &run : attackRuns) {
        if (t >= run.beamStart && t < run.killTime && slot < kBeamSlots) {
          targets[slot++] = run.attackerIndex;
        }
      }
    }
    if (targets != uOldmanBeamTargets) {
      uOldmanBeamTargets = targets;
      program.set("uOldmanBeamTargets", uOldmanBeamTargets);
    }
    // during the chase oldman stops shooting back
    bool beamsOn = inKillPhase;
    if (beamsOn != uOldmanBeamActive) {
      uOldmanBeamActive = beamsOn;
      program.set("uOldmanBeamActive", uOldmanBeamActive);
    }

    // --- the escape ----------------------------------------------------
    // Positions are recomputed from elapsed chase time (not integrated per
    // frame) so they're framerate independent and replay-safe.
    float chaseTime = max(0.0f, t - kFlightStart);
    vec3 offset = kEscapeVelocity * chaseTime;
    vec3 newOldmanPos = fightPos + offset;
    attackerSwarmPos = fightPos + vec3(800.0f, 0.0f, 0.0f) + offset;

    // oldman.move() translates by an absolute delta, so feed it the
    // difference from wherever the mesh currently sits
    if (newOldmanPos != oldmanWorldPos) {
      oldman.move(1.0f, newOldmanPos - oldmanWorldPos);
      oldman.update(program);
      oldmanWorldPos = newOldmanPos;
    }

    assetManager.spawn("oldman", oldmanWorldPos, 1.0f);
    assetManager.spawn("attacker", attackerSwarmPos, kAttackerScale);

    // survivors keep their beams locked on the fleeing oldman
    uAttackerLaserTarget = oldmanWorldPos;
    program.set("uAttackerLaserTarget", uAttackerLaserTarget);
    uOldmanBeamStart = oldmanWorldPos + kOldmanMuzzleOffset;
    program.set("uOldmanBeamStart", uOldmanBeamStart);
  }

  bool keys[(int)Key::MENU];
  float move_speed = 1.0;

  MainApp() : App(800, 600) {
    mesh.load(Mesh::FULLSCREEN_VERTICES, Mesh::FULLSCREEN_INDICES);
    load_shaders(program, "shaders", "main.vert", "main.frag");
    camera.worldPosition = vec3(300.0f, 200.0f, 300.0f);

    // register objects
    assetManager.registerObject("oldman");
    assetManager.registerObject("attacker");

    Scene linearSpace;
    linearSpace.name = "LinearSpace";
    vec3 linSpaceCamPos = fightPos + vec3(800.0f, 0.0f, 0.0f);
    linearSpace.director.holdAt(linSpaceCamPos, fightPos, 6.0f);
    linearSpace.windowEvents.push_back(
        {0.0f, 2.0f,
         [this]() { assetManager.spawn("oldman", fightPos, 1.0f); }});
    linearSpace.windowEvents.push_back({0.0f, 2.0f, [this]() {
                                          uInLinearSpace = true;
                                          program.set("uInLinearSpace",
                                                      uInLinearSpace);
                                        }});

    Scene Laser;
    Laser.name = "Laser";
    vec3 LaserCamPos = fightPos + vec3(800.0f, 0.0f, 0.0f);
    vec3 SunPos = vec3(800.0f, 0.0f, 0.0f);
    Laser.director.moveCameraTo(LaserCamPos + vec3(8.0f, 60.0f, 0.0f), fightPos,
                                100.0);
    Laser.director.moveCameraTo(LaserCamPos + vec3(8.0f, 60.0f, 60.0f), SunPos,
                                40.0);

    Laser.windowEvents.push_back(
        {0.0f, 1.0f,
         [this]() {
           uInLinearSpace = true;
           program.set("uInLinearSpace", uInLinearSpace);
         },
         [this]() {
           uInLinearSpace = false;
           program.set("uInLinearSpace", uInLinearSpace);
         }});

    Laser.windowEvents.push_back({5.0f, 12.0f, [this]() {
                                    uLaserActive = true;
                                    program.set("uLaserActive", uLaserActive);
                                  }});

    Scene kampf;
    kampf.name = "Kampf";

    // Camera path: oldman intro -> swing past the attacker swarm -> pull
    // back for the final fight shot.
    kampf.director.moveCameraTo(fightPos + vec3(700.0f, 300.0f, 0.0f), fightPos,
                                114.0f);
    vec3 attackerPos = fightPos + vec3(800.0f, 0.0f, 0.0f);
    kampf.director.moveCameraTo(attackerPos + vec3(100.0f, 50.0f, -100.0f),
                                attackerPos, 56.0f);
    kampf.director.moveCameraTo(fightPos + vec3(1200.0f, 500.0f, 1200.0f),
                                fightPos, 201.0f);
    // Holding so the scene is long enough
    kampf.director.holdAt(fightPos + vec3(1200.0f, 500.0f, 1200.0f), fightPos,
                          10.0f);

    kampf.windowEvents.push_back(
        {0.0f, 25.0f,
         [this]() { assetManager.spawn("oldman", fightPos, 1.0f); }});
    kampf.windowEvents.push_back({0.0f, 25.0f, [this]() {
                                    uLaserActive = true;
                                    program.set("uLaserActive", uLaserActive);
                                  }});

    // attacker swarm shows up partway through (6s+)
    kampf.windowEvents.push_back({6.0f, 25.0f, [this, attackerPos]() {
                                    assetManager.spawn("attacker", attackerPos,
                                                       7.5f);
                                  }});

    // one-shot visual explosion, paired with the SFX scheduled below at the
    // same kAttackExplosionTime
    kampf.oneShotEvents.push_back({kAttackExplosionTime, [this, attackerPos]() {
                                     explosions.spawn(attackerPos, 10.0);
                                   }});

    // attacker swarm opens fire on oldman at uLaserFireEnd, alongside the
    // still-firing blue laser, and (like everything else above) just stays
    // on rather than auto-deactivating
    kampf.windowEvents.push_back({uLaserFireEnd, 25.0f, [this]() {
                                    uAttackerLaserActive = true;
                                    program.set("uAttackerLaserActive",
                                                uAttackerLaserActive);
                                  }});

    Scene POV;
    POV.name = "POV";
    {
      // so the POV attacker isnt moved and we are suddenly just in space
      const int povAttackerIndex = 6;
      vec3 povAttackerPos = attackerWorldPos(povAttackerIndex, attackerPos);

      // Classic over-the-shoulder framing
      vec3 forward = normalize(fightPos - povAttackerPos); // attacker -> oldman
      vec3 left = normalize(cross(vec3(0.0f, 1.0f, 0.0f), forward));
      vec3 up = cross(forward, left);

      float behindDistance =
          60.0f; // how far behind the attacker, away from oldman
      float leftDistance =
          15.0f; // slight sideways offset so the attacker isn't dead-center
      float upDistance = 10.0f; // slight upward offset, for a nicer angle

      vec3 povCamPos = povAttackerPos - forward * behindDistance +
                       left * leftDistance + up * upDistance;
      POV.director.holdAt(povCamPos, fightPos, 7.0f);
    }

    for (int round = 0; round < kKillRounds; round++) {
      for (int slot = 0; slot < kBeamSlots; slot++) {
        int index = round * kBeamSlots + slot;
        float killTime = kSlotStagger * float(slot) + kBeamDwell +
                         float(round) * (kBeamDwell + kBeamReload);
        attackRuns.push_back({index, killTime - kBeamDwell, killTime});
      }
    }

    Scene old_man_attacks;
    old_man_attacks.name = "Old Man attacks";

    // Camera: swing back out to a wide vantage for the duel, hold through the
    // kill phase, then trail the fleeing oldman.
    vec3 duelCamPos = fightPos + vec3(1200.0f, 500.0f, 1200.0f);
    old_man_attacks.director.moveCameraTo(duelCamPos, fightPos, 400.0f);
    float flyInEnd = old_man_attacks.director.currentTimelineEnd;
    old_man_attacks.director.holdAt(duelCamPos, fightPos,
                                    max(0.0f, kFlightStart - flyInEnd));

    // Endpoint of the chase is known up-front because the escape is a
    // constant velocity starting at a fixed time.
    vec3 escapeEnd = fightPos + kEscapeVelocity * kChaseDuration;
    vec3 escapeDir = normalize(kEscapeVelocity);
    vec3 chaseCamPos =
        escapeEnd - escapeDir * 1400.0f + vec3(0.0f, 450.0f, 0.0f);
    old_man_attacks.director.moveCameraTo(
        chaseCamPos, escapeEnd,
        glm::distance(duelCamPos, chaseCamPos) / kChaseDuration);

    float attackDuration = old_man_attacks.director.currentTimelineEnd;

    // the big blue laser is done - oldman switches to the green beams
    old_man_attacks.windowEvents.push_back({0.0f, attackDuration, [this]() {
                                              uLaserActive = false;
                                              program.set("uLaserActive",
                                                          uLaserActive);
                                            }});

    // survivors keep firing on oldman for the whole shot
    old_man_attacks.windowEvents.push_back(
        {0.0f, attackDuration, [this]() {
           uAttackerLaserActive = true;
           program.set("uAttackerLaserActive", uAttackerLaserActive);
         }});

    // per-frame driver: kills, beam targeting and the escape
    old_man_attacks.windowEvents.push_back(
        {0.0f, attackDuration,
         [this]() { updateOldmanAttack(currentSceneTime); }});

    // one explosion + SFX per kill, fired the frame that attacker dies
    for (const auto &run : attackRuns) {
      int index = run.attackerIndex;
      old_man_attacks.oneShotEvents.push_back(
          {run.killTime, [this, index]() {
             explosions.spawn(attackerWorldPos(index, attackerSwarmPos), 10.0);
             audio.playSFX("src/audio/explosion.wav");
           }});
    }

    Scene supernova;
    supernova.name = "Supernova";

    vec3 sunPos = vec3(1000.0f, 500.0f, 1000.0f);
    vec3 supernovaCamPos = fightPos + vec3(1200.0f, 500.0f, 1200.0f);

    supernova.director.holdAt(supernovaCamPos, sunPos, 8.0f);

    supernova.windowEvents.push_back(
        {0.0f, 8.0f,
         [this]() {
           uSunSize = glm::max(uSunSize - App::delta * 150.0f, 0.0f);
           program.set("uSunSize", uSunSize);
         },
         [this]() {
           uSunSize = 1000.0f;
           program.set("uSunSize", uSunSize);
         }});

    scenes.push_back(linearSpace);
    scenes.push_back(Laser);
    // recorded so the explosion SFX below can be offset onto the global clock
    // by this scene's start, independent of how many scenes follow it
    const int kampfSceneIndex = (int)scenes.size();
    scenes.push_back(kampf);
    scenes.push_back(POV);
    scenes.push_back(old_man_attacks);
    // scenes.push_back(linearSpace); TODO: uncomment
    scenes.push_back(supernova);

    // Audio: an underlying score loops for the whole cinematic, and sound
    // effects are triggered off the same global uFlightTime clock that
    // drives the camera/spawn timeline. kAttackExplosionTime is local to
    // "Kampf" (matches its oneShotEvent above), so it's offset by
    // sceneStartTime() here to land on the same global instant even if
    // other scenes end up chained in front of Kampf later.
    audio.init();
    audio.playMusic("src/audio/Soundtrack.wav", 0.5f);
    audio.scheduleSFX(sceneStartTime(kampfSceneIndex) + kAttackExplosionTime,
                      "src/audio/explosion.wav");

    // Init uniforms
    program.set("uLightColor", vec3(1.0));
    program.set("uLightDir", uLightDir);
    program.set("uNear", uNear);
    program.set("uFar", uFar);
    program.set("uSteps", uSteps);
    program.set("uEpsilon", uEpsilon);
    program.set("uNormalEps", uNormalEps);
    program.set("uWarp", uWarp);
    program.set("uScan", uScan);
    program.set("uResolution", resolution);
    program.set("uInLinearSpace", uInLinearSpace);
    program.set("uFocusDistance", uFocusDistance);
    program.set("uApertureSize", uApertureSize);
    program.set("uFocusSamples", uFocusSamples);
    program.set("uLaserActive", uLaserActive);
    program.set("uLaserStart", uLaserStart);
    program.set("uLaserEnd", uLaserStart + normalize(uLightDir) * kLaserLength);
    program.set("uLaserRadius", uLaserRadius);
    program.set("uLaserColor", uLaserColor);
    program.set("uLaserCoreColor", uLaserCoreColor);
    program.set("uLaserGlowRadius", uLaserGlowRadius);
    program.set("uLaserGlowIntensity", uLaserGlowIntensity);
    program.set("uAttackerLaserActive", uAttackerLaserActive);
    program.set("uAttackerLaserTarget", uAttackerLaserTarget);
    program.set("uAttackerLaserRadius", uAttackerLaserRadius);
    program.set("uAttackerLaserColor", uAttackerLaserColor);
    program.set("uAttackerLaserCoreColor", uAttackerLaserCoreColor);
    program.set("uAttackerLaserGlowRadius", uAttackerLaserGlowRadius);
    program.set("uAttackerLaserGlowIntensity", uAttackerLaserGlowIntensity);
    program.set("uAttackerAliveMask", uAttackerAliveMask);
    program.set("uAttackerSwarmPivot", uAttackerSwarmPivot);
    program.set("uOldmanBeamActive", uOldmanBeamActive);
    program.set("uOldmanBeamStart", uOldmanBeamStart);
    program.set("uOldmanBeamTargets", uOldmanBeamTargets);
    program.set("uOldmanBeamRadius", uOldmanBeamRadius);
    program.set("uOldmanBeamColor", uOldmanBeamColor);
    program.set("uOldmanBeamCoreColor", uOldmanBeamCoreColor);
    program.set("uOldmanBeamGlowRadius", uOldmanBeamGlowRadius);
    program.set("uOldmanBeamGlowIntensity", uOldmanBeamGlowIntensity);
    program.set("uSunSize", uSunSize);
    program.use();

    explosions.init();

    SNMesh unit_sphere, unit_cube;
    unit_sphere.fromObj("meshes/lowpolysphere.obj");
    unit_cube.fromObj("meshes/cube.obj");
    oldman.init(unit_sphere, unit_cube, program, 0);
    // The CSG mesh is built centered at the world origin (see oldman.cpp)
    // but every Scene positions the "oldman" AssetManager object at
    // fightPos and points its cameras/laser there, so the actual mesh
    // geometry must be translated to match or it's nowhere near what's
    // being filmed.
    oldman.move(1.0f, fightPos);
    oldman.update(program);
  }

  ~MainApp() override { audio.shutdown(); }

  void render() override {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program.use();

    // autocam
    if (uAutoCam == true) {
      uFlightTime += App::delta; // global clock, shared across all scenes

      // find which scene the global clock currently falls inside, and how
      // far into that scene we are (its own events/keyframes are all
      // defined in scene-local time, starting at 0)
      float elapsed = 0.0f;
      int activeScene = (int)scenes.size() - 1;
      for (size_t i = 0; i < scenes.size(); i++) {
        if (uFlightTime < elapsed + scenes[i].duration()) {
          activeScene = (int)i;
          break;
        }
        elapsed += scenes[i].duration();
      }

      if (activeScene != currentScene) {
        // force the outgoing scene through its own end-of-window once, so
        // its onInactive callbacks (despawns etc.) fire before we stop
        // driving it and move on
        scenes[currentScene].update(scenes[currentScene].duration());
        currentScene = activeScene;
      }

      float localTime = uFlightTime - elapsed;
      currentSceneTime = localTime;
      Scene &scene = scenes[currentScene];
      // drives every spawn/despawn/laser/explosion trigger for this scene
      scene.update(localTime);
      audio.update(uFlightTime);
      // gets the smooth camera curve
      camera.worldPosition = CinematicSpline::getInterpolatedPosition(
          scene.director.timelineKeyframes, localTime, false);
      camera.target = CinematicSpline::getInterpolatedPosition(
          scene.director.timelineKeyframes, localTime, true);
      camera.invalidate();

      // end: stop once every chained scene has played, and rewind so the
      // next "Automatic Camera" run starts clean from scene 0 again
      if (uFlightTime > sceneStartTime((int)scenes.size())) {
        uAutoCam = false;
        resetTimeline();
      }
    }

    if (uAutoCam == false) {
      // move camera
      float actual_move_speed = move_speed;
      if (keys[(int)Key::LEFT_SHIFT]) {
        actual_move_speed *= 10;
      }
      if (keys[(int)Key::W]) {
        camera.moveInEyeSpace(vec3(0.0, 0.0, -actual_move_speed));
      }
      if (keys[(int)Key::A]) {
        camera.moveInEyeSpace(vec3(-actual_move_speed, 0.0, 0.0));
      }
      if (keys[(int)Key::S]) {
        camera.moveInEyeSpace(vec3(0.0, 0.0, actual_move_speed));
      }
      if (keys[(int)Key::D]) {
        camera.moveInEyeSpace(vec3(actual_move_speed, 0.0, 0.0));
      }
      if (keys[(int)Key::SPACE]) {
        vec3 camDelta = vec3(0.0, actual_move_speed, 0.0);
        camera.worldPosition += camDelta;
        camera.target += camDelta;
        camera.invalidate();
      }
      if (keys[(int)Key::LEFT_CONTROL] || keys[(int)Key::C]) {
        vec3 camDelta = vec3(0.0, -actual_move_speed, 0.0);
        camera.worldPosition += camDelta;
        camera.target += camDelta;
        camera.invalidate();
      }
    }
    program.set("uTime", time);
    // update objects for shader
    assetManager.updateShaderUniforms(program);

    // laser always points from oldman towards the current light (sun)
    // direction, so it stays aimed correctly if uLightDir changes at runtime
    program.set("uLaserEnd", uLaserStart + normalize(uLightDir) * kLaserLength);

    // Update camera information on change
    bool camera_changed = camera.updateIfChanged();
    if (camera_changed) {
      program.set("uCameraMatrix", camera.cameraMatrix);
      program.set("uAspectRatio", camera.aspectRatio);
      program.set("uFocalLength", camera.focalLength);
      program.set("uCameraPosition", camera.worldPosition);
      // change focus distanz to current position
      uFocusDistance =
          max(100.0f, glm::distance(camera.worldPosition, camera.target));
      program.set("uFocusDistance", uFocusDistance);
    }

    // We already bind the program in the constructor, so we don't need to bind
    // it again here
    mesh.draw();

    explosions.render(camera, App::delta, camera_changed);
    // focus blur only during explosion
    if (explosions.isActive()) {
      uApertureSize = 15.0f;
      uFocusSamples = 4;
    } else {
      // goes down slowly instead of instantly
      // glm::mix(a, b, c) = a + (b - a) * c
      uApertureSize = glm::mix(uApertureSize, 0.0f, 0.20f);
      uFocusSamples = uApertureSize < 0.1f ? 1 : 4;
    }
    // after every frame
    program.set("uApertureSize", uApertureSize);
    program.set("uFocusSamples", uFocusSamples);
  }

  void buildImGui() override {
    ImGui::StatisticsWindow(App::delta, App::resolution);

    // UI to control the uniforms
    ImGui::Begin("Settings", nullptr, ImGuiChildFlags_AlwaysAutoResize);
    if (ImGui::SphericalSlider("Light Dir", uLightDir))
      program.set("uLightDir", uLightDir);
    if (ImGui::SliderFloat("Near Plane", &uNear, 0.0, 10.0, "%.1f"))
      program.set("uNear", uNear);
    if (ImGui::SliderFloat("Far Plane", &uFar, 0.0, 10'000'000.0, "%.1f",
                           ImGuiSliderFlags_Logarithmic))
      program.set("uFar", uFar);
    if (ImGui::SliderInt("Max Steps", &uSteps, 0, 2000))
      program.set("uSteps", uSteps);
    if (ImGui::SliderFloat("Epsilon", &uEpsilon, 0.0, 1.0, "%.6f",
                           ImGuiSliderFlags_Logarithmic))
      program.set("uEpsilon", uEpsilon);
    if (ImGui::SliderFloat("Normal Epsilon", &uNormalEps, 0.0, 1.0, "%.6f",
                           ImGuiSliderFlags_Logarithmic))
      program.set("uNormalEps", uNormalEps);

    if (ImGui::SliderFloat("Camera move speed", &move_speed, 0.1, 100.0, "%.6f",
                           ImGuiSliderFlags_Logarithmic))
      program.set("uNormalEps", uNormalEps);

    if (ImGui::Checkbox("In linear space", &uInLinearSpace))
      program.set("uInLinearSpace", uInLinearSpace);

    if (ImGui::Checkbox("Automatic Camera", &uAutoCam)) {
      resetTimeline();
    }
    ImGui::Text("Shot: %s (t=%.1fs)", scenes[currentScene].name.c_str(),
                uFlightTime - sceneStartTime(currentScene));

    if (ImGui::SliderFloat("CRT Warp", &uWarp, 0.0f, 4.0f))
      program.set("uWarp", uWarp);
    if (ImGui::SliderFloat("CRT Scan", &uScan, 0.0f, 4.0f))
      program.set("uScan", uScan);
    if (ImGui::SliderFloat("Focus Distance", &uFocusDistance, 1.0f, 2000.0f,
                           "%.1f"))
      program.set("uFocusDistance", uFocusDistance);
    if (ImGui::SliderFloat("Aperture Size", &uApertureSize, 0.0f, 50.0f,
                           "%.3f"))
      program.set("uApertureSize", uApertureSize);
    if (ImGui::SliderInt("Focus Samples", &uFocusSamples, 1, 8))
      program.set("uFocusSamples", uFocusSamples);

    ImGui::SeparatorText("Big Blue Laser");
    if (ImGui::Checkbox("Laser Active", &uLaserActive))
      program.set("uLaserActive", uLaserActive);
    if (ImGui::SliderFloat("Laser Radius", &uLaserRadius, 0.1f, 100.0f, "%.2f",
                           ImGuiSliderFlags_Logarithmic))
      program.set("uLaserRadius", uLaserRadius);
    if (ImGui::ColorEdit3("Laser Glow Color", value_ptr(uLaserColor)))
      program.set("uLaserColor", uLaserColor);
    if (ImGui::ColorEdit3("Laser Core Color", value_ptr(uLaserCoreColor)))
      program.set("uLaserCoreColor", uLaserCoreColor);
    if (ImGui::SliderFloat("Laser Glow Radius", &uLaserGlowRadius, 0.5f, 200.0f,
                           "%.2f", ImGuiSliderFlags_Logarithmic))
      program.set("uLaserGlowRadius", uLaserGlowRadius);
    if (ImGui::SliderFloat("Laser Glow Intensity", &uLaserGlowIntensity, 0.0f,
                           4.0f))
      program.set("uLaserGlowIntensity", uLaserGlowIntensity);
    ImGui::SliderFloat("Laser Fire Start (s)", &uLaserFireStart, 0.0f, 400.0f);
    ImGui::SliderFloat("Laser Fire End (s)", &uLaserFireEnd, 0.0f, 400.0f);
    ImGui::SeparatorText("Attacker Lasers");
    if (ImGui::Checkbox("Attacker Lasers Active", &uAttackerLaserActive))
      program.set("uAttackerLaserActive", uAttackerLaserActive);
    if (ImGui::SliderFloat("Attacker Laser Radius", &uAttackerLaserRadius, 0.1f,
                           100.0f, "%.2f", ImGuiSliderFlags_Logarithmic))
      program.set("uAttackerLaserRadius", uAttackerLaserRadius);
    if (ImGui::ColorEdit3("Attacker Laser Glow Color",
                          value_ptr(uAttackerLaserColor)))
      program.set("uAttackerLaserColor", uAttackerLaserColor);
    if (ImGui::ColorEdit3("Attacker Laser Core Color",
                          value_ptr(uAttackerLaserCoreColor)))
      program.set("uAttackerLaserCoreColor", uAttackerLaserCoreColor);
    if (ImGui::SliderFloat("Attacker Laser Glow Radius",
                           &uAttackerLaserGlowRadius, 0.5f, 200.0f, "%.2f",
                           ImGuiSliderFlags_Logarithmic))
      program.set("uAttackerLaserGlowRadius", uAttackerLaserGlowRadius);
    if (ImGui::SliderFloat("Attacker Laser Glow Intensity",
                           &uAttackerLaserGlowIntensity, 0.0f, 4.0f))
      program.set("uAttackerLaserGlowIntensity", uAttackerLaserGlowIntensity);
    ImGui::SeparatorText("Oldman Beams");
    if (ImGui::Checkbox("Oldman Beams Active", &uOldmanBeamActive))
      program.set("uOldmanBeamActive", uOldmanBeamActive);
    if (ImGui::SliderFloat("Oldman Beam Radius", &uOldmanBeamRadius, 0.1f,
                           100.0f, "%.2f", ImGuiSliderFlags_Logarithmic))
      program.set("uOldmanBeamRadius", uOldmanBeamRadius);
    if (ImGui::ColorEdit3("Oldman Beam Glow Color",
                          value_ptr(uOldmanBeamColor)))
      program.set("uOldmanBeamColor", uOldmanBeamColor);
    if (ImGui::ColorEdit3("Oldman Beam Core Color",
                          value_ptr(uOldmanBeamCoreColor)))
      program.set("uOldmanBeamCoreColor", uOldmanBeamCoreColor);
    if (ImGui::SliderFloat("Oldman Beam Glow Radius", &uOldmanBeamGlowRadius,
                           0.5f, 200.0f, "%.2f", ImGuiSliderFlags_Logarithmic))
      program.set("uOldmanBeamGlowRadius", uOldmanBeamGlowRadius);
    if (ImGui::SliderFloat("Oldman Beam Glow Intensity",
                           &uOldmanBeamGlowIntensity, 0.0f, 4.0f))
      program.set("uOldmanBeamGlowIntensity", uOldmanBeamGlowIntensity);
    int aliveCount = 0;
    for (int i = 0; i < kAttackerAmount; i++)
      if (uAttackerAliveMask & (1 << i))
        aliveCount++;
    ImGui::Text("Attackers alive: %d / %d", aliveCount, kAttackerAmount);

    ImGui::SeparatorText("Audio");
    if (!audio.isAvailable()) {
      ImGui::TextColored(ImVec4(1, 0, 0, 1), "Audio unavailable");
    } else {
      static float musicVolume = 0.5f;
      if (ImGui::SliderFloat("Music Volume", &musicVolume, 0.0f, 1.0f))
        audio.setMusicVolume(musicVolume);
      if (ImGui::Button("Play Explosion SFX"))
        audio.playSFX("src/audio/explosion.wav");
    }
    ImGui::End();
  }

  void keyCallback(Key key, Action action, Modifier modifier) override {
    float speed = 10000.0;

    if (key == Key::ESC && action == Action::PRESS)
      App::close();
    else if (key == Key::COMMA && action == Action::PRESS)
      App::imguiEnabled = !App::imguiEnabled;

    // update keys bit map
    if (action == Action::PRESS) {
      keys[(int)key] = true;
    } else if (action == Action::RELEASE) {
      keys[(int)key] = false;
    }

    if (action == Action::PRESS) {
      explosions.spawn(camera.worldPosition, 10.0);
      audio.playSFX("src/audio/explosion.wav");
    }
  }

  void moveCallback(const vec2 &movement, bool leftButton, bool rightButton,
                    bool middleButton) override {
    if (rightButton | middleButton)
      camera.orbit(movement * 0.01f);
  }

  void resizeCallback(const vec2 &resolution) override {
    camera.resize(resolution.x / resolution.y);
    program.set("uResolution", resolution);
  }
};

}; // namespace sn

int main() {
#ifndef NDEBUG
  // cd to parent directory when in debug mode to find resources
  std::filesystem::current_path(
      std::filesystem::path(__FILE__).parent_path().parent_path());
#endif
  sn::MainApp app;
  app.run();
}
