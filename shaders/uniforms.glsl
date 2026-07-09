#line 2

const uint MAX_TRIANGLES = 100u;

uniform vec3 uCameraPosition; // deps: raytrace
uniform float uFar; // deps: raytraycing
uniform uint uTriangleCount; // deps: raytraycing
uniform uvec3 uIndices[MAX_TRIANGLES]; // deps: raytraycing
uniform vec3 uLightDir; // deps: procedural
uniform vec4 uVertices[MAX_TRIANGLES * 3u * 2u]; // deps: raytraycing
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
