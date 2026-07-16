#line 2

const uint MAX_POINT_COUNT = 600u;
const uint MAX_TRIANGLE_COUNT = 100u; // TODO: maybe export into a uniform

struct SNMesh {
  vec4 vertices[MAX_POINT_COUNT];
  vec4 normals[MAX_POINT_COUNT];
  uvec4 indices[MAX_TRIANGLE_COUNT];
  uint pointCount;
  uint triangleCount;
};

layout(std430, binding = 0) readonly buffer MeshData {
  SNMesh umesh;
};
