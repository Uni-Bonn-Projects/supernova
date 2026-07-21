#pragma once

#include "framework/camera.hpp"
#include "framework/gl/program.hpp"
#include "framework/mesh.hpp"

namespace sn {

struct Particles {
  static const unsigned int AMOUNT = 100'000;
  const float RADIUS = 0.005f;
  // Billboard radius actually used for the most recent burst. Defaults to the
  // tiny point-blank RADIUS but is scaled up by Explosions::spawn() for
  // far-away blasts so they don't render sub-pixel. Shared across concurrent
  // explosions, which is fine here: overlapping bursts in the cinematic are
  // always the same size.
  float current_radius = RADIUS;
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
  // `size` <= 0 keeps the original small point-blank burst; a positive value
  // is the approximate world-space radius of the cloud (billboards are scaled
  // proportionally), so a distant explosion can be made large enough to see.
  void spawn(const glm::vec3 &center, float duration, float size = 0.0f);
  bool isActive(void);

private:
  Program _program;
  Mesh _mesh;
  GLuint _offsetVBO;
  Particles _particles;
};

}; // namespace sn
