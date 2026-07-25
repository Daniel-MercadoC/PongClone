#ifndef UI_H
#define UI_H

#include "entities.h"

int DrawMenu(GameState *gameState, ImageButton *menuButtons, const size_t buttonCount);
int DrawGame(GameState *gameState);
int DrawGameOver(GameState *gameState, ImageButton *menuButtons, const size_t buttonCount);

#endif
