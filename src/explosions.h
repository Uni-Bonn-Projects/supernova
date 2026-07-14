#pragma once

#include "framework/camera.hpp"
#include "framework/gl/program.hpp"
#include "framework/mesh.hpp"

struct Particles;

struct Particles {
  static const unsigned int AMOUNT = 100'000;
  const float RADIUS = 0.005f;
  unsigned int _last_unused = 0; // the last return value of get_unused

  vec3 pos[AMOUNT];
  vec3 vel[AMOUNT];
  float max_duration[AMOUNT];
  float alive_prob[AMOUNT];

  /** Returns the index of an unused particle. (The dead ones)
   * If none could be found, the first one is returned.
   */
  unsigned int get_unused(void);

  /** Add a new particle.
   * If the particle cap (Particle.AMOUNT) is reached, the first particle is
   * overwritten.
   */
  void add(vec3 particle_pos, vec3 particle_vel, float particle_duration);

  /** Update all particles. */
  void update(float delta_time);
};

class Explosions {
public:
  void init(void);
  void render(Camera &camera, float delta_time, bool camera_changed);
  void spawn(const glm::vec3 &center, float duration);
  bool isActive(void);

private:
  Program _program;
  Mesh _mesh;
  GLuint _offsetVBO;
  Particles _particles;
};
