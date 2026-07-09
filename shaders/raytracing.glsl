#include "math.glsl"
#include "procedural.glsl"
#include "uniforms.glsl"

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
    return vec3(Inf);
  }
}

/// Takes a ray and background color and returns the color of the point hit.
vec3 raytrace(vec3 rayOrigin, vec3 rayDir, vec3 background_color) {
  vec3 color = background_color;
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

      // Shade
      color = max(0.0, dot(normal, uLightDir)) + vec3(0.005);
    }
  }

  // procedual stuff
  // (only a shere for now)
  vec3 sphere_pos = vec3(60, 20, 20);
  float sphere_radius = 10;

  float result = proceduralSphere(rayOrigin, rayDir, sphere_pos, sphere_radius);
  if (result < depth) {
    color = vec3(1.0, 1.0, 0.0);
  }

  return color;
}
