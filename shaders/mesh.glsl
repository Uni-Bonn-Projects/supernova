#line 2

const uint MAX_POINT_COUNT = 600u;
const uint MAX_EDGE_COUNT = 100u; // TODO: maybe export into a uniform

struct Mesh {
  vec3 vertices[MAX_POINT_COUNT];
  vec3 normals[MAX_POINT_COUNT];
  uvec3 indices[MAX_EDGE_COUNT];
  uint vertexCount;
  uint indexCount;
};
