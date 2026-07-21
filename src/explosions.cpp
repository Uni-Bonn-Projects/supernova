#include "framework/app.hpp"
#include "framework/camera.hpp"
#include "framework/gl/program.hpp"
#include "framework/mesh.hpp"
#include "glm/fwd.hpp"
#include <cassert>
#include <cstdlib>
#include <vector>

#include "explosions.h"

using namespace glm;

namespace sn {

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

unsigned int Particles::get_unused(void) {
  // start searching from the last returned pos
  for (auto i = _last_unused; i < AMOUNT; i++) {
    if (alive_prob[i] <= 0.0f) {
      _last_unused = i;
      return i;
    }
  }

  // if we still not found any unused particle we search from the beginning
  // to _last_unused
  for (auto i = 0; i < _last_unused; i++) {
    if (alive_prob[i] <= 0.0f) {
      _last_unused = i;
      return i;
    }
  }

  // and if we havent found any unused at this point, all particle must be
  // used
  _last_unused = 0;
  return 0;
}
void Particles::add(vec3 particle_pos, vec3 particle_vel,
                    float particle_max_duration) {
  auto i = get_unused();
  pos[i] = particle_pos;
  vel[i] = particle_vel;
  max_duration[i] = particle_max_duration;
  alive_prob[i] = 1.0;
}

void Particles::update(float delta_time) {
  const vec3 the_ranch = vec3(999'999.0);

  for (int i = 0; i < AMOUNT; i++) {
    if (alive_prob[i] >= 0.0f) {
      const float prob = (float)rand() / (1.2 * RAND_MAX);

      if (prob <= alive_prob[i]) {
        // particle lives on
        alive_prob[i] -= delta_time / max_duration[i];
        pos[i] += vel[i] * delta_time;
      } else {
        // particle has lost the lottery
        alive_prob[i] = 0.0;
        pos[i] = the_ranch;
      }
    } else {
      pos[i] = the_ranch;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//                                 Explosions                                //
///////////////////////////////////////////////////////////////////////////////

void Explosions::init(void) {
  _program.loadSource(vertexShaderSource, fragmentShaderSource);

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
}

void Explosions::render(Camera &camera, float delta_time, bool camera_changed) {
  _program.use();

  _particles.update(delta_time);

  // Set the billboard basis + view-projection every frame: during a static
  // cinematic hold the camera doesn't report as "changed", but an explosion
  // spawned then must still be billboarded with a valid matrix.
  mat4 VP = camera.projectionMatrix * camera.viewMatrix;
  vec3 camera_right = {camera.viewMatrix[0][0], camera.viewMatrix[1][0],
                       camera.viewMatrix[2][0]};
  vec3 camera_up = {camera.viewMatrix[0][1], camera.viewMatrix[1][1],
                    camera.viewMatrix[2][1]};
  _program.set("viewProjection", VP);
  _program.set("cameraRight", camera_right);
  _program.set("cameraUp", camera_up);
  (void)camera_changed;

  _program.set("particleRadius", _particles.current_radius);

  // update offsets buffer
  glBindBuffer(GL_ARRAY_BUFFER, _offsetVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(_particles.pos), _particles.pos);

  _mesh.draw(_particles.AMOUNT);
}

void Explosions::spawn(const glm::vec3 &center, float duration, float size) {
  // size <= 0: legacy point-blank burst (slow drift, tiny billboards).
  // size > 0: scale the outward speed so particles spread to ~`size` over the
  // burst, and enlarge the billboards to match, so the blast reads from far.
  bool scaled = size > 0.0f;
  float maxSpeed = scaled ? size / duration : 0.5f;

  for (int i = 0; i < 10'000; i++) {
    vec3 dir = vec3(rand() - RAND_MAX / 2, rand() - RAND_MAX / 2,
                    rand() - RAND_MAX / 2);

    _particles.add(center,
                   normalize(dir) * maxSpeed * ((float)rand() / RAND_MAX),
                   duration);
  }

  _particles.current_radius = scaled ? size * 0.06f : _particles.RADIUS;
}

bool Explosions::isActive(void) {
  for (int i = 0; i < _particles.AMOUNT; i++) {
    if (_particles.alive_prob[i] > 0.0f)
      return true;
  }
  return false;
}

}; // namespace sn
