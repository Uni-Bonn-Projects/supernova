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

  self.data.triangleCount = indices.size() / 3;
  self.data.pointCount = vertices.size();

  assert(self.data.triangleCount <= MAX_TRIANGLE_COUNT);
  assert(self.data.pointCount <= MAX_TRIANGLE_COUNT);

  for (auto i = 0; i < self.data.pointCount; i++) {
    self.data.vertices[i] = vec4(vertices[i].position, 0);
    self.data.normals[i] = vec4(vertices[i].normal, 0);
  }
  for (auto i = 0; i < self.data.triangleCount; i += 1) {
    auto j = i * 3;
    self.data.indices[i] = vec4(indices[j], indices[j + 1], indices[j + 2], 0);
  }

  return self;
}

SNMesh &moveMesh(SNMesh &self, float km, vec3 dir) {
  for (uint i = 0; i < self.data.pointCount; i++) {
    self.data.vertices[i] += vec4(km * dir, 0);
  }

  return self;
}

SNMesh &scaleMesh(SNMesh &self, float scale, vec3 amount) {
  for (uint i = 0; i < self.data.pointCount; i++) {
    self.data.vertices[i].x *= scale * amount.x;
    self.data.vertices[i].y *= scale * amount.y;
    self.data.vertices[i].z *= scale * amount.z;
  }

  return self;
}

SNMesh &initMesh(SNMesh &self, Program &program, uint32_t binding) {
  glGenBuffers(1, &self.ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, self.ssbo);

  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SNMesh::data_t), &self.data,
               GL_DYNAMIC_DRAW);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, self.ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return self;
}

SNMesh &updateMesh(SNMesh &self, Program &program) {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, self.ssbo);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(SNMesh::data_t),
                  &self.data);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return self;
}

}; // namespace sn
