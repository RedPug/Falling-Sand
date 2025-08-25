#include "util/Color.h"
#include "particles/ParticleType.h"
#include <unordered_map>
#include <any>
#include <string>
#include <memory>

#ifndef PARTICLE_H
#define PARTICLE_H

struct Particle {
    uint16_t x, y;
    float density;
    bool isQueued;
    bool hasChanged;
    bool received_update = false;
    ParticleTypeID type_id;
    Color color;
    ParticleTypeData data;
    MatterState state = MatterState::NONE;  // Default to none
    

    Particle(Color initial_color = Color(255, 0, 0), ParticleTypeID type = ParticleTypeID::SAND)
        : type_id(type), isQueued(false), hasChanged(false), 
          color(initial_color){
        x = 0;
        y = 0;
        data.raw = 0;
    }

    void onTick(){
        hasChanged = false;
        received_update = false;
    }
    
    void onBlockUpdate(){
        if(hasChanged) return;

        received_update = true;

        ParticleType& type = ParticleTypeRegistry::getType(type_id);
        
        type.executeBehaviors(*this);
    }

    Color getColor() const {
        Color base_color = color;
    
    // Check if this particle has moisture data
    if(type_id == ParticleTypeID::WET_SAND) {
        uint8_t moisture = data.wet_sand.moisture;

        // Darken based on moisture level (0-10 scale)
        float darkness_factor = 1.0f - (moisture * 0.07f);
        darkness_factor = std::max(0.5f, darkness_factor);
        
        base_color.r = static_cast<int>(base_color.r * darkness_factor);
        base_color.g = static_cast<int>(base_color.g * darkness_factor);
        base_color.b = static_cast<int>(base_color.b * darkness_factor);
    }
    
    return base_color;
    }

    

    Particle& operator=(const Particle& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
            isQueued = other.isQueued;
            hasChanged = other.hasChanged;
            type_id = other.type_id;
            state = other.state;
            color = other.color;
            density = other.density;
            received_update = other.received_update;
            data = other.data;  // Use the union assignment
        }
        return *this;
    }

    Particle (const Particle& other) 
        : x(other.x), y(other.y), isQueued(other.isQueued), hasChanged(other.hasChanged),
          type_id(other.type_id), color(other.color), density(other.density), state(other.state), data(other.data){}
};

#endif