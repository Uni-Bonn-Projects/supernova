#version 330 core

in vec3 viewDir;
out vec3 fragColor;

uniform vec3 uCameraPosition;
uniform float uFocalLength;
uniform float uTime;

uniform vec3 uLightDir;
uniform float uNear;
uniform float uFar;
uniform int uSteps;
uniform float uEpsilon;
uniform float uNormalEps;
uniform mat4 uCameraMatrix;
uniform float uAspectRatio;
uniform vec2 uResolution = vec2(800.0, 600.0);
uniform bool uInLinearSpace;

uniform float uWarp = 0.75; // simulate curvature of CRT monitor
uniform float uScan = 0.75; // simulate darkness between scanlines

const vec3 uLightColor = vec3(1.0);

// colors
const vec3 starshipColor = vec3(0.4, 0.4, 0.4);
const vec3 moonColor = vec3(0.96, 0.91, 0.77);
const vec3 sunColor = vec3(0.98, 0.81, 0.32);
const vec3 earthColor = vec3(0.0, 0.0, 0.62);

const int SKY_ID = 0;
const int STARSHIP_ID = 1;
const int MOON_ID = 3;
const int SUN_ID = 4;
const int EARTH_ID = 5;

#include "scene_construction.glsl"
#include "sdf.glsl"
#include "math.glsl"
#include "linear_space.glsl"

struct Intersection {
  float depth; // Current depth on the ray
  vec3 pos; // Current position on the ray
  int ID; // ID of the closest object
  bool glowing; // whether or not the closest object "glows"
  int steps; // Number of steps taken
};

vec3 proceduralSky(vec3 rayDir) {
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
  return pow(max(0.0, dot(rayDir, uLightDir)), 1000) * uLightColor;
}

/* Returns the color of the object ID at pos */
vec3 colorScene(vec3 pos, int ID) {
  switch (ID) {
    case STARSHIP_ID:
    return starshipColor;
    case MOON_ID:
    return moonColor;
    case SUN_ID:
    return sunColor;
    case EARTH_ID:
    return earthColor;
    default:
    return vec3(1.0, 0.0, 0.0);
  }
}

/* Returns the normalized gradient of the signed distance field of our scene */
vec3 normalScene(vec3 pos) {
  vec2 h = vec2(uNormalEps, 0);
  return normalize(vec3(
      sdScene(pos + h.xyy).dist - sdScene(pos - h.xyy).dist,
      sdScene(pos + h.yxy).dist - sdScene(pos - h.yxy).dist,
      sdScene(pos + h.yyx).dist - sdScene(pos - h.yyx).dist
    ));
}

Intersection raymarchScene(vec3 rayOrigin, vec3 rayDir, float near, float far) {
  Intersection intsec = Intersection(
      0,
      rayOrigin,
      SKY_ID,
      true,
      0
    );

  float next_depth = near;

  while (
    next_depth > uEpsilon
      && intsec.depth < far
      && intsec.steps < uSteps
  ) {
    intsec.depth += next_depth;
    intsec.pos += rayDir * next_depth;

    SD sd = sdScene(intsec.pos);
    next_depth = sd.dist;
    intsec.ID = sd.ID;
    intsec.glowing = sd.glowing;

    intsec.steps += 1;
  }

  return intsec;
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

  vec3 rayDir = normalize(viewDir); // Renormalize after interpolation
  vec3 rayOrigin = uCameraPosition;

  Intersection isec = raymarchScene(rayOrigin, rayDir, uNear, uFar);

  vec3 color;
  if (isec.depth >= uFar) {

    // No hit, render a procedural background
    if (uInLinearSpace) {
      color = color_linear_space(rayDir);
    } else {
      color = proceduralSky(rayDir) + proceduralSun(rayDir);
    }
  } else {
    // We hit something

    vec3 intensity;
    if (isec.glowing || uInLinearSpace) {
      intensity = vec3(1.0);
    } else {
      // The normal is the normalized gradient of the signed distance field
      vec3 normal = normalScene(isec.pos);
      // Lambert lighting term
      vec3 lighting = max(dot(normal, uLightDir), 0.0) * uLightColor;
      // Test if something lies between the hit point and the light source
      float shadow = raymarchScene(isec.pos, uLightDir, uNear, uFar).depth >= uFar ? 1.0 : 0.0;

      intensity = lighting * shadow;
    }

    // Get the color of the hit point
    vec3 albedo = colorScene(isec.pos, isec.ID);

    // Calculate lighting
    color = intensity * albedo;
  }
  // determine if we are drawing in a scanline
  float apply = abs(sin(gl_FragCoord.y) * 0.5 * uScan);
  // sample the texture
  fragColor = mix(color, vec3(0.0), apply);
}

// end: render
