#include <filesystem>
#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

#include <imgui.h>

#include <cinematic.cpp>
#include <framework/app.hpp>
#include <framework/camera.hpp>
#include <framework/gl/program.hpp>
#include <framework/imguiutil.hpp>
#include <framework/mesh.hpp>

#include <string>
#include <unordered_map>
#include <vector>

#include "multifile_shaders.h"

struct MainApp : public App {
  Program program;
  Mesh mesh;
  Camera camera;
  AssetManager assetManager;
  CinematicDirector director;
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
  float uApertureSize = 5.0f;
  int uFocusSamples = 4;

  bool keys[(int)Key::MENU]; // "bit map" for all keys
  float move_speed = 1.0;

  MainApp() : App(800, 600) {
    mesh.load(Mesh::FULLSCREEN_VERTICES, Mesh::FULLSCREEN_INDICES);
    load_shaders(program, "shaders", "main.vert", "main.frag");
    camera.worldPosition = vec3(5.0f, 3.0f, 5.0f);

    // register objects
    assetManager.registerObject("oldman");
    assetManager.registerObject("attacker");

    // camera movement
    director.moveCameraTo(fightPos + vec3(700.0f, 300.0f, 0.0f), fightPos,
                          114.0f);
    vec3 attackerPos = fightPos + vec3(800.0f, 0.0f, 0.0f);
    director.moveCameraTo(attackerPos + vec3(100.0f, 50.0f, -100.0f),
                          attackerPos, 56.0f);
    director.moveCameraTo(fightPos + vec3(1200.0f, 500.0f, 1200.0f), fightPos,
                          201.0f);

    // Init uniforms
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
    program.use();
  }

  void render() override {
    // autocam
    if (uAutoCam == true) {
      uFlightTime += App::delta;
      // spawn and despawn
      if (uFlightTime >= 0.0f && uFlightTime < 25.0f) {
        assetManager.spawn("oldman", fightPos, 1.0f);
      } else {
        assetManager.despawn("oldman");
      }

      if (uFlightTime >= 6.0f && uFlightTime < 25.0f) {
        assetManager.spawn("attacker", fightPos + vec3(800.0f, 0.0f, 0.0f),
                           2.5f);
      } else {
        assetManager.despawn("attacker");
      }
      // gets the smooth camera curve
      camera.worldPosition = CinematicSpline::getInterpolatedPosition(
          director.timelineKeyframes, uFlightTime, false);
      camera.target = CinematicSpline::getInterpolatedPosition(
          director.timelineKeyframes, uFlightTime, true);
      camera.invalidate();

      // end
      if (uFlightTime > director.currentTimelineEnd) {
        uAutoCam = false;
        uFlightTime = 0.0f;
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

    // Update camera information on change
    if (camera.updateIfChanged()) {
      program.set("uCameraMatrix", camera.cameraMatrix);
      program.set("uAspectRatio", camera.aspectRatio);
      program.set("uFocalLength", camera.focalLength);
      program.set("uCameraPosition", camera.worldPosition);
    }

    // We already bind the program in the constructor, so we don't need to bind
    // it again here
    mesh.draw();
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

    if (ImGui::Checkbox("Automatic Camera", &uAutoCam))
      uFlightTime = 0.0f;
    ImGui::Text("Shot: %s", uFlightTime < 6.0f    ? "1 - Old Man"
                            : uFlightTime < 12.0f ? "2 - Attacker"
                            : uFlightTime < 25.0f ? "3 - Kampf"
                                                  : "---");

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

int main() {
#ifndef NDEBUG
  // cd to parent directory when in debug mode to find resources
  std::filesystem::current_path(
      std::filesystem::path(__FILE__).parent_path().parent_path());
#endif
  MainApp app;
  app.run();
}
