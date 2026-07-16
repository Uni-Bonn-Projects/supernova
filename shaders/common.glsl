#line 2

const uint MAX_POINT_COUNT = 600u;
const uint MAX_TRIANGLE_COUNT = 100u;

struct SNMesh {
  vec4 vertices[MAX_POINT_COUNT];
  vec4 normals[MAX_POINT_COUNT];
  uvec4 indices[MAX_TRIANGLE_COUNT];
  uint pointCount;
  uint triangleCount;
  uint ssbo_id;
};

struct Oldman {
  SNMesh top_dome;
  SNMesh bottom_body_cut;
  SNMesh bottom_dome;
  SNMesh sections[12];
  uint ssbo_id;
};

struct RaytraceResult {
  vec3 hitPos;
  vec3 normal;
  vec3 objectColor;
  float distance;
  bool glowing;
};

const vec3 starshipColor = vec3(0.4, 0.4, 0.4);
const vec3 moonColor = vec3(0.96, 0.91, 0.77);
const vec3 sunColor = vec3(0.98, 0.81, 0.32);
const vec3 earthColor = vec3(0.0, 0.0, 0.62);
