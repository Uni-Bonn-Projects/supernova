#include <filesystem>
#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

#include <imgui.h>

#include "audio/miniaudio.h"
#include <framework/app.hpp>
#include <framework/camera.hpp>
#include <framework/gl/program.hpp>
#include <framework/imguiutil.hpp>
#include <framework/mesh.hpp>
#include <iostream>

struct MainApp : public App {
  Program program;
  Mesh mesh;
  Camera camera;
  ma_engine audioEngine;
  bool audioAvailable = false;
  vec3 uLightDir = normalize(vec3(1.0));
  float uNear = 0.1;
  float uFar = 10'000.0;
  int uSteps = 1000;
  float uEpsilon = 0.0001;
  float uNormalEps = 0.0001;
  bool uAutoCam = false;
  float uFlightTime = 0.0f;

  bool keys[(int)Key::MENU]; // "bit map" for all keys
  float move_speed = 1.0;

  MainApp() : App(800, 600) {
    mesh.load(Mesh::FULLSCREEN_VERTICES, Mesh::FULLSCREEN_INDICES);
    program.load("shaders/raygen.vert", "shaders/raymarch.frag");
    camera.worldPosition = vec3(5.0f, 3.0f, 5.0f);

    // Init uniforms
    program.set("uLightDir", uLightDir);
    program.set("uNear", uNear);
    program.set("uFar", uFar);
    program.set("uSteps", uSteps);
    program.set("uEpsilon", uEpsilon);
    program.set("uNormalEps", uNormalEps);
    program.use();

    auto result = ma_engine_init(NULL, &audioEngine);
    audioAvailable = result == MA_SUCCESS;
    if (!audioAvailable) {
      std::cerr << "Audio engine init failed: " << result << "\n";
    } else {
      result = ma_engine_play_sound(&audioEngine, "src/audio/Geist.wav", NULL);
      audioAvailable = result == MA_SUCCESS;
      if (!audioAvailable) {
        std::cerr << "Failed to play audio: " << result << "\n";
      }
    }
  }

  ~MainApp() {
    if (audioAvailable) {
      ma_engine_uninit(&audioEngine);
    }
  }

  void render() override {
    // automatic camera for movie
    //  TODO FLO: richtige kamera positionen und eventuel Winkel und bisschen
    //  kamerafahrt
    if (uAutoCam == true) {
      // timer wie lange clip geht
      uFlightTime += delta;

      // old man folgen (Platzhalter)
      if (uFlightTime < 6.0f) {
        camera.worldPosition = vec3(0.0f, 150.0f, 500.0f);
        camera.target = vec3(0.0f);
        camera.invalidate();

        // panning shot auf attacker (Platzhalter)
      } else if (uFlightTime < 12.0f) {
        camera.worldPosition = vec3(500.0f, 50.0f, 0.0f);
        camera.target = vec3(0.0f);
        camera.invalidate();

        // kampft old man und attacker (Platzhalter)
      } else if (uFlightTime < 19.0f) {
        camera.worldPosition = vec3(300.0f, 200.0f, 300.0f);
        camera.target = vec3(0.0f);
        camera.invalidate();

        // ende
      } else {
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

    if (ImGui::Checkbox("Automatic Camera", &uAutoCam))
      uFlightTime = 0.0f;
    ImGui::Text("Shot: %s", uFlightTime < 6.0f    ? "1 - Old Man"
                            : uFlightTime < 12.0f ? "2 - Attacker"
                            : uFlightTime < 19.0f ? "3 - Kampf"
                                                  : "---");
    if (!audioAvailable) {
      ImGui::TextColored(ImVec4(1, 0, 0, 1), "Audio unavailable");
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
  }

  void moveCallback(const vec2 &movement, bool leftButton, bool rightButton,
                    bool middleButton) override {
    if (rightButton | middleButton)
      camera.orbit(movement * 0.01f);
  }

  void resizeCallback(const vec2 &resolution) override {
    camera.resize(resolution.x / resolution.y);
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
