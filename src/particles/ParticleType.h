#include <vector>
#include "util/Color.h"
#include "particles/ParticleBehavior.h"
#include <functional>
#include <array>

#ifndef PARTICLE_TYPE_H
#define PARTICLE_TYPE_H

//avoid importing recursion
class Particle;

enum ParticleTypeID {
    EMPTY = 0,
    SAND = 1,
    WATER = 2,
    STONE = 3,
    WET_SAND = 4,
};

enum MatterState{
    NONE,
    SOLID,
    LIQUID,
    GAS,
};

const int NUM_PARTICLE_TYPES = 5;  // Update this if you add more particle types

union ParticleTypeData {
    struct { uint8_t moisture; } wet_sand;      // 1 byte
    struct { uint32_t velocity;} water;
    uint64_t raw;                               // 8 bytes
};

struct ParticleType {
    float base_density;
    std::vector<Color> color_palette;
    std::vector<int> color_weights;

    std::function<void(Particle&)> executeBehaviors;

    MatterState state;

    ParticleType(){
        executeBehaviors = [](Particle& p) {};
    }

    ParticleType(float density, MatterState state, std::vector<Color> colors, std::vector<int> weights, std::function<void(Particle&)> behaviors)
        : base_density(density), color_palette(colors), color_weights(weights), executeBehaviors(behaviors), state(state) {}
};



class ParticleTypeRegistry {
    inline static std::array<ParticleType, NUM_PARTICLE_TYPES> types;

public:
    static ParticleType& getType(ParticleTypeID id) {
        return types[static_cast<size_t>(id)];
    }
    
    inline static void initialize() {
        types[EMPTY] = ParticleType();
        
        types[SAND] = ParticleType(1.6f, MatterState::SOLID,
            {Color(255, 204, 102),
            Color(204, 153, 0),
            Color(204, 102, 0),
            Color(153, 102, 51)},
            {80, 10, 8, 2},
            [](Particle& p) {
                Behaviors::gravity(p);
                Behaviors::spread(p, 0.6f);
                // Behaviors::absorb(p);
            });

        // Wet Sand type
        types[WET_SAND] = ParticleType(1.8f, MatterState::SOLID,
            {//Color(255, 204, 102),
                Color(255,0,0),
            Color(204, 153, 0),
            Color(204, 102, 0),
            Color(153, 102, 51)},
            {80, 10, 8, 2},
            [](Particle& p) {
            Behaviors::gravity(p);
            Behaviors::spread(p, 2.0f);
            // Behaviors::absorb(p);
            // Behaviors::spreadWetSand(p);
            });

        //Water type
        types[WATER] = ParticleType(1.0f, MatterState::LIQUID,
            {Color(51,51,255), Color(0,0,153), Color(0,51,204), Color(102,102,255)},
            {80, 10, 8, 2},
            [](Particle& p) {
                
                
                Behaviors::gravity(p);
                Behaviors::spreadLiquid(p);
            });
        
        // Stone type
        types[STONE] = ParticleType(3.0f, MatterState::SOLID, {Color(128,128,128)}, {100},
            [](Particle& p) {
            });
    }
};

#endif