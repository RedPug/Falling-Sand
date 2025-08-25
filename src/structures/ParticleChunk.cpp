#include "structures/ParticleChunk.h"
#include "structures/Grid.h"


bool ParticleChunk::hasParticleType(ParticleTypeID type) const {
    return type_bitmask & (1 << static_cast<int>(type));
}

void ParticleChunk::rebuildTypeData() {

    type_bitmask = 0;
    
    // Scan this chunk's particles, plus a border of 1 cell.
    int start_x = std::max(x * CHUNK_SIZE - 1, 0);
    int start_y = std::max(y * CHUNK_SIZE - 1, 0);
    int end_x = std::min(start_x + CHUNK_SIZE + 1, Grid::width);
    int end_y = std::min(start_y + CHUNK_SIZE + 1, Grid::height);

    for(int py = start_y; py < end_y; py++) {
        for(int px = start_x; px < end_x; px++) {
            ParticleTypeID type = Grid::getParticle(px, py).type_id;
            type_bitmask |= (1 << static_cast<int>(type));
        }
    }
    
    type_data_valid = true;
}