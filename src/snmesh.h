#pragma once

#include <cstdint>

#include <framework/gl/program.hpp>
#include <glm/glm.hpp>

#include <string>

namespace sn {

const size_t MAX_POINT_COUNT = 600;
const size_t MAX_TRIANGLE_COUNT = 100;

struct alignas(16) SNMesh {
  // using vec4's because of 16 byte alignment of glsl std430 layout
  glm::vec4 vertices[MAX_POINT_COUNT];
  glm::vec4 normals[MAX_POINT_COUNT];
  glm::uvec4 indices[MAX_TRIANGLE_COUNT];

  uint32_t pointCount;
  uint32_t triangleCount;

  GLuint ssbo_id;

  /** Load .obj file into this struct */
  SNMesh &fromObj(const std::string &filename);

  /** Move the given Mesh km kilometers in direction dir */
  SNMesh &move(float km, glm::vec3 dir);
  /** Scale a mesh by the given scale.
   * The vec3 amount defines how much to scale in which axis. (in percent)
   */
  SNMesh &scale(float scale, glm::vec3 amount);
  /** Rotate a mesh by the given angle (in radians) around a given axis.
   */
  SNMesh &rotate(float angle, glm::vec3 axis);

  /** Initialise the mesh's data in the GPU.
   * Binding must match to one from the shader.
   */
  SNMesh &init(Program &program, uint32_t binding);

  /** Update the mesh's data in the GPU
   * Binding must match to one from the shader.
   */
  SNMesh &update(Program &program);
};

}; // namespace sn
