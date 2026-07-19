#include "common.glsl"
#include "csg.glsl"
#include "math.glsl"
#include "procedural_raytracing.glsl"
#include "uniforms.glsl"
#line 6

/// We are only using triangles to render OLDMAN. Therefore, this function
/// raytraces all triangle stuff with some values, like color, hardcoded for
/// OLDMAN.
RaytraceResult raytraceOldman(
  vec3 rayOrigin,
  vec3 rayDir,
  RaytraceResult result
) {
  CSGInterval topDome = CSGSub(
      calcCSGInterval(rayOrigin, rayDir, 0u),
      calcCSGInterval(rayOrigin, rayDir, 1u)
    );
  CSGInterval mainBody = CSGOr(
      topDome,
      calcCSGInterval(rayOrigin, rayDir, 2u)
    );

  // attach sections
  CSGInterval fullBody = mainBody;
  for (uint i = 0; i < 12; i++) {
    fullBody = CSGOr(
        fullBody,
        calcCSGInterval(rayOrigin, rayDir, i + 3u)
      );
  }

  result = renderCSGInterval(rayOrigin, rayDir, result, fullBody);

  return result;
}

/// Casts a ray from a hit point towards the (directional) light source and
/// tests whether any scene geometry blocks it. Reuses the same intersection
/// routines as the primary ray, so every object that can be hit can also
/// cast a shadow.
bool inShadow(vec3 pos, vec3 normal) {
  // Bias the origin along the surface normal to avoid immediately
  // re-intersecting the surface we just hit.
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
/// Also writes the distance to that hit to hitDistance
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

/// Purely volumetric laser halo, there is no solid laser
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
    ? dot((uLaserStart + uLaserEnd) * 0.5 - ro, rd) / A : (B * F - C * E) / denom;

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
