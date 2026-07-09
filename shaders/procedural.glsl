#include "math.glsl"
#include "uniforms.glsl"

vec3 proceduralSun(vec3 rayDir) {
  return vec3(pow(max(0.0, dot(rayDir, uLightDir)), 1000));
}

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
  float x = lhs + rhs;
  if (x < 0.0) {
    x = lhs - rhs;
  }

  // return the selected x if it is in front of the camera
  // no result else
  // (both x's can be behind the camera)
  return (x >= 0) ? x : Inf;
}
