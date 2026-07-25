#include "raylib.h"

#include "style.h"
#include "ui.h"

#define MOUSE_DEAD_ZONE 6.0f

static bool UpdateImageButton(ImageButton *btn, const bool highlighted) {
    // When the button is highlighted, it'll scale up. When it's pressed, it'll shrink back to normal.
    bool pressed = highlighted && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

    float scaledWidth = btn->bounds.width * ((highlighted && !pressed) ? btn->hoverScale : btn->baseScale);
    float scaledHeight = btn->bounds.height * ((highlighted && !pressed) ? btn->hoverScale : btn->baseScale);
    float x = btn->bounds.x + (btn->bounds.width - scaledWidth) / 2;
    float y = btn->bounds.y + (btn->bounds.height - scaledHeight) / 2;

    // Button needs to be redrawn to a slightly different coordinate to keep its central position.
    DrawTexturePro(btn->tex,
                   (Rectangle){0, 0, btn->tex.width, btn->tex.height},
                   (Rectangle){x, y, scaledWidth, scaledHeight},
                   (Vector2){0, 0}, 0.0f,
                   WHITE
                   );

    return pressed;
}

static void UpdateMenuSelection(GameState *gameState, ImageButton *menuButtons, const size_t buttonCount) {
    // This prevents sudden movements (likely made accidentaly) to be detected in changing menu selection. The 6.0f represents the amount of pixels per frame allowed, which
    //   makes it so the user has to purposefully move the cursor pretty fast to not be detected.
    //
    // It could be better handled by calculating a minimum distance and maximum speed, but those are finer details outside of the scope of a simple project such as this one.

    Vector2 mouseDelta = GetMouseDelta();
    if ((
         ( (mouseDelta.x > 0.0f) && (mouseDelta.x <  MOUSE_DEAD_ZONE) ) ||
         ( (mouseDelta.x < 0.0f) && (mouseDelta.x > -MOUSE_DEAD_ZONE) ) ||
         ( (mouseDelta.y > 0.0f) && (mouseDelta.y <  MOUSE_DEAD_ZONE) ) ||
         ( (mouseDelta.y < 0.0f) && (mouseDelta.y > -MOUSE_DEAD_ZONE) )
       )) {

        gameState->selectedButton = -1;

        // This check is made first to prevent having to make a second pass after rendering the buttons, which would also mean a re-draw.
        for (size_t i=0; i<buttonCount; i++) {
            if (CheckCollisionPointRec(GetMousePosition(), menuButtons[i].bounds)) {
                gameState->selectedButton = i;
            } 
        }
    }
}

int DrawMenu(GameState *gameState, ImageButton *menuButtons, const size_t buttonCount) {
    int buttonPressed = 0;

    UpdateMenuSelection(gameState, menuButtons, buttonCount);
    
    ClearBackground(BLACK);

    // Main menu consists of title and buttons.
    DrawText("PONG",
             (SCREEN_WIDTH/2) - (MeasureText("PONG", TITLE_SIZE)/2),
             TITLE_PADDING,
             TITLE_SIZE,
             RAYWHITE
             );

    for (size_t i=0; i<buttonCount; i++) {
        bool highlighted = (gameState->selectedButton == i);
        if (UpdateImageButton(&menuButtons[i], highlighted)) {
            buttonPressed = menuButtons[i].id;
        }
    }
    
    // The function returns the corresponding id of the button pressed, if any.
    return buttonPressed;
}

int DrawGame(GameState *gameState) {
    ClearBackground(BLACK);
    DrawRectangleRec(gameState->player1, RAYWHITE);
    DrawRectangleRec(gameState->enemy.rect, LIGHTGRAY);
    DrawRectangleRec(gameState->projectile, RAYWHITE);
    // DrawFPS(720, 0);
    DrawText(TextFormat("%d - %d", gameState->score1, gameState->score2), (SCREEN_WIDTH/2)-UI_SIZE, SCREEN_HEIGHT-UI_SIZE, UI_SIZE, RAYWHITE);

    return 0;
}

// Maybe DrawMenu() and DrawGameOver() could be made into one, but since their text and size for titles differs it was left as two.
// This could be passed as an argument, but could be confusing from the caller file's perspective and would make it so there's more than one place where the menus need to be modified.
int DrawGameOver(GameState *gameState, ImageButton *menuButtons, const size_t buttonCount) {
    int buttonPressed = 0;

    UpdateMenuSelection(gameState, menuButtons, buttonCount);
    
    ClearBackground(BLACK);

    // Gameover screen consists of text and buttons.
    DrawText("GAME OVER",
             (SCREEN_WIDTH/2) - (MeasureText("GAME OVER", TITLE_SIZE/2)/2),
             TITLE_PADDING,
             TITLE_SIZE/2,
             RAYWHITE
             );

    for (size_t i=0; i<buttonCount; i++) {
        bool highlighted = (gameState->selectedButton == i);
        if (UpdateImageButton(&menuButtons[i], highlighted)) {
            buttonPressed = menuButtons[i].id;
        }
    }
    
    // The function returns the corresponding id of the button pressed, if any.
    return buttonPressed;
}
