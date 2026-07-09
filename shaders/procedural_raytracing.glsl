#include "colors.glsl"
#include "math.glsl"
#include "uniforms.glsl"
#line 5

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
  return vec3(pow(max(0.0, dot(rayDir, uLightDir)), 1000));
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

vec3 sphereColor(
  vec3 rayOrigin,
  vec3 rayDir,
  vec3 spherePos,
  float distance,
  vec3 albedo
) {
  vec3 intensity;
  if (uInLinearSpace) {
    intensity = vec3(1.0);
  } else {
    vec3 hitPos = rayOrigin + distance * rayDir;
    vec3 normal = normalize(hitPos - spherePos);

    // Lambert lighting term
    vec3 lighting = vec3(max(dot(normal, uLightDir), 0.0));

    // Test if something lies between the hit point and the light source
    // float shadow = raymarchScene(isec.pos, uLightDir, uNear, uFar).depth >= uFar ? 1.0 : 0.0;
    float shadow = 1.0; // FIXME: proper shadows

    intensity = lighting * shadow;
  }

  // Calculate lighting
  return intensity * albedo;
}

/// Performs raytracing on all of the procedural geometry
///
/// Returned will be a "tuple": (depth, color)
vec4 proceduralScene(
  vec3 rayOrigin,
  vec3 rayDir,
  float current_depth,
  vec3 background_color
) {
  float depth = current_depth;
  vec3 color = background_color;

  // procedual stuff
  // (only a shere for now)
  vec3 spherePos = vec3(60, 20, 20);
  float sphereRadius = 10;

  float distance = proceduralSphere(rayOrigin, rayDir, spherePos, sphereRadius);
  if (distance < depth) {
    depth = distance;
    color = sphereColor(rayOrigin, rayDir, spherePos, distance, moonColor);
  }

  return vec4(depth, color);
}
