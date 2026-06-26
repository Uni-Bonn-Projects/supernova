#version 330 core

layout(location = 0) in vec3 _position;

out vec3 viewDir;

uniform mat4 uCameraMatrix;
uniform float uAspectRatio;
uniform float uFocalLength;

/**
 * Calculates the ray direction for every vertex, which is then hardware
 * interpolated to calculate the per pixel ray direction
 */
void main() {
  gl_Position = vec4(_position, 1.0);
  viewDir = mat3(uCameraMatrix) * vec3(_position.x * uAspectRatio, _position.y, -uFocalLength);
}
