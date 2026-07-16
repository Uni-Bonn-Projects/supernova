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
/// Also writes the distance to that hit (or uFar if nothing was hit) to
/// hitDistance, so callers can occlude volumetric effects (e.g. the laser
/// glow) behind solid geometry.
vec3 raytrace(vec3 rayOrigin, vec3 rayDir, vec3 background_color, out float hitDistance) {
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

  hitDistance = result.distance;

  // return the final color after lighting calcs
  return calcLighting(result);
}

/// Purely volumetric laser halo - there is no solid hit for the laser, this
/// is the only place it gets drawn. Returns (color, alpha): alpha is 1.0 on
/// the beam axis and falls off exponentially with distance, so the caller
/// does a plain alpha blend (mix) instead of adding light. Marching is
/// windowed around the closest point on the ray to the beam axis so we don't
/// waste steps sampling empty space far from the laser.
vec4 laserGlow(vec3 ro, vec3 rd, float maxDepth) {
  if (!uLaserActive) return vec4(0.0);

  vec3 ba = uLaserEnd - uLaserStart;
  vec3 r = ro - uLaserStart;
  float A = dot(rd, rd);
  float B = dot(rd, ba);
  float E = dot(ba, ba);
  float C = dot(rd, r);
  float F = dot(ba, r);
  float denom = A * E - B * B;
  float tc = (abs(denom) < 1e-6)
      ? dot((uLaserStart + uLaserEnd) * 0.5 - ro, rd) / A
      : (B * F - C * E) / denom;

  float W = uLaserGlowRadius * 4.0; // half-window
  float t0 = max(uNear, tc - W);
  float t1 = min(maxDepth, tc + W);
  if (t1 <= t0) return vec4(0.0);

  const int GLOW_STEPS = 32;
  float dt = (t1 - t0) / float(GLOW_STEPS);
  vec3 accumColor = vec3(0.0);
  float accumAlpha = 0.0;
  for (int i = 0; i < GLOW_STEPS; i++) {
    vec3 p = ro + rd * (t0 + dt * (float(i) + 0.5));
    float d = max(distToAxis(p, uLaserStart, uLaserEnd) - uLaserRadius, 0.0);
    // alpha = 1 on the axis (d = 0), exponential falloff with distance
    float sampleAlpha = exp(-(d * d) / (uLaserGlowRadius * uLaserGlowRadius));
    // white-hot near the axis, fading to the blue halo color outward
    vec3 sampleColor = mix(uLaserCoreColor, uLaserColor,
        clamp(d / uLaserGlowRadius, 0.0, 1.0));

    // front-to-back "over" compositing along the march window
    float contribution = sampleAlpha * (1.0 - accumAlpha);
    accumColor += sampleColor * contribution;
    accumAlpha += contribution;
  }
  return vec4(accumColor, clamp(accumAlpha * uLaserGlowIntensity, 0.0, 1.0));
}
