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
  this->top_dome.scale(100, vec3(1));

  this->bottom_body_cut = unit_cube;
  this->bottom_body_cut.scale(100, vec3(1, 1, 0.5)).move(50, vec3(0, 0, 1));

  this->bottom_dome = unit_sphere;
  this->bottom_dome.scale(25, vec3(1));

  for (auto i = 0; i < 12; i++) {
    auto angle = radians(30.0 * i);
    auto dir = vec3(cos(angle), 0.0, sin(angle));

    this->sections[i] = unit_cube;
    this->sections[i].scale(10, vec3(5, 1, 5)).move(100, dir);
  }

  glGenBuffers(1, &this->ssbo_id);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo_id);

  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Oldman), this, GL_DYNAMIC_DRAW);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, this->ssbo_id);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return *this;
}
Oldman &Oldman::update(Program &program) {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo_id);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Oldman), this);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return *this;
}

Oldman &Oldman::move(float km, glm::vec3 dir) {
  this->top_dome.move(km, dir);
  this->bottom_body_cut.move(km, dir);
  this->bottom_dome.move(km, dir);

  for (auto &s : this->sections) {
    s.move(km, dir);
  }

  return *this;
}

}; // namespace sn
