#line 2

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
