#include <cassert>
#include <cstdint>

#include <framework/mesh.hpp>
#include <framework/objparser.hpp>
#include <glm/glm.hpp>

#include <string>
#include <vector>

using namespace glm;

#include "snmesh.h"

namespace sn {

SNMesh &SNMesh::fromObj(const std::string &filename) {
  std::vector<Mesh::VertexPTN> vertices;
  std::vector<unsigned int> indices;
  ObjParser::parse(filename, vertices, indices);

  this->triangleCount = indices.size() / 3;
  this->pointCount = vertices.size();

  assert(this->triangleCount <= MAX_TRIANGLE_COUNT);
  assert(this->pointCount <= MAX_TRIANGLE_COUNT);

  for (auto i = 0; i < this->pointCount; i++) {
    this->vertices[i] = vec4(vertices[i].position, 0);
    this->normals[i] = vec4(vertices[i].normal, 0);
  }
  for (auto i = 0; i < this->triangleCount; i += 1) {
    auto j = i * 3;
    this->indices[i] = vec4(indices[j], indices[j + 1], indices[j + 2], 0);
  }

  return *this;
}

SNMesh &SNMesh::move(float km, vec3 dir) {
  for (uint i = 0; i < this->pointCount; i++) {
    this->vertices[i] += vec4(km * dir, 0);
  }

  return *this;
}

SNMesh &SNMesh::scale(float scale, vec3 amount) {
  for (uint i = 0; i < this->pointCount; i++) {
    this->vertices[i].x *= scale * amount.x;
    this->vertices[i].y *= scale * amount.y;
    this->vertices[i].z *= scale * amount.z;
  }

  return *this;
}

SNMesh &SNMesh::init(Program &program, uint32_t binding) {
  glGenBuffers(1, &this->ssbo_id);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo_id);

  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SNMesh), this, GL_DYNAMIC_DRAW);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, this->ssbo_id);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return *this;
}

SNMesh &SNMesh::update(Program &program) {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo_id);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(SNMesh), this);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return *this;
}

}; // namespace sn
