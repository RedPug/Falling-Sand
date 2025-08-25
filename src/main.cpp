#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "structures/Grid.h"
#include "particles/ParticleFactory.h"
#include "particles/ParticleType.h"
#include "structures/ThreadGroup.h"


#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

enum class ActionState{
    NONE,
    PLACE,
    REMOVE
};

static std::pair<int, int> mouse_pos = std::make_pair(0, 0);  // Store mouse position
static ActionState currentAction = ActionState::NONE;
static ParticleTypeID selectedParticle = ParticleTypeID::SAND;  // Default particle type to place
static int selectionSize = 5;
static int gridSpacing = 4;  // Spacing between grid cells in pixels
static bool is_debug = false;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]){
    setvbuf(stdout, NULL, _IONBF, 0);
    
    SDL_SetAppMetadata("Falling Sand Sim", "1.0", "com.redpug.falling-sand");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    if (!SDL_CreateWindowAndRenderer("Falling Sand!", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    printf("Initializing ParticleTypeRegistry...\n");
    ParticleTypeRegistry::initialize();
    printf("Initializing Grid...\n");
    Grid::init(SCREEN_WIDTH/gridSpacing, SCREEN_HEIGHT/gridSpacing);
    printf("Initialization complete!\n");

    // int x = 0;
    // struct Data{
    //     int v;
    // };
    // ThreadGroup<Data> threadGroup;
    // threadGroup.initializeThreads(4);
    // printf("ThreadGroup initialized.\n");
    // threadGroup.setFunction([](Data& data, int thread_id) {
    //     printf("Thread %d got value to %d\n", thread_id, data.v);
    // });
    // threadGroup.setAllThreadData({Data{0}, Data{1}, Data{2}, Data{3}});
    // threadGroup.executeAndWait();
    // printf("ThreadGroup executed and waited!\n");
    // threadGroup.executeThreads();
    // printf("ThreadGroup executed again without waiting!\n");
    // threadGroup.waitForCompletion();
    // printf("ThreadGroup completed all tasks!\n");

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void onTick(){
    if(currentAction == ActionState::PLACE) {
        // Fill the circle with particles
        int mousex = mouse_pos.first;
        int mousey = mouse_pos.second;

        for(int y = -selectionSize; y <= selectionSize; y++) {
            for(int x = -selectionSize; x <= selectionSize; x++) {
                if(x * x + y * y < selectionSize * selectionSize) {
                    int grid_x = (mousex + x * gridSpacing) / gridSpacing;
                    int grid_y = (mousey + y * gridSpacing) / gridSpacing;
                    if(grid_x >= 0 && grid_x < Grid::width && grid_y >= 0 && grid_y < Grid::height) {
                        Particle particle = ParticleFactory::createParticle(selectedParticle);
                        // particle.hasChanged = true;
                        Grid::setParticle(grid_x, grid_y, particle);
                    }
                }
            }
        }
    } else if(currentAction == ActionState::REMOVE) {
        // Remove particles in a circle
        for(int y = -selectionSize; y <= selectionSize; y++) {
            for(int x = -selectionSize; x <= selectionSize; x++) {
                if(x * x + y * y < selectionSize * selectionSize) {
                    int grid_x = (mouse_pos.first + x * gridSpacing) / gridSpacing;
                    int grid_y = (mouse_pos.second + y * gridSpacing) / gridSpacing;
                    if(grid_x >= 0 && grid_x < Grid::width && grid_y >= 0 && grid_y < Grid::height) {
                        Grid::removeParticle(grid_x, grid_y);
                    }
                }
            }
        }
    }

    // reset particles for this round of processing
    for(int y = 0; y < Grid::height; y++) {
        for(int x = 0; x < Grid::width; x++) {
            Particle& particle = Grid::getParticle(x, y);
            particle.onTick();
        }
    }
    

    Grid::processParticles();
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        printf("Key pressed: %d\n", event->key.key);

        if(event->key.key == SDLK_1){
            selectedParticle = ParticleTypeID::SAND;
            // printf("Selected particle: SAND\n");
        } else if(event->key.key == SDLK_2){
            selectedParticle = ParticleTypeID::STONE;
            // printf("Selected particle: STONE\n");
        } else if(event->key.key == SDLK_3){
            selectedParticle = ParticleTypeID::WATER;
            // printf("Selected particle: WATER\n");
        } else if(event->key.key == SDLK_D){
            is_debug = !is_debug;
        }

    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {

        mouse_pos.first = static_cast<int>(event->motion.x);
        mouse_pos.second = static_cast<int>(event->motion.y);

    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        // printf("Mouse button pressed: %d at (%d, %d)\n", event->button.button, event->button.x, event->button.y);
        if(event->button.button == SDL_BUTTON_LEFT) {
            currentAction = ActionState::PLACE;

        }else if(event->button.button == SDL_BUTTON_RIGHT) {
            currentAction = ActionState::REMOVE;

        }

    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        // printf("Mouse wheel scrolled: (%.2f, %.2f)\n", event->wheel.x, event->wheel.y);
        float scroll = event->wheel.y;

        if(scroll > 0){
            selectionSize = std::min(selectionSize + 1, 30);
        } else if(scroll < 0) {
            selectionSize = std::max(selectionSize - 1, 5);
        }
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP){
        currentAction = ActionState::NONE;
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void drawCircle(int x, int y, int r, int thickness = 1){
    for(float angle = 0; angle < SDL_PI_F*2; angle += SDL_PI_F / 20.0f) {
        float dx = r * cos(angle);
        float dy = r * sin(angle);

        SDL_FRect rect = {
        .x = x + dx,
        .y = y + dy,
        .w = static_cast<float>(thickness),
        .h = static_cast<float>(thickness)
        };
        SDL_RenderFillRect(renderer, &rect);
    }
}

void renderGrid(){

    for(auto& chunk : Grid::particleChunks) {
        if(chunk.texture == nullptr) {
            chunk.texture = SDL_CreateTexture(renderer,
                SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET,
                ParticleChunk::CHUNK_SIZE * gridSpacing,
                ParticleChunk::CHUNK_SIZE * gridSpacing);
            SDL_SetRenderTarget(renderer, chunk.texture);
            SDL_SetRenderDrawColorFloat(renderer, 0.0f, 0.0f, 0.0f, 0.0f);
            SDL_RenderClear(renderer);
            SDL_SetRenderTarget(renderer, nullptr);
        }

        if(chunk.dirty || is_debug){

            SDL_SetRenderTarget(renderer, chunk.texture);

            // Clear the chunk texture with transparent background
            SDL_SetRenderDrawColorFloat(renderer, 0.0f, 0.0f, 0.0f, 0.0f);
            SDL_RenderClear(renderer);

            for(int y = 0; y < ParticleChunk::CHUNK_SIZE; y++) {
                for(int x = 0; x < ParticleChunk::CHUNK_SIZE; x++) {
                    // Calculate actual grid coordinates
                    int grid_x = chunk.x * ParticleChunk::CHUNK_SIZE + x;
                    int grid_y = chunk.y * ParticleChunk::CHUNK_SIZE + y;
                    
                    // Skip if outside grid bounds
                    if(grid_x >= Grid::width || grid_y >= Grid::height) continue;
                    
                    Particle& particle = Grid::getParticle(grid_x, grid_y);
                    if(particle.type_id == ParticleTypeID::EMPTY) continue;

                    // Set color for this particle
                    Color color;
                    if(is_debug) {
                        if(particle.hasChanged && particle.received_update) {
                            color = Color(0, 255, 0);  // Debug color for changed particles
                        }else if(particle.hasChanged && !particle.received_update){
                            color = Color(255, 0, 0);  // Debug color for unchanged particles
                        }else if(!particle.hasChanged && particle.received_update){
                            color = Color(0, 0, 255);  // Debug color for unchanged particles
                        }else{
                            color = Color(128,128,128);
                        }
                    } else {
                        color = particle.getColor();
                    }
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);

                    // Render relative to chunk texture (0,0 to CHUNK_SIZE,CHUNK_SIZE)
                    SDL_FRect rect = {
                        .x = static_cast<float>(x * gridSpacing),
                        .y = static_cast<float>(y * gridSpacing),
                        .w = static_cast<float>(gridSpacing),
                        .h = static_cast<float>(gridSpacing)
                    };
                    SDL_RenderFillRect(renderer, &rect);
                }
            }

            chunk.dirty = false;  // Reset dirty flag after rendering


            if(is_debug && chunk.shouldProcess){
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);

                SDL_FRect thisRect = {
                    .x = 0,
                    .y = 0,
                    .w = (float)(ParticleChunk::CHUNK_SIZE * gridSpacing),
                    .h = (float)(ParticleChunk::CHUNK_SIZE * gridSpacing)
                };

                SDL_RenderRect(renderer, &thisRect);
            }


            SDL_SetRenderTarget(renderer, nullptr);
        }

        SDL_FRect chunk_rect = {
            .x = static_cast<float>(chunk.x * ParticleChunk::CHUNK_SIZE * gridSpacing),
            .y = static_cast<float>(chunk.y * ParticleChunk::CHUNK_SIZE * gridSpacing),
            .w = static_cast<float>(ParticleChunk::CHUNK_SIZE * gridSpacing),
            .h = static_cast<float>(ParticleChunk::CHUNK_SIZE * gridSpacing)
        };

        if(chunk.texture)
            SDL_RenderTexture(renderer, chunk.texture, nullptr, &chunk_rect);
    }
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate){
    static Uint64 last_time = SDL_GetTicks();
    static int tick_counter = 0;
    static int tick_interval = 10;  // Time in ms between ticks
    static int frames = 0;
    Uint64 now = SDL_GetTicks();

    int dt = now - last_time;
    // int target_time = 1000/60;

    int max_frame_time = 1000 / 60;  // Maximum frame time for 60 FPS

    frames++;

    tick_counter += dt;

    
    //stop ticking if it's taking too long to presere frame rate
    // printf("Ticking...\n");
    while(tick_counter >= tick_interval && SDL_GetTicks() - now < max_frame_time) {
        
        tick_counter -= tick_interval;
        
        onTick();
        
        
    }
    // printf("done ticking...\n");

    last_time = now;
    
    
    //clear the window.
    SDL_SetRenderDrawColorFloat(renderer, 0.1f, 0.1f, 0.1f, SDL_ALPHA_OPAQUE_FLOAT);
    SDL_RenderClear(renderer);

    {
        Uint64 t0 = SDL_GetTicks();

        // printf("rendering...\n");
        renderGrid();
        // printf("done rendering...\n");

        Uint64 t1 = SDL_GetTicks();

        // printf("Render took %llu ms\n", t1 - t0);
    }

    //render UI
    SDL_SetRenderDrawColor(renderer, 128, 0, 0, 128);
    drawCircle(mouse_pos.first, mouse_pos.second, selectionSize * gridSpacing, 2);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result){
    /* SDL will clean up the window/renderer for us. */
    // Grid::stopThreadedProcessing();
    Grid::cleanup();  // Cleanup grid and particles
}