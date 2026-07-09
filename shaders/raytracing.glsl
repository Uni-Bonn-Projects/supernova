#include "colors.glsl"
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

  if (u > 0 && v > 0 && u + v < 1) {
    return vec3(u, v, t);
  } else {
    return vec3(Inf);
  }
}

/// We are only using triangles to render OLDMAN. Therefore, this function
/// raytraces all triangle stuff with some values, like color, hardcoded for
/// OLDMAN.
///
/// Returned will be a "tuple": (depth, color)
vec4 raytraceOldman(
  vec3 rayOrigin,
  vec3 rayDir,
  float current_depth,
  vec3 background_color
) {
  float depth = current_depth;
  vec3 color = background_color;

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

      vec3 intensity;
      if (uInLinearSpace) {
        intensity = vec3(1.0);
      } else {
        // Interpolate normals
        vec3 normal = normalize(mat3(n0, n1, n2) * barycentrics);
        // Lambert lighting term
        vec3 lighting = vec3(max(dot(normal, uLightDir), 0.0));
        // Test if something lies between the hit point and the light source
        // float shadow = raymarchScene(isec.pos, uLightDir, uNear, uFar).depth >= uFar ? 1.0 : 0.0;
        float shadow = 1.0; // FIXME: proper shadows

        intensity = lighting * shadow;
      }

      // Calculate lighting
      color = intensity * starshipColor;
    }
  }

  return vec4(depth, color);
}

/// Takes a ray and background color and returns the color of the point hit.
vec3 raytrace(vec3 rayOrigin, vec3 rayDir, vec3 background_color) {
  vec3 color = background_color;
  float depth = uFar;

  // render triangles / oldman
  {
    vec4 result = raytraceOldman(rayOrigin, rayDir, depth, color);
    depth = result.x;
    color = result.yzw;
  }

  // render procedural stuff
  {
    vec4 result = proceduralScene(rayOrigin, rayDir, depth, color);
    depth = result.x;
    color = result.yzw;
  }

  return color;
}
