#include "common.glsl"
#line 2

layout(std430, binding = 0) readonly buffer OldmanData {
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

// Per-attacker life state: bit i set == attacker i is alive. One int instead
// of a bool array so it's a single uniform set. 0x1FFF == all 13 alive.
uniform int uAttackerAliveMask = 0x1FFF; // deps: procedural_raytracing
// Pivot the swarm formation is rotated around
uniform vec3 uAttackerSwarmPivot = vec3(0.0); // deps: procedural_raytracing
// Moon on/off; can be toggled to make the moon vanish - deps: procedural_raytracing
uniform bool uMoonActive = true;

// attacker swarm fire (small red/orange lasers converging on oldman) - deps: raytracing
uniform bool uAttackerLaserActive = false;
uniform vec3 uAttackerLaserTarget = vec3(0.0); // oldman's center, may move
uniform float uAttackerLaserRadius = 5.0;
uniform vec3 uAttackerLaserColor = vec3(0.9, 0.2, 0.05); // red/orange halo
uniform vec3 uAttackerLaserCoreColor = vec3(1.0, 0.8, 0.3);
uniform float uAttackerLaserGlowRadius = 3.0;
uniform float uAttackerLaserGlowIntensity = 1.0;

// oldman's return fire: up to 3 small green beams, each aimed at one attacker
// sphere by index (-1 == that slot is unused) - deps: raytracing
uniform bool uOldmanBeamActive = false;
uniform vec3 uOldmanBeamStart = vec3(0.0); // oldman's muzzle, may move
uniform ivec3 uOldmanBeamTargets = ivec3(-1);
uniform float uOldmanBeamRadius = 1.5;
uniform vec3 uOldmanBeamColor = vec3(0.1, 1.0, 0.2); // green halo
uniform vec3 uOldmanBeamCoreColor = vec3(0.8, 1.0, 0.85);
uniform float uOldmanBeamGlowRadius = 1.0;
uniform float uOldmanBeamGlowIntensity = 1.0;

// size of sun for supernova
uniform float uSunSize = 1000.0;