#include <cassert>
#include <cstdint>

#include <framework/mesh.hpp>
#include <framework/objparser.hpp>
#include <glm/glm.hpp>

#include <string>
#include <vector>

using namespace glm;

#include "meshes.h"

namespace sn {

SNMesh meshFromObj(const std::string &filename) {
  SNMesh self;

  std::vector<Mesh::VertexPTN> vertices;
  std::vector<unsigned int> indices;
  ObjParser::parse(filename, vertices, indices);

  self.triangleCount = indices.size() / 3;
  self.pointCount = vertices.size();

  assert(self.triangleCount <= MAX_TRIANGLE_COUNT);
  assert(self.pointCount <= MAX_TRIANGLE_COUNT);

  for (auto i = 0; i < self.pointCount; i++) {
    self.vertices[i] = vec4(vertices[i].position, 0);
    self.normals[i] = vec4(vertices[i].normal, 0);
  }
  for (auto i = 0; i < self.triangleCount; i += 1) {
    auto j = i * 3;
    self.indices[i] = vec4(indices[j], indices[j + 1], indices[j + 2], 0);
  }

  return self;
}

SNMesh &moveMesh(SNMesh &self, float km, vec3 dir) {
  for (uint i = 0; i < self.pointCount; i++) {
    self.vertices[i] += vec4(km * dir, 0);
  }

  return self;
}

SNMesh &scaleMesh(SNMesh &self, float scale, vec3 amount) {
  for (uint i = 0; i < self.pointCount; i++) {
    self.vertices[i].x *= scale * amount.x;
    self.vertices[i].y *= scale * amount.y;
    self.vertices[i].z *= scale * amount.z;
  }

  return self;
}

SNMesh &initMesh(SNMesh &self, Program &program, uint32_t binding) {
  GLuint ssbo;
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SNMesh), &self, GL_STATIC_DRAW);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);

  return self;
}

SNMesh &updateMesh(SNMesh &self, Program &program) {
  // TODO: impl
  return self;
}

}; // namespace sn
