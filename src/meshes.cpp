#include "glm/gtc/type_ptr.hpp"
#include <glm/glm.hpp>

#include <framework/mesh.hpp>
#include <framework/objparser.hpp>

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

  self.edgeCount = indices.size() / 3;
  self.pointCount = vertices.size();
  for (auto i = 0; i < self.pointCount; i++) {
    self.vertices[i] = vertices[i].position;
    self.normals[i] = vertices[i].normal;
  }
  for (auto i = 0; i < self.edgeCount; i += 1) {
    auto j = i * 3;
    self.indices[i] = glm::vec3(indices[j], indices[j + 1], indices[j + 2]);
  }

  return self;
}

SNMesh moveMesh(SNMesh self, float km, vec3 dir) {
  for (uint i = 0; i < self.pointCount; i++) {
    self.vertices[i] += km * dir;
  }

  return self;
}

SNMesh scaleMesh(SNMesh self, float scale, vec3 amount) {
  for (uint i = 0; i < self.pointCount; i++) {
    self.vertices[i].x *= scale * amount.x;
    self.vertices[i].y *= scale * amount.y;
    self.vertices[i].z *= scale * amount.z;
  }

  return self;
}

SNMesh initMesh(SNMesh self, Program &program) {
  // Pass the vertices to the shader as vec3 array
  glProgramUniform3fv(program.handle, program.uniform("uVertices"),
                      self.pointCount, value_ptr(self.vertices[0]));

  // Pass the normals to the shader as vec3 array
  glProgramUniform3fv(program.handle, program.uniform("uNormals"),
                      self.pointCount, value_ptr(self.normals[0]));

  // Pass the indices to the shader as uvec3 array, each uvec3 being a
  // triangle
  glProgramUniform3uiv(program.handle, program.uniform("uIndices"),
                       self.edgeCount, value_ptr(self.indices[0]));

  // Pass triangle count to the shader
  program.set("uTriangleCount", self.edgeCount);
  program.set("uVerticesCount", self.pointCount);

  return self;
}

SNMesh updateMesh(SNMesh self, Program &program) {
  // TODO: impl
  return self;
}

}; // namespace sn
