#include "framework/app.hpp"
#include "framework/camera.hpp"
#include "framework/gl/program.hpp"
#include "framework/imguiutil.hpp"
#include "framework/mesh.hpp"
#include "glm/fwd.hpp"
#include <cassert>
#include <cstdlib>
#include <vector>

#include "particles.h"

using namespace glm;

// every particle is just a square
const std::vector<float> VERTICES = {
    -1.0f, -1.0f, 0.0f, // vertex 1
    1.0f,  -1.0f, 0.0f, // vertex 2
    -1.0f, 1.0f,  0.0f, // vertex 3
    1.0f,  1.0f,  0.0f, // vertex 4
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

///////////////////////////////////////////////////////////////////////////////
//                                 Particles                                 //
///////////////////////////////////////////////////////////////////////////////

Particles::Particles() {
  for (auto &l : life) {
    l = -1.0f;
  }
}
unsigned int Particles::get_unused(void) {
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
void Particles::add(vec3 particle_pos, vec3 particle_vel) {
  auto i = get_unused();
  pos[i] = particle_pos;
  vel[i] = particle_vel;
  life[i] = _INIT_LIFE;
}

///////////////////////////////////////////////////////////////////////////////
//                                 Explosions                                //
///////////////////////////////////////////////////////////////////////////////

void Explosions::init(void) {
  _program.loadSource(vertexShaderSource, fragmentShaderSource);
  _program.use();

  _mesh.load(VERTICES, INDICES);

  // add offset buffer at location = 1
  {
    // create the buffer
    glGenBuffers(1, &_offsetVBO);
    glBindBuffer(GL_ARRAY_BUFFER, _offsetVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(_particles.pos), _particles.pos,
                 GL_DYNAMIC_DRAW);

    // attach the buffer
    _mesh.vao.bind();
    glBindBuffer(GL_ARRAY_BUFFER, _offsetVBO);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, // location
                          3, // vec3
                          GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

    // Advance once per instance instead of once per vertex
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);
  }

  // enable some other opengl features
  // TODO: really required?
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Explosions::render(Camera &camera, float delta_time) {
  _updateParticles(delta_time);

  if (camera.updateIfChanged()) {
    mat4 VP = camera.projectionMatrix * camera.viewMatrix;
    vec3 camera_right = {camera.viewMatrix[0][0], camera.viewMatrix[1][0],
                         camera.viewMatrix[2][0]};
    vec3 camera_up = {camera.viewMatrix[0][1], camera.viewMatrix[1][1],
                      camera.viewMatrix[2][1]};
    _program.set("viewProjection", VP);
    _program.set("cameraRight", camera_right);
    _program.set("cameraUp", camera_up);
  }

  _program.set("particleRadius", _particles.RADIUS);

  // update offsets buffer
  glBindBuffer(GL_ARRAY_BUFFER, _offsetVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(_particles.pos), _particles.pos);

  _mesh.draw(_particles.AMOUNT);
}

void Explosions::spawn(const glm::vec3 &center) {
  for (int i = 0; i < 10'000; i++) {
    vec3 dir = vec3(rand() - RAND_MAX / 2, rand() - RAND_MAX / 2,
                    rand() - RAND_MAX / 2);

    _particles.add(center, normalize(dir) * 0.5f);
  }
}

void Explosions::_updateParticles(float dt) {
  for (int i = 0; i < _particles.AMOUNT; i++) {
    if (_particles.life[i] > 0.0f) {
      _particles.life[i] -= dt;
      _particles.pos[i] += _particles.vel[i] * dt;
    } else {
      _particles.pos[i] = vec3(9999.0);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//                                Example Code                               //
///////////////////////////////////////////////////////////////////////////////

struct MainApp : App {
  Camera camera;

  Explosions explosions;

  MainApp() : App(600, 500) { explosions.init(); }

  void render() override {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    explosions.render(camera, App::delta);
  }

  void buildImGui() override {
    ImGui::StatisticsWindow(App::delta, App::resolution);
  }

  void keyCallback(Key key, Action action, Modifier modifier) override {
    if (action == Action::PRESS)
      explosions.spawn(vec3(0, 0, 0));
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
