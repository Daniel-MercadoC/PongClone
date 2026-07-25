#ifndef GAME_H
#define GAME_H

#include "entities.h"

void EnemyAI(GameState *gameState, const int vel, const float positionToFollow, const int screenHeight);
int RunGame(const int screenHeight, const int screenWidth, GameState *gameState, bool *collisionTriggered, bool *resetProjectilePos);

#endif
