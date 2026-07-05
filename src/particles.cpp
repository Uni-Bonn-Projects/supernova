#include "framework/app.hpp"
#include "framework/camera.hpp"
#include "framework/gl/program.hpp"
#include "framework/imguiutil.hpp"
#include "framework/mesh.hpp"
#include "glm/fwd.hpp"
#include <cassert>
#include <cstdlib>
#include <vector>

using namespace glm;

struct Particles {
  static const unsigned int AMOUNT = 100'000;
  const float _INIT_LIFE = 5.0f; // initial time a particle lives
  unsigned int _last_unused = 0; // the last return value of get_unused

  vec3 pos[AMOUNT];
  vec3 vel[AMOUNT];
  float life[AMOUNT];

  Particles() {
    for (auto &l : life) {
      l = -1.0f;
    }
  }

  /** Returns the index of an unused particle. (The dead ones)
   * If none could be found, the first one is returned.
   */
  unsigned int get_unused(void) {
    // start searching from the last returned pos
    for (auto i = _last_unused; i < AMOUNT; i++) {
      if (life[i] < 0.0f) {
        _last_unused = i;
        return i;
      }
    }

    // if we still not found any unused particle we search from the beginning
    // to _last_unused
    for (auto i = 0; i < _last_unused; i++) {
      if (life[i] < 0.0f) {
        _last_unused = i;
        return i;
      }
    }

    // and if we havent found any unused at this point, all particle must be
    // used
    _last_unused = 0;
    return 0;
  }

  /** Add a new particle.
   * If the particle cap (Particle.AMOUNT) is reached, the first particle is
   * overwritten.
   */
  void add(vec3 particle_pos, vec3 particle_vel) {
    auto i = get_unused();
    pos[i] = particle_pos;
    vel[i] = particle_vel;
    life[i] = _INIT_LIFE;
  }
};

const float PARTICLE_RADIUS = 0.05;

// every particle is just a square
const std::vector<float> VERTICES = {
    -PARTICLE_RADIUS, -PARTICLE_RADIUS, 0.0f, // vertex 1
    PARTICLE_RADIUS,  -PARTICLE_RADIUS, 0.0f, // vertex 2
    -PARTICLE_RADIUS, PARTICLE_RADIUS,  0.0f, // vertex 3
    PARTICLE_RADIUS,  PARTICLE_RADIUS,  0.0f, // vertex 4
};
const std::vector<unsigned int> INDICES = {
    0, 1, 2, // triangle 1
    2, 1, 3, // triangle 2
};

const std::string vertexShaderSource = R"(
#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 offset;

uniform mat4 viewProjection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform float particleRadius;

void main() {
    vec3 worldPos = aPos + offset;
    vec3 billboarded = offset
                        + cameraRight * aPos.x * particleRadius
                        + cameraUp * aPos.y * particleRadius;

    gl_Position = viewProjection * vec4(billboarded, 1.0);
}
)";

const std::string fragmentShaderSource = R"(
#version 410 core

out vec4 color;

void main() {
    color = vec4(1.0, 0.4, 0.1, 1.0); // orange explosion
}
)";

struct MainApp : App {
  Program program;
  Mesh mesh;
  Camera camera;

  GLuint offsetVBO;

  Particles particles;

  MainApp() : App(600, 500) {
    program.loadSource(vertexShaderSource, fragmentShaderSource);

    mesh.load(VERTICES, INDICES);
    // add offset buffer at location = 1
    {
      // create the buffer
      glGenBuffers(1, &offsetVBO);
      glBindBuffer(GL_ARRAY_BUFFER, offsetVBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(particles.pos), particles.pos,
                   GL_DYNAMIC_DRAW);

      // attach the buffer
      mesh.vao.bind();
      glBindBuffer(GL_ARRAY_BUFFER, offsetVBO);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, // location
                            3, // vec3
                            GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

      // Advance once per instance instead of once per vertex
      glVertexAttribDivisor(1, 1);

      glBindVertexArray(0);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  void spawnExplosion(const glm::vec3 &center) {
    for (int i = 0; i < 10'000; i++) {
      vec3 dir = vec3(rand() - RAND_MAX / 2, rand() - RAND_MAX / 2,
                      rand() - RAND_MAX / 2);

      particles.add(center, normalize(dir) * 0.5f);
    }
  }

  void updateParticles(float dt) {
    for (int i = 0; i < particles.AMOUNT; i++) {
      if (particles.life[i] > 0.0f) {
        particles.life[i] -= dt;
        particles.pos[i] += particles.vel[i] * dt;
      } else {
        particles.pos[i] = vec3(9999.0);
      }
    }
  }

  void render() override {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateParticles(App::delta);

    camera.update();
    mat4 VP = camera.projectionMatrix * camera.viewMatrix;
    vec3 camera_right = {camera.viewMatrix[0][0], camera.viewMatrix[1][0],
                         camera.viewMatrix[2][0]};
    vec3 camera_up = {camera.viewMatrix[0][1], camera.viewMatrix[1][1],
                      camera.viewMatrix[2][1]};

    program.use();
    program.set("viewProjection", VP);
    program.set("cameraRight", camera_right);
    program.set("cameraUp", camera_up);
    program.set("particleRadius", PARTICLE_RADIUS);

    // update offsets buffer
    glBindBuffer(GL_ARRAY_BUFFER, offsetVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(particles.pos), particles.pos);

    mesh.draw(particles.AMOUNT);
  }

  void buildImGui() override {
    ImGui::StatisticsWindow(App::delta, App::resolution);
  }

  void keyCallback(Key key, Action action, Modifier modifier) override {
    if (action == Action::PRESS)
      spawnExplosion(vec3(0, 0, 0));
  }

  void moveCallback(const vec2 &movement, bool leftButton, bool rightButton,
                    bool middleButton) override {
    if (rightButton | middleButton)
      camera.orbit(movement * 0.01f);
  }
};

int main() {
  MainApp app;
  app.run();
  return 0;
}
