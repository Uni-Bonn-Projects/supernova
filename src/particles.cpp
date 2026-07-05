#include "framework/app.hpp"
#include "framework/camera.hpp"
#include "framework/gl/program.hpp"
#include "framework/imguiutil.hpp"
#include "framework/mesh.hpp"
#include "glm/fwd.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace glm;

struct Particles {
  static const unsigned int AMOUNT = 1000;
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
    std::cout << "particle cap reached" << std::endl;
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

const float PARTICLE_RADIUS = 0.01;

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

uniform mat4 viewProjection;
uniform vec3 offsets[1000];

void main() {
    int id = gl_InstanceID;

    vec3 worldPos = aPos + offsets[id];

    gl_Position = viewProjection * vec4(worldPos, 1.0);
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

  Particles particles;

  MainApp() : App(600, 500) {
    program.loadSource(vertexShaderSource, fragmentShaderSource);

    mesh.load(VERTICES, INDICES);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  void spawnExplosion(const glm::vec3 &center) {
    const int DIRECTIONS = 100'000;

    for (int i = 0; i < 100; i++) {
      vec3 dir = vec3(rand() - RAND_MAX / 2, rand() - RAND_MAX / 2,
                      rand() - RAND_MAX / 2);

      particles.add(center, normalize(dir) * 1.0f);
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

    float dt = App::delta;

    camera.update();
    glm::mat4 VP = camera.projectionMatrix * camera.viewMatrix;

    updateParticles(dt);

    program.use();
    program.set("viewProjection", VP);

    // upload offsets array
    GLint loc = program.uniform("offsets");
    glUniform3fv(loc, particles.AMOUNT, &particles.pos[0].x);

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
