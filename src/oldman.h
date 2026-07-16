#pragma once

#include <glm/glm.hpp>

#include "framework/gl/program.hpp"
#include "meshes.h"

namespace sn {

struct alignas(16) Oldman {
  SNMesh top_dome;
  SNMesh bottom_body_cut;
  SNMesh bottom_dome;
  SNMesh sections[12];

  GLuint ssbo_id;

  /** Initialise the OLDMAN meshes using a unit shere and unit cube.
   *
   * unit shere refers to a shere with radius 1.
   * unit cube refers to a cube with lengths 2. (radius 1)
   */
  Oldman &init(SNMesh unit_sphere, SNMesh unit_cube, Program &program,
               uint32_t binding);
  Oldman &update(Program &program);
  Oldman &move(float km, glm::vec3 dir);
  Oldman &scale(float scale, glm::vec3 amount);
};

}; // namespace sn
