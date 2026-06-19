#include <string>
#include <vector>

#include <glad/gl.h>

#include <framework/app.hpp>
#include <framework/gl/program.hpp>
#include <framework/mesh.hpp>

const std::vector VERTICES{-0.5f, -0.5f, 0.0f, 0.5f,  -0.5f, 0.0f,
                           0.5f,  0.5f,  0.0f, -0.5f, 0.5f,  0.0f};

const std::vector<unsigned int> INDICES{
    // down right
    0u,
    1u,
    2u,

    // up left
    0u,
    2u,
    3u,
};

const std::string vertexShaderSource = R"(
    #version 410 core

    layout(location = 0) in vec3 aPos;

    void main() {
        gl_Position = vec4(aPos, 1.0);
    }
)";

const std::string fragmentShaderSource = R"(
    #version 410 core

    out vec4 fragColor;

    void main() {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

struct MainApp : App {
  Program program;
  Mesh mesh;

  MainApp() : App(600, 500) {
    program.loadSource(vertexShaderSource, fragmentShaderSource);

    mesh.load(VERTICES, INDICES);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // Set clear color

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Uncomment for wireframe
    // mode
  }

  void render() override {
    glClear(GL_COLOR_BUFFER_BIT); // Clear the screen
    program.use();
    mesh.draw();
  }
};

int main() {
  MainApp app;
  app.run();
  return 0;
}
