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

// Purely volumetricc laser halo
vec4 laserSegmentGlow(vec3 ro, vec3 rd, float maxDepth, vec3 start, vec3 end,
                       float radius, vec3 color, vec3 coreColor,
                       float glowRadius, float glowIntensity, int steps) {
  vec3 ba = end - start;
  vec3 r = ro - start;
  float A = dot(rd, rd);
  float B = dot(rd, ba);
  float E = dot(ba, ba);
  float C = dot(rd, r);
  float F = dot(ba, r);
  float denom = A * E - B * B;
  float tc = (abs(denom) < 1e-6)
    ? dot((start + end) * 0.5 - ro, rd) / A : (B * F - C * E) / denom;

  float W = glowRadius * 4.0; // half-window
  float t0 = max(uNear, tc - W);
  float t1 = min(maxDepth, tc + W);
  if (t1 <= t0) return vec4(0.0);

  float dt = (t1 - t0) / float(steps);
  vec3 accumColor = vec3(0.0);
  float accumAlpha = 0.0;
  for (int i = 0; i < steps; i++) {
    vec3 p = ro + rd * (t0 + dt * (float(i) + 0.5));
    float d = max(distToAxis(p, start, end) - radius, 0.0);
    // alpha = 1 on the axis (d = 0), exponential falloff with distance
    float sampleAlpha = exp(-(d * d) / (glowRadius * glowRadius));
    // white-hot near the axis, fading to the halo color outward
    vec3 sampleColor = mix(coreColor, color, clamp(d / glowRadius, 0.0, 1.0));

    // front-to-back "over" compositing along the march window
    float contribution = sampleAlpha * (1.0 - accumAlpha);
    accumColor += sampleColor * contribution;
    accumAlpha += contribution;
  }
  return vec4(accumColor, clamp(accumAlpha * glowIntensity, 0.0, 1.0));
}

vec4 laserGlow(vec3 ro, vec3 rd, float maxDepth) {
  if (!uLaserActive) return vec4(0.0);
  return laserSegmentGlow(ro, rd, maxDepth, uLaserStart, uLaserEnd,
                           uLaserRadius, uLaserColor, uLaserCoreColor,
                           uLaserGlowRadius, uLaserGlowIntensity, 32);
}

const float oldmanHitRadius = 90.0;

// so that not every attacker beam shoots at the same poition
vec3 oldmanHitPoint(int i) {
  float y = 1.0 - (float(i) / float(attackerAmount - 1)) * 2.0;
  float radius_at_y = sqrt(1.0 - y * y);
  float theta = float(i) * GOLDEN_ANGLE + PI;
  float x = cos(theta) * radius_at_y;
  float z = sin(theta) * radius_at_y;
  return uAttackerLaserTarget + vec3(x, y, z) * oldmanHitRadius;
}


vec4 attackerLaserGlow(vec3 ro, vec3 rd, float maxDepth) {
  if (!uAttackerLaserActive) return vec4(0.0);

  vec4 accum = vec4(0.0);
  for (int i = 0; i < attackerAmount; i++) {
    // destroyed attackers stop shooting
    if (!attackerAlive(i)) {
      continue;
    }
    vec3 beamStart = attackerSpherePosition(i, u_attacker_pos);
    vec4 beam = laserSegmentGlow(ro, rd, maxDepth, beamStart,
        oldmanHitPoint(i), uAttackerLaserRadius, uAttackerLaserColor,
        uAttackerLaserCoreColor, uAttackerLaserGlowRadius,
        uAttackerLaserGlowIntensity, 16);
    accum.rgb += beam.rgb * beam.a * (1.0 - accum.a);
    accum.a += beam.a * (1.0 - accum.a);
  }
  return accum;
}

/// Oldman firing back: up to 3 green beams, each locked onto one attacker sphere. 
vec4 oldmanBeamGlow(vec3 ro, vec3 rd, float maxDepth) {
  if (!uOldmanBeamActive) return vec4(0.0);

  vec4 accum = vec4(0.0);
  for (int slot = 0; slot < 3; slot++) {
    int target = uOldmanBeamTargets[slot];
    if (target < 0 || target >= attackerAmount || !attackerAlive(target)) {
      continue;
    }
    vec3 beamEnd = attackerSpherePosition(target, u_attacker_pos);
    vec4 beam = laserSegmentGlow(ro, rd, maxDepth, uOldmanBeamStart,
        beamEnd, uOldmanBeamRadius, uOldmanBeamColor,
        uOldmanBeamCoreColor, uOldmanBeamGlowRadius,
        uOldmanBeamGlowIntensity, 16);
    accum.rgb += beam.rgb * beam.a * (1.0 - accum.a);
    accum.a += beam.a * (1.0 - accum.a);
  }
  return accum;
}
