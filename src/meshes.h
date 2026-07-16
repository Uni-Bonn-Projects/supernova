#include "framework/gl/program.hpp"
#include <cstdint>
#include <glm/glm.hpp>

#include <string>

namespace sn {

const size_t MAX_POINT_COUNT = 600;
const size_t MAX_EDGE_COUNT = 100;

// TODO: check real std140 layout
// with 16-byte alignment to match opengl std140 layout
struct alignas(16) SNMesh {
  glm::vec3 vertices[MAX_POINT_COUNT];
  glm::vec3 normals[MAX_POINT_COUNT];
  glm::uvec3 indices[MAX_EDGE_COUNT];
  uint32_t pointCount;
  uint32_t edgeCount;
};

SNMesh meshFromObj(const std::string &filename);

/** Move the given Mesh km kilometers in direction dir */
SNMesh moveMesh(SNMesh self, float km, glm::vec3 dir);
/** Scale a mesh by the given scale.
 * The vec3 amount defines how much to scale in which axis. (in percent)
 */
SNMesh scaleMesh(SNMesh self, float scale, glm::vec3 amount);

/** Initialise the mesh's data in the GPU */
SNMesh initMesh(SNMesh self, Program &program);

/** Update the mesh's data in the GPU */
SNMesh updateMesh(SNMesh self, Program &program);

}; // namespace sn
