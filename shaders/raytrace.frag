#version 330 core

in vec3 viewDir;
out vec3 fragColor;

const uint MAX_TRIANGLES = 100u;
uniform vec4 uVertices[MAX_TRIANGLES * 3u * 2u];
uniform uvec3 uIndices[MAX_TRIANGLES];
uniform uint uTriangleCount;

uniform vec3 uCameraPosition;

uniform uint uView = 0u;
uniform vec3 uLightDir = normalize(vec3(1.0));
uniform vec3 uLightColor = vec3(1.0);
uniform float uNear = 0.1;
uniform float uFar = 100.0;

const float INF = 1.0 / 0.0;
const float NAN = 0.0 / 0.0;

vec3 proceduralSun(vec3 rayDir) {
  return pow(max(0.0, dot(rayDir, uLightDir)), 1000) * uLightColor;
}

vec3 intersectTriangle(vec3 rayOrigin, vec3 rayDir, vec3 v0, vec3 v1, vec3 v2) {
  // edges
  vec3 e1 = v1 - v0;
  vec3 e2 = v2 - v0;

  // plane
  vec3 n = cross(e1, e2);
  vec3 c = v0;

  // ray till triangle
  float t = dot(n, c - rayOrigin) / dot(n, rayDir);
  vec3 p = rayOrigin + t * rayDir - c;

  float u = dot(cross(p, e2), n) / dot(n, n);
  float v = dot(cross(e1, p), n) / dot(n, n);

  if (u > 0 && v > 0 && u + v < 1) {
    return vec3(u, v, t);
  } else {
    return vec3(INF);
  }
}

/// Calculate the distance to the given procedual Sphere
///
/// It is assumed that rayDir is normalized.
float procedualSphere(
  vec3 rayOrigin,
  vec3 rayDir,
  vec3 spherePos,
  float sphereRadius
) {
  vec3 oToSP = rayOrigin - spherePos; // rayOrigin to SpherePos
  float h = dot(oToSP, rayDir); // result we use a lot

  // no result if rhs would need an imaginary number
  float sqrt_inner = h * h - dot(oToSP, oToSP) + sphereRadius * sphereRadius;
  if (sqrt_inner < 0.0) {
    return INF;
  }

  // left and right side of the quadratic formula (seperator is the +-)
  float lhs = -h;
  float rhs = sqrt(sqrt_inner);

  // use x1 if is infront of the camera
  // use x2 else
  float x = lhs + rhs;
  if (x < 0.0) {
    x = lhs - rhs;
  }

  // return the selected x if it is in front of the camera
  // no result else
  // (both x's can be behind the camera)
  return (x >= 0) ? x : INF;
}

void main() {
  // Generate camera ray
  vec3 rayDir = normalize(viewDir); // Renormalize after interpolation
  vec3 rayOrigin = uCameraPosition;

  // Clear with background
  fragColor = proceduralSun(rayDir);
  float depth = uFar;

  // Loop through each triangle uIndices[i].xyz
  for (uint i = 0u; i < uTriangleCount; i++) {

    // Fetch vertex positions
    vec3 v0 = uVertices[uIndices[i].x * 2u].xyz;
    vec3 v1 = uVertices[uIndices[i].y * 2u].xyz;
    vec3 v2 = uVertices[uIndices[i].z * 2u].xyz;

    vec3 result = intersectTriangle(rayOrigin, rayDir, v0, v1, v2);

    // Overdraw if closer
    if (result.z < depth) {
      // Unpack result
      depth = result.z;
      vec3 barycentrics = vec3(1.0 - result.x - result.y, result.xy);

      // Fetch vertex normals
      vec3 n0 = uVertices[uIndices[i].x * 2u + 1u].yzw;
      vec3 n1 = uVertices[uIndices[i].y * 2u + 1u].yzw;
      vec3 n2 = uVertices[uIndices[i].z * 2u + 1u].yzw;

      // Interpolate normals
      vec3 normal = normalize(mat3(n0, n1, n2) * barycentrics);
      // = normalize(n0 * barycentrics.x + n1 * barycentrics.y + n2 * barycentrics.z);

      // Shade
      fragColor = max(dot(normal, uLightDir), 0.0) * uLightColor + vec3(0.005);

      // Debug views
      if (uView == 1u) fragColor = normal * 0.5 + 0.5;
      if (uView == 2u) fragColor = vec3(1.0 - (depth / uFar));
      if (uView == 3u) fragColor = barycentrics;
    }
  }

  // procedual stuff
  // (only a shere for now)
  vec3 sphere_pos = vec3(60, 20, 20);
  float sphere_radius = 10;

  float result = procedualSphere(rayOrigin, rayDir, sphere_pos, sphere_radius);
  if (result < depth) {
    fragColor = vec3(1.0, 1.0, 0.0);
  }
}
