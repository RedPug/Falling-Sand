#include <functional>

#ifndef PARTICLE_BEHAVIOR_H
#define PARTICLE_BEHAVIOR_H

// Forward declaration to avoid circular include
class Particle;

namespace Behaviors{
    void gravity(Particle& particle);
    void spread(Particle& particle, float slope);
    void spreadLiquid(Particle& particle);
    void absorb(Particle& particle);
    void spreadWetSand(Particle& particle);
}

#endif // PARTICLE_BEHAVIOR_H