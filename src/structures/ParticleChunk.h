#include <SDL3/SDL_render.h>
#include <array>
#include "particles/ParticleType.h"


#ifndef PARTICLE_CHUNK_H
#define PARTICLE_CHUNK_H

struct ParticleChunk{
    SDL_Texture* texture = nullptr;
    mutable bool dirty = true;
    mutable bool type_data_valid = false;
    mutable bool shouldProcessNextFrame = false;
    mutable bool shouldProcess = false;

    int x, y;
    static const int CHUNK_SIZE = 32;  // Size of each chunk in grid cells

    mutable uint32_t type_bitmask = 0;

    bool hasParticleType(ParticleTypeID type) const;

    void rebuildTypeData();
};

#endif