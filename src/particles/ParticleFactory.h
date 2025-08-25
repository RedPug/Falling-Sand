#include <random>
#include "particles/Particle.h"
#include "particles/ParticleBehavior.h"
#include "particles/ParticleType.h"
#include "util/Color.h"


#ifndef PARTICLE_FACTORY_H
#define PARTICLE_FACTORY_H

namespace ParticleFactory {
    inline Particle createParticle(ParticleTypeID type_id) {
        ParticleType& type = ParticleTypeRegistry::getType(type_id);
        auto palette = type.color_palette.data();
        auto size = type.color_palette.size();
        auto weights = type.color_weights.data();
        Color color = Color::fromSelection(palette, size, weights);
        Particle particle = Particle(color, type_id);
        particle.state = type.state;
        particle.density = type.base_density;

        // if(type_id == ParticleTypeID::SAND) {
        //     printf("Creating particle of type %d with state %d\n", 
        //       static_cast<int>(type_id), static_cast<int>(particle.state));
        // }
        

        return particle;
    }
}

#endif