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

      result.hitPos = rayOrigin + curResult.z * rayDir;
      result.normal = normalize(mat3(n0, n1, n2) * barycentrics);
      result.objectColor = starshipColor;
      result.distance = curResult.z;
      result.glowing = false;
    }
  }

  return result;
}

/// Casts a ray from a hit point towards the (directional) light source and
/// tests whether any scene geometry blocks it. Reuses the same intersection
/// routines as the primary ray, so every object that can be hit can also
/// cast a shadow.
bool inShadow(vec3 pos, vec3 normal) {
  // Bias the origin along the surface normal to avoid immediately
  // re-intersecting the surface we just hit (shadow acne). The scene spans
  // scales from a few km (starship/attackers) to hundreds of thousands of km
  // (earth/moon), so the bias is scaled relative to the hit point's distance
  // from the world origin instead of a fixed constant.
  vec3 shadowOrigin = pos + normal * max(1e-3, length(pos) * 1e-6);

  RaytraceResult result = RaytraceResult(
    vec3(Inf),
    vec3(Inf),
    vec3(0.0),
    uFar,
    true
  );

  result = raytraceOldman(shadowOrigin, uLightDir, result);
  result = proceduralScene(shadowOrigin, uLightDir, result);

  return result.distance < uFar;
}

vec3 calcLighting(RaytraceResult x) {
  vec3 intensity;
  if (x.glowing || uInLinearSpace) {
    intensity = vec3(1.0);
  } else {
    // Lambert lighting term
    vec3 lighting = vec3(max(dot(x.normal, uLightDir), 0.0));
    // Test if something lies between the hit point and the light source
    float shadow = inShadow(x.hitPos, x.normal) ? 0.0 : 1.0;

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
