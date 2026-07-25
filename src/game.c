#include "raylib.h"

#define RAYMATH_IMPLEMENTATION
#define RAYMATH_DISABLE_CPP_OPERATORS
#include "raymath.h"

#include "game.h"

int RunGame(const int screenHeight, const int screenWidth, GameState *gameState, bool *collisionTriggered, bool *resetProjectilePos) {
    const int playerVel = 200;

    // ------------------------------Player input------------------------------
    gameState->player1.y -= IsKeyDown(KEY_UP) * fmaxf(playerVel * GetFrameTime(), 1);
    gameState->player1.y += IsKeyDown(KEY_DOWN) * fmaxf(playerVel * GetFrameTime(), 1);
    gameState->player1.y = Clamp(gameState->player1.y, 0, screenHeight - gameState->player1.height);

    // ------------------------------Enemy AI------------------------------
    // Enemy moves faster than the player, hence the reduced velocity.
    EnemyAI(gameState, playerVel/4, gameState->projectile.y, screenHeight);

    // ------------------------------Game logic------------------------------
    if (CheckCollisionRecs(gameState->player1, gameState->projectile) || CheckCollisionRecs(gameState->enemy.rect, gameState->projectile)) {
        Rectangle *hitPlayer = CheckCollisionRecs(gameState->player1, gameState->projectile) ? &gameState->player1 : &gameState->enemy.rect;
        
        gameState->projectileVel.x *= -1;

        gameState->projectile.x = (gameState->projectile.x < hitPlayer->x + hitPlayer->width/2) ? (hitPlayer->x - gameState->projectile.width) : (hitPlayer->x + hitPlayer->width);
        *collisionTriggered = true;
    }
    if ((gameState->projectile.y <= 0) || (gameState->projectile.y >= (screenHeight-gameState->projectile.height))) {
        gameState->projectileVel.y *= -1;
        *collisionTriggered = true;
    }

    gameState->projectile.x += gameState->projectileVel.x * GetFrameTime();
    gameState->projectile.y += gameState->projectileVel.y * GetFrameTime();    
    
    gameState->projectile.x = Clamp(gameState->projectile.x, 0, screenWidth - gameState->projectile.width);
    gameState->projectile.y = Clamp(gameState->projectile.y, 0, screenHeight - gameState->projectile.height);

    // ------------------------------Scoring------------------------------
    if ((gameState->projectile.x <= 0)) {
        gameState->score2 += 1;
        *resetProjectilePos = true;
        gameState->projectileVel.x = (GetRandomValue(0,1) ? 1 : -1) * gameState->projectileVel.x;
        gameState->projectileVel.y = (GetRandomValue(0,1) ? 1 : -1) * gameState->projectileVel.y;
    }
    if (gameState->projectile.x >= (screenWidth-gameState->projectile.width)) {
        gameState->score1 += 1;
        *resetProjectilePos = true;
        gameState->projectileVel.x = (GetRandomValue(0,1) ? 1 : -1) * gameState->projectileVel.x;
        gameState->projectileVel.y = (GetRandomValue(0,1) ? 1 : -1) * gameState->projectileVel.y;
    }

    return 0;
}

void EnemyAI(GameState *gameState, const int vel, const float positionToFollow, const int screenHeight) {
    int state = gameState->enemy.state;
    int topLimit = 0;
    int bottomLimit = screenHeight - (gameState->enemy.rect.height);

    // Change gameState->enemy behaviour.
    gameState->enemy.changeStateTimer += GetFrameTime();
    if (gameState->enemy.changeStateTimer > gameState->enemy.changeStateThreshold) {
        gameState->enemy.changeStateTimer -= gameState->enemy.changeStateThreshold;
        gameState->enemy.state = GetRandomValue(1, 10);
    }

    // Enemy behaviour depending on state.
    if (state > 6) {
        if (gameState->enemy.rect.y > (positionToFollow - gameState->enemy.rect.height/2)) gameState->enemy.rect.y -= fmaxf(vel * GetFrameTime(), 1);
        if (gameState->enemy.rect.y < (positionToFollow - gameState->enemy.rect.height/2)) gameState->enemy.rect.y += fmaxf(vel * GetFrameTime(), 1);
    } else if (state > 3) {
        gameState->enemy.rect.y += fmaxf(vel * GetFrameTime(), 1);
    } else {
        gameState->enemy.rect.y -= fmaxf(vel * GetFrameTime(), 1);
    }

    // Limit movement to screen.
    if (gameState->enemy.rect.y < topLimit) gameState->enemy.rect.y = topLimit;
    if (gameState->enemy.rect.y > bottomLimit) gameState->enemy.rect.y = bottomLimit;
}
