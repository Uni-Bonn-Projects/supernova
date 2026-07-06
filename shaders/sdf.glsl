/* Struct to hold the signed distance and ID of the nearest object */
struct SD {
  float dist;
  int ID;
  bool glowing; // whether or not this object glows
};

SD sdUnion(SD a, SD b) {
  return a.dist < b.dist ? a : b;
}

// begin: boolean operators

float intersectSDF(float distA, float distB) {
  return max(distA, distB);
}

float unionSDF(float distA, float distB) {
  return min(distA, distB);
}

float differenceSDF(float distA, float distB) {
  return max(distA, -distB);
}

// end: boolean operators

// begin: rotation

vec3 rotateZ(vec3 pos, float angle) {
  float c = cos(angle);
  float s = sin(angle);

  mat3 m = mat3(
      c, s, 0.0,
      -s, c, 0.0,
      0.0, 0.0, 1.0
    );
  return m * pos;
}

vec3 rotateY(vec3 pos, float angle) {
  float c = cos(angle);
  float s = sin(angle);

  mat3 m = mat3(
      c, 0.0, s,
      0.0, 1.0, 0.0,
      -s, 0.0, c
    );
  return m * pos;
}

// end: rotation

// begin: objects

float sdSphere(vec3 pos, float radius) {
  return length(pos) - radius;
}

float sdBox(vec3 pos, vec3 size) {
  vec3 q = abs(pos) - size;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// signed distance to a capsule
float sdCapsule(vec3 pos, vec3 a, vec3 b, float radius) {
  vec3 pa = pos - a, ba = b - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
  return length(pa - ba * h) - radius;
}

// unsigned distance from pos to the segment axi, used by the laser glow
float distToAxis(vec3 pos, vec3 a, vec3 b) {
  vec3 pa = pos - a, ba = b - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
  return length(pa - ba * h);
}

// end: objects
