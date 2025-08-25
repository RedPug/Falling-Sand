#include "particles/Particle.h"
#include "particles/ParticleBehavior.h"
#include "particles/ParticleType.h"
#include "particles/ParticleFactory.h"
#include "structures/Grid.h"
#include <random>


void Behaviors::gravity(Particle& particle) {
    if(particle.hasChanged) return;

    if(!Grid::isInBounds(particle.x, particle.y + 1)) return;  // Check bounds
    Particle& below = Grid::getParticle(particle.x, particle.y + 1);

    if(below.type_id == ParticleTypeID::EMPTY){
        Grid::swapParticles(particle.x, particle.y, particle.x, particle.y + 1);
    }else if(below.state != MatterState::SOLID) {
        if(particle.density > below.density) {
            Grid::swapParticles(particle.x, particle.y, particle.x, particle.y + 1);
        }
    }
}

void Behaviors::spread(Particle& particle, float min_slope) {
    if(particle.hasChanged) return;

    int max_dx = 3;
    int max_dy = 3;

    if(min_slope <= 0.1f){
        max_dx = 8;
    }else if(min_slope <= 0.2f){
        max_dx = 6;
    }else if(min_slope <= 0.5f){
        max_dx = 4;
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(0, 1);

    int best_dx = 0, best_dy = 0;
    float best_slope = 0.0f;

    for(int i = 0; i <= 1; i++){
        int dx = i ? 1 : -1;
        int dir = i ? 1 : -1;
        int dy = 0;

        while(abs(dx) <= max_dx && abs(dy) <= max_dy) {
            if(Grid::isCellNonSolid(particle.x + dx, particle.y + dy)){
                float current_slope = static_cast<float>(dy) / dx;
                if(current_slope < 0) current_slope = -current_slope;

                if(current_slope >= min_slope){
                    if ((current_slope > best_slope) || (current_slope == best_slope && dist(gen))) {
                        best_slope = current_slope;
                        best_dx = dx;
                        best_dy = dy;
                    }
                }

                if(Grid::isCellNonSolid(particle.x + dx, particle.y + dy+1)){
                    dy++;
                }else{
                    dx += dir;
                }
            }else{
                break;
            }
        }
    }

    if (best_slope >= min_slope && best_dx != 0) {
        int dx = best_dx > 0 ? 1 : -1;
        int dy = 0;

        if(Grid::isCellNonSolid(particle.x + dx, particle.y + 1)){
            dy = 1;
        }

        Grid::swapParticles(particle.x, particle.y, particle.x + dx, particle.y + dy);
    }
}

void Behaviors::spreadLiquid(Particle& particle){
    if(particle.hasChanged) return;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(0, 1);

    int r = dist(gen);

    int min_dx = 0;
    int max_dx = 0;

    for(int i = 0; i <= 1; i++){
        int dir = (i != r) ? 1 : -1;
        for(int j = 1; j <= 3; j++){
            int dx = dir*j;
            if(Grid::isInBounds(particle.x + dx, particle.y) && Grid::isCellEmpty(particle.x + dx, particle.y)) {
                if(dx > 0){
                    max_dx = dx;
                }else{
                    min_dx = dx;
                }
            }else{
                break;  // Stop if we hit a non-empty cell
            }
        }
    }
    if(max_dx > -min_dx){
        Grid::swapParticles(particle.x, particle.y, particle.x + max_dx, particle.y);
    }else if(max_dx < -min_dx){
        Grid::swapParticles(particle.x, particle.y, particle.x + min_dx, particle.y);
    }else{
        if(dist(gen)){
            Grid::swapParticles(particle.x, particle.y, particle.x + max_dx, particle.y);
        }else{
            Grid::swapParticles(particle.x, particle.y, particle.x + min_dx, particle.y);
        }
    }
    
}


//sand touching water
void Behaviors::absorb(Particle& particle) {
    if(particle.hasChanged) return;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dist(1,10);

    if(!Grid::isParticleNearType(particle.x, particle.y, ParticleTypeID::WATER, 1, 1)) {
        // Check all neighbors for water
        return;
    }

    for(int dx = -1; dx <= 1; dx++) {
        for(int dy = -1; dy <= 1; dy++) {
            if(dx == 0 && dy == 0) continue;  // Skip self

            // Random chance to skip some checks
            if(dist(gen) <= 2) {
                continue;
            }
            if(!Grid::isInBounds(particle.x + dx, particle.y + dy)) continue;  // Check bounds
            Particle& neighbor = Grid::getParticle(particle.x + dx, particle.y + dy);
            if(neighbor.type_id == ParticleTypeID::WATER) {

                bool flag = false;

                if(particle.type_id == ParticleTypeID::WET_SAND) {
                    // If it's wet sand, absorb more moisture
                    uint8_t current_moisture = particle.data.wet_sand.moisture;
                    // printf("detected moisture: %d\n", current_moisture);
                    if(current_moisture <= 15){
                        particle.data.wet_sand.moisture += 8;
                        // printf("absorbing water. Moisture was %d, now: %d\n", current_moisture, particle.getData<int>("moisture", 1));
                        flag = true;
                    }
                } else {
                    // Otherwise, turn it into wet sand
                    Particle wet_sand = ParticleFactory::createParticle(ParticleTypeID::WET_SAND);
                    wet_sand.x = particle.x;
                    wet_sand.y = particle.y;

                    particle = wet_sand;

                    flag = true;
                }

                if(flag){
                    Grid::removeParticle(particle.x + dx, particle.y + dy);
                    particle.hasChanged = true;
                    Grid::onParticleUpdate(particle.x, particle.y);
                    return;
                }
            }
        }
    }
}

void Behaviors::spreadWetSand(Particle& particle) {
    if(particle.hasChanged) return;


    int dx_arr[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int dy_arr[] = {-1,  0,  1, -1,1,-1, 0, 1};

    for(int i = 0; i < 8; i++) {
        int dx = dx_arr[i];
        int dy = dy_arr[i];

        if(!Grid::isInBounds(particle.x + dx, particle.y + dy)) continue;

        Particle& neighbor = Grid::getParticle(particle.x + dx, particle.y + dy);
        if(neighbor.type_id == ParticleTypeID::SAND) {
            uint8_t current_moisture = particle.data.wet_sand.moisture;

            if(current_moisture >= 2) {
                particle.data.wet_sand.moisture -= 1;
                particle.hasChanged = true;
                Grid::onParticleUpdate(particle.x, particle.y);
                Grid::setParticle(particle.x + dx, particle.y + dy, ParticleFactory::createParticle(ParticleTypeID::WET_SAND));
                return;
            }
        }else if(neighbor.type_id == ParticleTypeID::WET_SAND){
            uint8_t neighbor_moisture = neighbor.data.wet_sand.moisture;
            uint8_t current_moisture = particle.data.wet_sand.moisture;

            if(current_moisture >= 2 && current_moisture - neighbor_moisture > 1 ) {
                particle.data.wet_sand.moisture -= 1;
                neighbor.data.wet_sand.moisture += 1;
                particle.hasChanged = true;
                neighbor.hasChanged = true;
                Grid::onParticleUpdate(particle.x, particle.y);
                Grid::onParticleUpdate(neighbor.x, neighbor.y);
                return;
            }
        }
    }
}

