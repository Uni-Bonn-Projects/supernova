#version 330 core

in vec3 viewDir;
out vec3 fragColor;

#include "procedural.glsl"
#include "raytracing.glsl"
#include "uniforms.glsl"
#include "linear_space.glsl"
#line 11

vec3 renderAt(vec2 ndc, vec2 offset) {
  vec3 rayDir = normalize(mat3(uCameraMatrix) * vec3(ndc.x * uAspectRatio, ndc.y, -uFocalLength)); // Renormalize after interpolation (with crt)
  vec3 rayOrigin = uCameraPosition;

  vec3 focusPoint = uCameraPosition + rayDir * uFocusDistance;

  // shifted focusPoint
  vec3 x = normalize(mat3(uCameraMatrix) * vec3(1.0, 0.0, 0.0));
  vec3 y = normalize(mat3(uCameraMatrix) * vec3(0.0, 1.0, 0.0));
  vec3 shiftedEye = uCameraPosition + x * offset.x + y * offset.y;

  // new ray
  vec3 shiftedRayDir = normalize(focusPoint - shiftedEye);

  // Clear with background
  vec3 color;
  if (uInLinearSpace) {
    color = color_linear_space(rayDir);
  } else {
    color = proceduralSky(rayDir);
  }

  return raytrace(shiftedEye, shiftedRayDir, color);
}

void main() {
  // squared distance from center
  vec2 uv = gl_FragCoord.xy / uResolution;
  vec2 dc = abs(0.5 - uv);
  dc *= dc;
  // warp the fragment coordinates
  uv.x -= 0.5;
  uv.x *= 1.0 + (dc.y * (0.3 * uWarp));
  uv.x += 0.5;
  uv.y -= 0.5;
  uv.y *= 1.0 + (dc.x * (0.4 * uWarp));
  uv.y += 0.5;
  // sample outside boundaries set to black
  if (uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0) {
    fragColor = vec3(0.0);
    return;
  }
  // warp uv from (0.0, 1.0) to (-1.0, 1.0)
  vec2 ndc = uv * 2.0 - 1.0;

  // focus blur
  vec3 color = vec3(0.0);
  // nothing
  if (uFocusSamples <= 1) {
    color = renderAt(ndc, vec2(0.0));
  } else {
    for (int i = 0; i < uFocusSamples; i++) {
      // angles in 360° with 2 pi
      float angle = float(i) / float(uFocusSamples) * 6.28318;
      float radius = uApertureSize * sqrt(float(i) / float(max(uFocusSamples - 1, 1)));
      vec2 offset = vec2(cos(angle), sin(angle)) * radius;

      // ndc with fucus blur offset
      color += renderAt(ndc, offset);
    }
    color /= float(uFocusSamples);
  }

  // determine if we are drawing in a scanline
  float apply = abs(sin(gl_FragCoord.y) * 0.5 * uScan);
  // sample the texture
  fragColor = mix(color, vec3(0.0), apply);
}
