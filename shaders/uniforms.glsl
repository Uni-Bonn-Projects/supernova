const uint MAX_TRIANGLES = 100u;

uniform vec3 uCameraPosition; // deps: raytrace
uniform float uFar; // deps: raytraycing
uniform uint uTriangleCount; // deps: raytraycing
uniform uvec3 uIndices[MAX_TRIANGLES]; // deps: raytraycing
uniform vec3 uLightDir; // deps: procedural
uniform vec4 uVertices[MAX_TRIANGLES * 3u * 2u]; // deps: raytraycing
uniform float uNear = 0.1;
