#ifndef ENTITIES_H
#define ENTITIES_H

#include "raylib.h"

// Enemy struct is defined here since it's needed for the game state.
typedef struct {
    Rectangle rect;
    int state;
    float changeStateTimer;
    float changeStateThreshold;
} Enemy;

// Game screen struct is also needed for game state.
typedef enum {
    SCREEN_MENU,
    SCREEN_GAME,
    SCREEN_GAMEOVER
} GameScreen;

// Game state was made into a separate header file since everyone (main.c, game.c and ui.c) needs to be able to access one or more of its components.
typedef struct {
    GameScreen currentScreen;
    Rectangle player1;
    Enemy enemy;
    Rectangle projectile;
    Vector2 projectileVel;
    int score1;
    int score2;
    int selectedButton;
} GameState;

// Image button was added here since more than one file (main.c and ui.c) needs access to its components.
typedef struct {
    Texture2D tex;
    Rectangle bounds;
    float baseScale;
    float hoverScale;
    int id;
} ImageButton;

#endif
