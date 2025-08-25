#ifndef GAME_H
#define GAME_H

#include "Grid.h"
#include <SDL3/SDL.h>

class Game {
private:
    Grid grid;
    Color currentColor;
    
public:
    Game(int gridWidth, int gridHeight, int cellSize = 10);
    
    // Game logic
    void handleMouseClick(float mouseX, float mouseY, bool isLeftClick);
    void handleKeyPress(SDL_Keycode key);
    void createSamplePattern();
    
    // Rendering
    void render(SDL_Renderer* renderer);
    
    // Getters
    Grid& getGrid();
    Color getCurrentColor() const;
};

#endif // GAME_H
