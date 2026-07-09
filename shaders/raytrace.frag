#version 330 core

in vec3 viewDir;
out vec3 fragColor;

#include "procedural.glsl"
#include "raytracing.glsl"
#include "uniforms.glsl"

void main() {
  // Generate camera ray
  vec3 rayDir = normalize(viewDir); // Renormalize after interpolation
  vec3 rayOrigin = uCameraPosition;

  // Clear with background
  fragColor = proceduralSun(rayDir);

  fragColor = raytrace(rayOrigin, rayDir, fragColor);
}
