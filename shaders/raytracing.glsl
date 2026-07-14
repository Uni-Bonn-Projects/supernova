#include "common.glsl"
#include "math.glsl"
#include "procedural_raytracing.glsl"
#include "uniforms.glsl"
#line 6

vec3 intersectTriangle(
  vec3 rayOrigin,
  vec3 rayDir,
  vec3 v0,
  vec3 v1,
  vec3 v2
) {
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

  if (u > 0 && v > 0 && u + v < 1 && t > 0) {
    return vec3(u, v, t);
  } else {
    return vec3(Inf);
  }
}

/// We are only using triangles to render OLDMAN. Therefore, this function
/// raytraces all triangle stuff with some values, like color, hardcoded for
/// OLDMAN.
RaytraceResult raytraceOldman(
  vec3 rayOrigin,
  vec3 rayDir,
  RaytraceResult result
) {
  // Loop through each triangle uIndices[i].xyz
  for (uint i = 0u; i < uTriangleCount; i++) {
    // Fetch vertex positions
    vec3 v0 = uVertices[uIndices[i].x * 2u].xyz;
    vec3 v1 = uVertices[uIndices[i].y * 2u].xyz;
    vec3 v2 = uVertices[uIndices[i].z * 2u].xyz;

    vec3 curResult = intersectTriangle(rayOrigin, rayDir, v0, v1, v2);

    // Overdraw if closer
    if (curResult.z < result.distance) {
      vec3 barycentrics = vec3(1.0 - curResult.x - curResult.y, curResult.xy);

      // Fetch vertex normals
      vec3 n0 = uVertices[uIndices[i].x * 2u + 1u].yzw;
      vec3 n1 = uVertices[uIndices[i].y * 2u + 1u].yzw;
      vec3 n2 = uVertices[uIndices[i].z * 2u + 1u].yzw;

      result.hitPos = curResult;
      result.normal = normalize(mat3(n0, n1, n2) * barycentrics);
      result.objectColor = starshipColor;
      result.distance = curResult.z;
      result.glowing = false;
    }
  }

  return result;
}

vec3 calcLighting(RaytraceResult x) {
  vec3 intensity;
  if (x.glowing || uInLinearSpace) {
    intensity = vec3(1.0);
  } else {
    // Lambert lighting term
    vec3 lighting = vec3(max(dot(x.normal, uLightDir), 0.0));
    // Test if something lies between the hit point and the light source
    // float shadow = raymarchScene(isec.pos, uLightDir, uNear, uFar).depth >= uFar ? 1.0 : 0.0;
    float shadow = 1.0; // FIXME: proper shadows

    intensity = lighting * shadow;
  }

  // Calculate and return lighting
  return intensity * x.objectColor;
}

/// Takes a ray and background color and returns the color of the point hit.
vec3 raytrace(vec3 rayOrigin, vec3 rayDir, vec3 background_color) {
  RaytraceResult result = RaytraceResult(
    vec3(Inf), // anything can be here
    vec3(Inf), // anything can be here
    background_color,
    uFar,
    true
  );

  // render triangles / oldman
  result = raytraceOldman(rayOrigin, rayDir, result);

  // render procedural stuff
  result = proceduralScene(rayOrigin, rayDir, result);

  // return the final color after lighting calcs
  return calcLighting(result);
}
