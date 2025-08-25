#include "particles/Particle.h"
#include <vector>
#include <SDL3/SDL.h>
#include <shared_mutex>
#include <queue>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "ThreadGroup.h"

#ifndef GRID_H
#define GRID_H

#include "structures/ParticleChunk.h"

struct ProcessingChunk{
    int x, y;
    int width, height;
    int is_flipped;
};

class Grid {
    private:
        static Particle* particles;
        static std::vector<Particle*> processing_queue;
        static ThreadGroup<ProcessingChunk> processing_threads;
        static ProcessingChunk* processing_chunks;

    public:
        static int width, height, num_particles, num_threads;
        static int num_particle_chunks_x, num_particle_chunks_y;
        

        static std::vector<ParticleChunk> particleChunks;

        static void init(int w, int h);
        static void cleanup();


        static Particle& getParticle(int x, int y);
        static inline bool isCellEmpty(int x, int y) {
            if(x < 0 || x >= width || y < 0 || y >= height) {
                return false;  // Out of bounds
            }
            return getParticle(x, y).type_id == ParticleTypeID::EMPTY;
        };
        static inline bool isCellNonSolid(int x, int y) {
            if(x < 0 || x >= width || y < 0 || y >= height) {
                return false;  // Out of bounds
            }

            return particles[y * width + x].state != MatterState::SOLID;
        };
        static bool isInBounds(int x, int y);
        static inline int getParticleIndex(int x, int y) {
            return y * width + x;  // Calculate index based on grid dimensions
        };
        static inline int getParticleChunkIndex(int px, int py) {
            int chunk_x = px / ParticleChunk::CHUNK_SIZE;
            int chunk_y = py / ParticleChunk::CHUNK_SIZE;
            return chunk_y * num_particle_chunks_x + chunk_x;
        };
        static inline ParticleChunk& getParticleChunk(int px, int py) {
            return particleChunks[getParticleChunkIndex(px, py)];
        };
        static bool isParticleNearType(int x, int y, ParticleTypeID type, int max_x=1, int max_y=1);

        static void setParticle(int x, int y, Particle particle);
        static void removeParticle(int x, int y);
        static void swapParticles(int x0, int y0, int x1, int y1);
        static void onParticleUpdate(int x, int y);

        static void processParticles();
        static void processingTask(ProcessingChunk chunk, int thread_id);

};

#endif // GRID_H