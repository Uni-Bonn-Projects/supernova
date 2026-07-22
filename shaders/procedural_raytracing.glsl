#include "common.glsl"
#include "math.glsl"
#include "uniforms.glsl"
#line 5

// everything in kilometers
const float attackerRadius = 2.5 / 2.0;
const float moonRadius = 3474.0 / 2.0;
const float earthRadius = 12756.0 / 2.0;
const float earthMoonDist = 384400.0;
const int attackerAmount = 13;
const float attacker_distance_val = 400.0;

// Spreads the swarm around oldman by rotating each attacker's own anchor
// around a fixed pivot. Index 6 = 0 because POV shot was implemented before
vec3 attackerSwarmAnchor(int i, vec3 origin) {
  float angleDeg = float(i - 6) * (360.0 / float(attackerAmount));
  vec3 relativeToPivot = origin - uAttackerSwarmPivot;
  return uAttackerSwarmPivot + rotateY(relativeToPivot, degToRad(angleDeg));
}

bool attackerAlive(int i) { return (uAttackerAliveMask & (1 << i)) != 0; }

vec3 attackerSpherePosition(int i, vec3 origin) {
  float y = 1.0 - (float(i) / float(attackerAmount - 1)) * 2.0;
  float radius_at_y = sqrt(1.0 - y * y);
  float theta = float(i) * GOLDEN_ANGLE;
  float x = cos(theta) * radius_at_y;
  float z = sin(theta) * radius_at_y;
  return attackerSwarmAnchor(i, origin) + vec3(x, y, z) * attacker_distance_val;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// BACKGROUND //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

vec3 proceduralStars(vec3 rayDir) {
  const int star_amount = 1000; // not the actual amount

  vec3 cell = floor(rayDir * star_amount);
  float h = hash(cell);

  if (h > 0.999) {
    // Random brightness
    float brightness = pow(hash(vec3(h)), 4.0);
    return vec3(brightness);
  } else {
    return vec3(0.0);
  }
}

vec3 proceduralSun(vec3 rayDir) {
  return vec3(pow(max(0.0, dot(rayDir, uLightDir)), uSunSize));
}

vec3 proceduralSky(vec3 rayDir) {
  return proceduralSun(rayDir) + proceduralStars(rayDir);
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// OBJECTS ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// Calculate the distance to the given procedual Sphere
///
/// It is assumed that rayDir is normalized.
float proceduralSphere(
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
    return Inf;
  }

  // left and right side of the quadratic formula (seperator is the +-)
  float lhs = -h;
  float rhs = sqrt(sqrt_inner);

  // use x1 if is infront of the camera
  // use x2 else
  float x = lhs - rhs;
  if (x < 0.0) {
    x = lhs + rhs;
  }

  // return the selected x if it is in front of the camera
  // no result else
  // (both x's can be behind the camera)
  return (x >= 0) ? x : Inf;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// SCENE /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// Performs raytracing on all of the procedural geometry
RaytraceResult proceduralScene(
  vec3 rayOrigin,
  vec3 rayDir,
  RaytraceResult result
) {
  // if (u_attacker_active > 0.5) {
  if (true) {
    vec3 attacker_origin = u_attacker_pos;

    for (int i = 0; i < attackerAmount; i++) {
      // destroyed attackers stop rendering (and stop casting shadows)
      if (!attackerAlive(i)) {
        continue;
      }
      vec3 attackerPos = attackerSpherePosition(i, attacker_origin);
      float distance = proceduralSphere(
          rayOrigin,
          rayDir,
          attackerPos,
          attackerRadius * u_attacker_scale
        );
      if (distance < result.distance) {
        result.hitPos = rayOrigin + distance * rayDir;
        result.normal = normalize(result.hitPos - attackerPos);
        result.objectColor = starshipColor;
        result.distance = distance;
        result.glowing = false;
      }
    }
  }

  // moon (gated so it can be made to disappear; moonPos stays declared
  // because earth is positioned relative to it below)
  vec3 moonPos = xInDir(10000, vec3(-1, -1, -1));
  if (uMoonActive) {
    float distance = proceduralSphere(rayOrigin, rayDir, moonPos, moonRadius);
    if (distance < result.distance) {
      result.hitPos = rayOrigin + distance * rayDir;
      result.normal = normalize(result.hitPos - moonPos);
      result.objectColor = moonColor;
      result.distance = distance;
      result.glowing = false;
    }
  }

  // earth
  vec3 earthPos = moonPos + xInDir(earthMoonDist, vec3(0.2, 0.3, 1));
  {
    float distance = proceduralSphere(rayOrigin, rayDir, earthPos, earthRadius);
    if (distance < result.distance) {
      result.hitPos = rayOrigin + distance * rayDir;
      result.normal = normalize(result.hitPos - earthPos);
      result.objectColor = earthColor;
      result.distance = distance;
      result.glowing = false;
    }
  }

  return result;
}
