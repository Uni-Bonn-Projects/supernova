#include <glm/glm.hpp>

#include "framework/gl/program.hpp"
#include "glm/trigonometric.hpp"
#include "meshes.h"
#include "oldman.h"

namespace sn {

using namespace glm;

Oldman &Oldman::init(SNMesh unit_sphere, SNMesh unit_cube, Program &program,
                     uint32_t binding) {
  this->top_dome = unit_sphere;
  scaleMesh(this->top_dome, 100, vec3(1));

  this->bottom_body_cut = unit_cube;
  scaleMesh(this->bottom_body_cut, 100, vec3(1, 1, 0.5));
  moveMesh(this->bottom_body_cut, 50, vec3(0, 0, 1));

  this->bottom_dome = unit_sphere;
  scaleMesh(this->bottom_dome, 25, vec3(1));

  for (auto i = 0; i < 12; i++) {
    auto angle = radians(30.0 * i);
    auto dir = vec3(cos(angle), 0.0, sin(angle));

    this->sections[i] = unit_cube;
    scaleMesh(this->sections[i], 10, vec3(5, 1, 5));
    moveMesh(this->sections[i], 100, dir);
  }

  glGenBuffers(1, &this->ssbo_id);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo_id);

  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Oldman), this, GL_DYNAMIC_DRAW);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, this->ssbo_id);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return *this;
}
Oldman &Oldman::update(Program &program) {
  // TODO: impl
  return *this;
}

Oldman &Oldman::move(float km, glm::vec3 dir) {
  // TODO: impl
  return *this;
}
Oldman &Oldman::scale(float scale, glm::vec3 amount) {
  // TODO: impl
  return *this;
}

}; // namespace sn
