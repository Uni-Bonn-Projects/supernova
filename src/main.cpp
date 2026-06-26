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

struct MainApp : public App {
  Program program;
  Mesh mesh;
  Camera camera;
  vec3 uLightDir = normalize(vec3(1.0));
  vec3 uLightColor = vec3(1.0);
  vec3 uSkyColor = vec3(0.1, 0.3, 0.6);
  float uAmbient = 0.3;
  float uNear = 0.1;
  float uFar = 1'000'000'000.0;
  int uSteps = 1000;
  float uEpsilon = 0.0001;
  float uNormalEps = 0.0001;

  bool keys[(int)Key::MENU]; // "bit map" for all keys
  float move_speed = 1000.0;

  MainApp() : App(800, 600) {
    mesh.load(Mesh::FULLSCREEN_VERTICES, Mesh::FULLSCREEN_INDICES);
    program.load("shaders/raygen.vert", "shaders/raymarch.frag");
    camera.worldPosition = vec3(5.0f, 3.0f, 5.0f);

    // Init uniforms
    program.set("uLightDir", uLightDir);
    program.set("uLightColor", uLightColor);
    program.set("uSkyColor", uSkyColor);
    program.set("uAmbient", uAmbient);
    program.set("uNear", uNear);
    program.set("uFar", uFar);
    program.set("uSteps", uSteps);
    program.set("uEpsilon", uEpsilon);
    program.set("uNormalEps", uNormalEps);
    program.use();
  }

  void render() override {
    // move camera
    float actual_move_speed = move_speed;
    if (keys[(int)Key::LEFT_SHIFT]) {
      actual_move_speed *= 10;
    }
    if (keys[(int)Key::W]) {
      camera.moveInEyeSpace(vec3(0.0, 0.0, -actual_move_speed));
    } else if (keys[(int)Key::A]) {
      camera.moveInEyeSpace(vec3(-actual_move_speed, 0.0, 0.0));
    } else if (keys[(int)Key::S]) {
      camera.moveInEyeSpace(vec3(0.0, 0.0, actual_move_speed));
    } else if (keys[(int)Key::D]) {
      camera.moveInEyeSpace(vec3(actual_move_speed, 0.0, 0.0));
    } else if (keys[(int)Key::SPACE]) {
      vec3 camDelta = vec3(0.0, actual_move_speed, 0.0);
      camera.worldPosition += camDelta;
      camera.target += camDelta;
      camera.invalidate();
    } else if (keys[(int)Key::LEFT_CONTROL]) {
      vec3 camDelta = vec3(0.0, -actual_move_speed, 0.0);
      camera.worldPosition += camDelta;
      camera.target += camDelta;
      camera.invalidate();
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
    if (ImGui::ColorEdit3("Light Color", value_ptr(uLightColor),
                          ImGuiColorEditFlags_Float))
      program.set("uLightColor", uLightColor);
    if (ImGui::ColorEdit3("Sky Color", value_ptr(uSkyColor),
                          ImGuiColorEditFlags_Float))
      program.set("uSkyColor", uSkyColor);
    if (ImGui::SliderFloat("Ambient", &uAmbient, 0.0, 1.0, "%.2f"))
      program.set("uAmbient", uAmbient);
    if (ImGui::SliderFloat("Near Plane", &uNear, 0.0, 10.0, "%.1f"))
      program.set("uNear", uNear);
    if (ImGui::SliderFloat("Far Plane", &uFar, 0.0, 1'000'000'000'000.0, "%.1f",
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

    if (ImGui::SliderFloat("Camera move speed", &move_speed, 100.0, 10000.0,
                           "%.6f", ImGuiSliderFlags_Logarithmic))
      program.set("uNormalEps", uNormalEps);
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

  void scrollCallback(float x, float y) override { camera.zoom(y); }

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
