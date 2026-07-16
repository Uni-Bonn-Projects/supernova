#include "common.glsl"
#line 2

layout(std430, binding = 0) readonly buffer MeshData {
  SNMesh umesh;
};
layout(std430, binding = 1) readonly buffer OldmanData {
  Oldman oldman;
};

uniform vec3 uCameraPosition; // deps: raytrace
uniform float uFar; // deps: raytraycing
uniform vec3 uLightDir; // deps: procedural
uniform float uNear = 0.1;

uniform float uFocalLength;
uniform float uTime;
uniform int uSteps;
uniform float uEpsilon;
uniform float uNormalEps;
uniform mat4 uCameraMatrix;
uniform float uAspectRatio;
uniform vec2 uResolution = vec2(800.0, 600.0);
uniform bool uInLinearSpace;
uniform float uWarp = 0.75; // simulate curvature of CRT monitor
uniform float uScan = 0.75; // simulate darkness between scanlines
uniform float uFocusDistance = 500.0;
uniform int uFocusSamples = 1;
uniform float uApertureSize = 0; // size of hole in pinhole camera

uniform vec3 u_oldman_pos;
uniform float u_oldman_active;
uniform float u_oldman_scale;

uniform vec3 u_attacker_pos; // deps: procedural_raytracing
uniform float u_attacker_active; // deps: procedural_raytracing
uniform float u_attacker_scale; // deps: procedural_raytracing

// big blue laser (volumetric glow, no solid hit) - deps: raytracing
uniform bool uLaserActive = false;
uniform vec3 uLaserStart = vec3(0.0);
uniform vec3 uLaserEnd = vec3(0.0);
uniform float uLaserRadius = 0.5;
uniform vec3 uLaserColor = vec3(0.1, 0.4, 1.0); // blue halo
uniform vec3 uLaserCoreColor = vec3(0.8, 0.9, 1.0);
uniform float uLaserGlowRadius = 3.0;
uniform float uLaserGlowIntensity = 1.0;
