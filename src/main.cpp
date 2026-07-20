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
      const int povAttackerIndex = 6;
      const float attackerDistance = 400.0f; // must match attacker_distance_val
      const float goldenAngle = 3.1415926f * (3.0f - sqrt(5.0f));
      float y = 1.0f - (float(povAttackerIndex) / float(13 - 1)) * 2.0f;
      float radiusAtY = sqrt(1.0f - y * y);
      float theta = float(povAttackerIndex) * goldenAngle;
      float x = cos(theta) * radiusAtY;
      float z = sin(theta) * radiusAtY;
      vec3 povAttackerPos = attackerPos + vec3(x, y, z) * attackerDistance;

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
      POV.director.holdAt(povCamPos, fightPos, 8.0f);
    }

    // Scene old_man_attacks;
    // old_man-attacks.name = "Old Man attacks"
    // ToDO: old man shoots attacker
    // ToDo: old man flies away

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
    scenes.push_back(kampf);
    scenes.push_back(POV);
    // scenes.push_back(old_man_attacks);
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
    audio.scheduleSFX(sceneStartTime((int)scenes.size() - 1) +
                          kAttackExplosionTime,
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
      uApertureSize = glm::mix(uApertureSize, 0.0f, 0.05f);
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
