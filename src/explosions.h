#pragma once

#include "framework/camera.hpp"
#include "framework/gl/program.hpp"
#include "framework/mesh.hpp"

struct Particles;

struct Particles {
  static const unsigned int AMOUNT = 100'000;
  const float RADIUS = 0.005f;
  const float _INIT_LIFE = 5.0f; // initial time a particle lives
  unsigned int _last_unused = 0; // the last return value of get_unused

  vec3 pos[AMOUNT];
  vec3 vel[AMOUNT];
  float life[AMOUNT];

  Particles();

  /** Returns the index of an unused particle. (The dead ones)
   * If none could be found, the first one is returned.
   */
  unsigned int get_unused(void);

  /** Add a new particle.
   * If the particle cap (Particle.AMOUNT) is reached, the first particle is
   * overwritten.
   */
  void add(vec3 particle_pos, vec3 particle_vel);
};

class Explosions {
public:
  void init(void);
  void render(Camera &camera, float delta_time, bool camera_changed);
  void spawn(const glm::vec3 &center);

private:
  void _updateParticles(float dt);

  Program _program;
  Mesh _mesh;
  GLuint _offsetVBO;
  Particles _particles;
};
