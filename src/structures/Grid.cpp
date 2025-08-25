#include "structures/Grid.h"
#include "particles/Particle.h"
#include "particles/ParticleFactory.h"
#include "structures/ParticleChunk.h"
#include <vector>
#include <shared_mutex>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <stdexcept>


Particle* Grid::particles = nullptr;
std::vector<Particle*> Grid::processing_queue;
ThreadGroup<ProcessingChunk> Grid::processing_threads;
ProcessingChunk* Grid::processing_chunks = nullptr;

int Grid::width = 0;
int Grid::height = 0;
int Grid::num_particles = 0;
int Grid::num_threads = 0;
int Grid::num_particle_chunks_x = 0;
int Grid::num_particle_chunks_y = 0;

std::vector<ParticleChunk> Grid::particleChunks;

void Grid::processingTask(ProcessingChunk chunk, int thread_id) {
    for(int y = chunk.y + chunk.height - 1; y >= chunk.y; y--) {
        for(int dx = 0; dx < chunk.width; dx++) {
            int x;
            if(chunk.is_flipped != (y % 2 == 0)) {
                x = chunk.x + (chunk.width - 1 - dx);
            }else{
                x = chunk.x + dx;
            }

            int i = y * width + x;
            if(i < 0 || i >= num_particles) continue;  // Out of bounds check
            particles[i].onBlockUpdate();
        }
    }
}

void Grid::init(int w, int h) {
    width = w;
    height = h;
    num_particles = width * height;

    particles = new Particle[num_particles];

    for(int i = 0; i < num_particles; i++) {
        particles[i] = ParticleFactory::createParticle(ParticleTypeID::EMPTY);
    }

    num_particle_chunks_x = 0;
    num_particle_chunks_y = 0;

    particleChunks.clear();

    for(int y = 0; y <= 1 + height/ParticleChunk::CHUNK_SIZE; y++){
        num_particle_chunks_y ++;
        num_particle_chunks_x = 0;
        for(int x = 0; x <= 1 + width/ParticleChunk::CHUNK_SIZE; x++){
            num_particle_chunks_x ++;
            ParticleChunk chunk;
            chunk.x = x;
            chunk.y = y;
            particleChunks.push_back(chunk);
        }
    }

    num_threads = 4;

    processing_chunks = new ProcessingChunk[num_threads*2];
    {
        int dy = (height) / (num_threads*2) + 1;
        for(int i = 0; i < num_threads*2; i++) {
            processing_chunks[i].x = 0;
            processing_chunks[i].y = dy*i;
            processing_chunks[i].width = width;
            processing_chunks[i].height = dy;
        }
    }
    
    processing_threads.initializeThreads(num_threads);
    processing_threads.setFunction(&processingTask);
}

Particle& Grid::getParticle(int x, int y){
    return particles[y * width + x];
}

bool Grid::isInBounds(int x, int y) {
    return x >= 0 && x < width && y >= 0 && y < height;
}


void Grid::setParticle(int x, int y, Particle particle) {
    if(!isInBounds(x, y)) {
        printf("Attempted to set out of bounds particle at (%d, %d)\n", x, y);
        return;  // Out of bounds
    }

    particle.x = x;  // Set particle's position
    particle.y = y;
    particle.hasChanged = true;  // Mark as changed

    particles[y * width + x] = particle;

    onParticleUpdate(x, y);
}

void Grid::removeParticle(int x, int y) {
    if(x < 0 || x >= width || y < 0 || y >= height) {
        // printf("Attempted to remove out of bounds particle at (%d, %d)\n", x, y);
        return;  // Out of bounds
    }

    Particle& existing = particles[y * width + x];
    Particle empty = ParticleFactory::createParticle(ParticleTypeID::EMPTY);
    existing = empty;  // Copy the empty particle's data

    onParticleUpdate(x, y);
}

void Grid::swapParticles(int x0, int y0, int x1, int y1) {

    // Pre-calculate indices once
    int idx0 = y0 * width + x0;
    int idx1 = y1 * width + x1;
    
    // Bounds check on indices (faster than coordinate checks)
    if (idx0 < 0 || idx0 >= num_particles || idx1 < 0 || idx1 >= num_particles) {
        printf("Attempted to swap out of bounds particles at (%d, %d) and (%d, %d)\n", x0, y0, x1, y1);
        return;
    }
    
    // Use std::swap (compiler optimized)
    std::swap(particles[idx0], particles[idx1]);
    
    // Update positions efficiently
    particles[idx0].x = x0;
    particles[idx0].y = y0;
    particles[idx0].hasChanged = true;
    
    particles[idx1].x = x1;
    particles[idx1].y = y1;
    particles[idx1].hasChanged = true;
    
    // Batch render updates
    onParticleUpdate(x0, y0);
    onParticleUpdate(x1, y1);
}

void updateChunk(int x, int y){
    ParticleChunk& chunk = Grid::getParticleChunk(x, y);
    chunk.dirty = true;
    chunk.type_data_valid = false;  // Invalidate type data
    chunk.shouldProcessNextFrame = true;  // Mark for processing next frame
}

void Grid::onParticleUpdate(int x, int y) {
    // if(x % ParticleChunk::CHUNK_SIZE == 0){
    //     updateChunk(x-1, y);
    // }else if(x % ParticleChunk::CHUNK_SIZE == ParticleChunk::CHUNK_SIZE - 1){
    //     updateChunk(x+1, y);
    // }

    // if(y % ParticleChunk::CHUNK_SIZE == 0){
    //     updateChunk(x, y-1);
    // }else if(y % ParticleChunk::CHUNK_SIZE == ParticleChunk::CHUNK_SIZE - 1){
    //     updateChunk(x, y+1);
    // }

    updateChunk(x,y);
}

// Check if there could be a particle of the specified type in the neighborhood
// return false guarantees no particles of that type are present
// return true means there could be particles of that type in the neighborhood
bool Grid::isParticleNearType(int x, int y, ParticleTypeID type, int max_x, int max_y) {
    // Calculate the bounding box of the search area
    int min_x = x - max_x;
    int max_x_coord = x + max_x;
    int min_y = y - max_y;
    int max_y_coord = y + max_y;
    
    // Convert to chunk coordinates
    int min_chunk_x = min_x / ParticleChunk::CHUNK_SIZE;
    int max_chunk_x = max_x_coord / ParticleChunk::CHUNK_SIZE;
    int min_chunk_y = min_y / ParticleChunk::CHUNK_SIZE;
    int max_chunk_y = max_y_coord / ParticleChunk::CHUNK_SIZE;
    
    // Clamp to valid chunk bounds
    min_chunk_x = std::max(0, min_chunk_x);
    max_chunk_x = std::min(num_particle_chunks_x - 1, max_chunk_x);
    min_chunk_y = std::max(0, min_chunk_y);
    max_chunk_y = std::min(num_particle_chunks_y - 1, max_chunk_y);
    
    // Check each unique chunk only once
    for(int chunk_y = min_chunk_y; chunk_y <= max_chunk_y; chunk_y++) {
        for(int chunk_x = min_chunk_x; chunk_x <= max_chunk_x; chunk_x++) {
            ParticleChunk& chunk = particleChunks[chunk_y * num_particle_chunks_x + chunk_x];
            if(chunk.hasParticleType(type)) {
                return true;
            }
        }
    }

    return false;
}

void Grid::processParticles() {
    static bool is_flipped = false;
    is_flipped = !is_flipped;

    for(int x = 0; x < num_particle_chunks_x; x++){
        for(int y = 0; y < num_particle_chunks_y; y++) {
            ParticleChunk& chunk = particleChunks[y * num_particle_chunks_x + x];
            if(!chunk.type_data_valid) {
                chunk.rebuildTypeData();
            }
        }
    }

    // for(int x = 0; x < num_particle_chunks_x; x++){
    //     for(int y = num_particle_chunks_y - 1; y >= 0; y--) {
    //         ParticleChunk& chunk = particleChunks[y * num_particle_chunks_x + x];
    //         if(chunk.shouldProcess) {
    //             for(int i = ParticleChunk::CHUNK_SIZE-1; i >=0; i--){
    //                 for(int j = 0; j < ParticleChunk::CHUNK_SIZE; j++) {
    //                     int x = j + chunk.x * ParticleChunk::CHUNK_SIZE;
    //                     int y = i + chunk.y * ParticleChunk::CHUNK_SIZE;
    //                     int index = y*width + x;
    //                     if(index < num_particles)
    //                         particles[y*width + x].onBlockUpdate();
    //                 }
    //             }
    //         }
    //     }
    // }

    for(int y = height - 1; y >= 0; y--){
        for(int x0 = 0; x0 < width; x0++) {
            int x = x0;
            if(is_flipped != (y%2==0)){
                x = width - x0 - 1;
            }
            
            particles[y * width + x].onBlockUpdate();
        }
    }

    for(int x = 0; x < num_particle_chunks_x; x++){
        for(int y = 0; y < num_particle_chunks_y; y++) {
            ParticleChunk& chunk = particleChunks[y * num_particle_chunks_x + x];
            // chunk.shouldProcess = chunk.shouldProcessNextFrame;
            // chunk.shouldProcessNextFrame = false;
            chunk.dirty = true;
        }
    }

    // for(int y = height - 1; y >= 0; y--) {
    //     for(int x0 = 0; x0 < width; x0++) {
    //         int x = x0;
    //         if(is_flipped != (y%2==0)){
    //             x = width - x0 - 1;
    //         }
            
    //         particles[y * width + x].onBlockUpdate();
    //     }
    // }

    // for(int offset0 = 0; offset0 <= 1; offset0++) {
    //     int offset = offset0;
    //     if(is_flipped){
    //         offset = 1 - offset0;  // Flip offset for even/odd rows
    //     }

    //     for(int i = 0; i < num_threads; i++) {
    //         ProcessingChunk& chunk = processing_chunks[i*2 + offset];
    //         chunk.is_flipped = is_flipped;
    //         processing_threads.setThreadData(i, chunk);
    //     }

    //     processing_threads.executeAndWait();
    // }
}


void Grid::cleanup() {
    delete[] particles;
    particles = nullptr;

    for(auto& chunk : Grid::particleChunks) {
        if(chunk.texture != nullptr) {
            SDL_DestroyTexture(chunk.texture);
        }
    }
}