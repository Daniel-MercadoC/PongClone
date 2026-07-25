#include <stdio.h>

#include "raylib.h"

#define RRES_IMPLEMENTATION
#include "rres.h"

#define RRES_RAYLIB_IMPLEMENTATION
#define RRES_SUPPORT_COMPRESSION_LZ4
#define RRES_SUPPORT_ENCRYPTION_AES
#define RRES_SUPPORT_ENCRYPTION_XCHACHA20
#include "rres-raylib.h"       // Required to map rres data chunks into raylib structs

#include "style.h"
#include "entities.h"
#include "game.h"
#include "ui.h"

#define COLLISION_SOUND_VARIATIONS 4
#define ENEMY_CHANGE_STATE_THRESHOLD 4.0

// Macros defined to make a flat enum for buttons in different screens.
// This allows for a couple of benefits:
//     Names for every button are only specified once.
//     Number of buttons is always consistent.
//     Ability to use loops for multiple types of data, like a button and its texture, which refer to the same object but need separate initialization.
//     Including a "BUTTON_COUNT" at the end of every menu's list allows counting them and cleanly separating.
//
// The downside to this is, of course, only the first screen's buttons will start at 0; but that only implies adding, subtracting or a very simple check whenever needed.
#define BUTTON_LIST \
    X(EMPTY_BUTTON          , "Empty") \
    X(PLAY_BUTTON           , "Play")  \
    X(QUIT_MENU_BUTTON      , "Quit")  \
    X(MENU_BUTTON_COUNT     , "Count") \
    \
    X(RETRY_BUTTON          , "Retry") \
    X(QUIT_GAMEOVER_BUTTON  , "Quit")  \
    X(GAMEOVER_BUTTON_COUNT , "Count")

typedef enum {
    #define X(name, str) name,
    BUTTON_LIST
    #undef X
} Buttons;
const char* button_to_string(const Buttons b) {
    #define X(name, str) if (b == name) return str;
    BUTTON_LIST
    #undef X
    return "Unknown";
}

void LoadResources(Texture2D* textureArray, Sound* soundArray);

int main() {
    const Vector2 playerSize    = {20, 200};
          int     playerOriginY = (SCREEN_HEIGHT/2)-(playerSize.y/2);
    
    const Vector2 projectileSize = {20, 20};
          Vector2 projectilePos  = {
            (SCREEN_WIDTH/2)-(projectileSize.x/2),
            (SCREEN_HEIGHT/2)-(projectileSize.y/2)
          };
          
    // State stores the data needed in both game logic and UI:
    //     Current screen allows for the menu to be controlled via the ui, since there's no reason to separate the menu from its logic to different files.
    //     Players store their size and position.
    //     Enemy stores a player, its current behaviour and a timer and frequency to change behaviour.
    //     Projectile stores its size and position.
    //     Projectile velocity is stored in game state because it should be persistent across the game's lifetime.
    //     Selected button is stored in GameState (not per-screen) since most screens need one, and the storage cost of a single int is negligible.

    Rectangle player2   = {SCREEN_WIDTH-playerSize.x, playerOriginY, playerSize.x, playerSize.y};
    GameState gameState = {
        .currentScreen  = SCREEN_MENU,
        .player1        = {0, playerOriginY, playerSize.x, playerSize.y},
        .enemy          = {player2, 10, 0, ENEMY_CHANGE_STATE_THRESHOLD},
        .projectile     = {projectilePos.x, projectilePos.y, projectileSize.x, projectileSize.y},
        .projectileVel  = {100, 100},
        .score1         = 0,
        .score2         = 0,
        .selectedButton = 0
    };

    // These variables aren't set inside a structure since they're never used outside the scope of this file.
    int buttonPressed = 0;
    bool collisionTriggered = false;
    bool resetProjectilePos = false;
    bool quitRequested = false;
    
    // ------------------------------Initialization------------------------------
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pong");
    InitAudioDevice();
    if (!IsAudioDeviceReady()) printf("No audio device was found\n");
    
    // All image and texture data should be loaded after Initializing the window, since they require OpenGL.
    const char *baseDir = GetApplicationDirectory();
    Image icon = LoadImage(TextFormat("%sicon.png", baseDir));
    
    if (!IsImageValid(icon)) printf("ERROR: Image is not valid\n");

    // Raylib expects this format for any RGBA image.
    ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(icon);

    // Need to load textures first for buttons made with images.
    Texture2D menuButtonTextures[GAMEOVER_BUTTON_COUNT-1] = {0};
    
    // Variations in the collision sound.
    Sound collisionSounds[COLLISION_SOUND_VARIATIONS];

    // Load textures and sounds using rres file
    LoadResources(menuButtonTextures, collisionSounds);

    // Menu buttons are initialized with coordinates by basing them as much as possible on the screen's size.
    // They also have their own id, which makes it easier to work with the flat enum defined before.
    // This could be handled by a loop, but wasn't in favor of positioning the buttons manually. Still, making this into a proper function will definitely be necessary in bigger projects.
    ImageButton menuButtons[MENU_BUTTON_COUNT-1] = {0};
    menuButtons[0] = (ImageButton){
        .tex = menuButtonTextures[PLAY_BUTTON-1],
        .bounds = (Rectangle){SCREEN_WIDTH/3, (SCREEN_HEIGHT/3) + TITLE_PADDING, SCREEN_WIDTH/3, 110},
        .baseScale = 1,
        .hoverScale = 1.2,
        .id = PLAY_BUTTON
    };
    menuButtons[1] = (ImageButton){
        .tex = menuButtonTextures[QUIT_MENU_BUTTON-1],
        .bounds = (Rectangle){SCREEN_WIDTH/3, (SCREEN_HEIGHT * 2/3), SCREEN_WIDTH/3, 110},
        .baseScale = 1,
        .hoverScale = 1.2,
        .id = QUIT_MENU_BUTTON
    };

    ImageButton gameoverButtons[GAMEOVER_BUTTON_COUNT-MENU_BUTTON_COUNT] = {0};
    gameoverButtons[0] = (ImageButton){
        .tex = menuButtonTextures[RETRY_BUTTON-1],
        .bounds = (Rectangle){SCREEN_WIDTH/8, (SCREEN_HEIGHT * 2/3), SCREEN_WIDTH/3, 110},
        .baseScale = 1,
        .hoverScale = 1.2,
        .id = RETRY_BUTTON
    };
    gameoverButtons[1] = (ImageButton){
        .tex = menuButtonTextures[QUIT_GAMEOVER_BUTTON-1],
        .bounds = (Rectangle){(SCREEN_WIDTH * 4/8), (SCREEN_HEIGHT * 2/3), SCREEN_WIDTH/3, 110},
        .baseScale = 1,
        .hoverScale = 1.2,
        .id = QUIT_GAMEOVER_BUTTON
    };

    // This game shouldn't pretty much ever run lower, given its use case.
    SetTargetFPS(120);

    while (!WindowShouldClose() && !quitRequested) {

        // The checks here were unified to keep everything inside one switch statement to make it easily readable and make sure everything runs in an appropriate order:
        //     Key detection is run before anything else to avoid any data loss from modifications during the rest of the loop. Which shouldn't happen, but can.
        //     Button selection from the user happens after that, since it's involved in further checks.
        //     Finally, screen management and variable resetting happens last. This facilitates keeping data clean rather than carrying over accidentaly from one frame to the other.
        switch (gameState.currentScreen) {
            
            case SCREEN_MENU:
                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) {
                    gameState.selectedButton = (gameState.selectedButton+1) % (MENU_BUTTON_COUNT-1);
                }
                if (IsKeyPressed(KEY_ENTER)) buttonPressed = menuButtons[gameState.selectedButton].id;
                if (buttonPressed == PLAY_BUTTON) {
                    gameState.currentScreen = SCREEN_GAME;
                    gameState.selectedButton = 0;
                }
                if (buttonPressed == QUIT_MENU_BUTTON) quitRequested = true;
                buttonPressed = 0;
                break;
                
            case SCREEN_GAME:
                break;
                
            case SCREEN_GAMEOVER:
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
                    gameState.selectedButton = (gameState.selectedButton+1) % (GAMEOVER_BUTTON_COUNT-MENU_BUTTON_COUNT-1);
                }
                if (IsKeyPressed(KEY_ENTER)) buttonPressed = gameoverButtons[gameState.selectedButton].id;
                if (buttonPressed == RETRY_BUTTON) {
                    gameState.currentScreen = SCREEN_GAME;
                    gameState.score1 = 0;
                    gameState.score2 = 0;
                    gameState.selectedButton = 0;
                }
                if (buttonPressed == QUIT_GAMEOVER_BUTTON) quitRequested = true;
                buttonPressed = 0;
                break;
        }

        // Logic in game.c
        // Sound should be moved to a different file for a bigger project, but it's kept here since it's only 4 sound variations for a projectile collision.
        if (gameState.currentScreen == SCREEN_GAME) {
            RunGame(SCREEN_HEIGHT, SCREEN_WIDTH, &gameState, &collisionTriggered, &resetProjectilePos);
                
            if (collisionTriggered) {
                PlaySound(collisionSounds[GetRandomValue(0,3)]);
                collisionTriggered = false;
            }
            if (resetProjectilePos) {
                gameState.projectile.x = projectilePos.x;
                gameState.projectile.y = projectilePos.y;
                resetProjectilePos     = false;
            }
        }

        if (gameState.score1 == 2 || gameState.score2 == 2) {
            gameState.currentScreen = SCREEN_GAMEOVER;
        }

        // Drawing in screen_manager.c
        // This could also happen on the same switch statement shown before, but it's kept as its own section to limit the amount of logic ran during frame rendering.
        BeginDrawing();
            switch (gameState.currentScreen) {
                case SCREEN_MENU:
                    buttonPressed = DrawMenu(&gameState, menuButtons, MENU_BUTTON_COUNT-1);
                    break;
                case SCREEN_GAME:
                    buttonPressed = DrawGame(&gameState);
                    break;
                case SCREEN_GAMEOVER:
                    buttonPressed = DrawGameOver(&gameState, gameoverButtons, GAMEOVER_BUTTON_COUNT-MENU_BUTTON_COUNT-1);
                    break;
            }
        EndDrawing();
    }

    // ------------------------------Close------------------------------
    for (int i=0; i<COLLISION_SOUND_VARIATIONS; i++) {
        UnloadSound(collisionSounds[i]);
    }

    UnloadImage(icon);

    for (size_t i=0; i<GAMEOVER_BUTTON_COUNT-1; i++) {
        UnloadTexture(menuButtonTextures[i]);
    }
    
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

void LoadResources(Texture2D* textureArray, Sound* soundArray) {
    // Load central directory from .rres file (if available)
    rresCentralDir dir = rresLoadCentralDirectory("pong_resources.rres");
    rresResourceChunk chunk = {0};
    unsigned int id = 0;

    // Load textures
    for (size_t i=0; i<GAMEOVER_BUTTON_COUNT-1; i++) {
        if (i == MENU_BUTTON_COUNT-1 || i == GAMEOVER_BUTTON_COUNT-1) continue;

        // Get resource id from original fileName (stored in central directory)
        id = rresGetResourceId(dir, TextFormat("%sButton.png", button_to_string(i+1)));
        if (id == 0) printf("ERROR: Resource id not found for %sButton.png", button_to_string(i+1));
        
        chunk = rresLoadResourceChunk("pong_resources.rres", id);
        // Unpack isn't always necessary, but it's good practice to add it (at least as a comment) in case there's any need to use passwords or compression later
        int result = UnpackResourceChunk(&chunk);

        if (result == RRES_SUCCESS) {
            Image image = LoadImageFromResource(chunk);
            textureArray[i] = LoadTextureFromImage(image);
            SetTextureFilter(textureArray[i], TEXTURE_FILTER_POINT);
            UnloadImage(image);
        } else {
            printf("ERROR: Couldn't unpack the resource chunk for %sButton.png\n", button_to_string(i+1));
        }
        
        // The chunk may be used next iteration of the loop, but it doesn't hurt to unload that part of memory as soon as it's no longer used
        rresUnloadResourceChunk(chunk);
    }

    // Load sounds
    for (int i=0; i<COLLISION_SOUND_VARIATIONS; i++) {
        
        // Get resource id from original fileName (stored in central directory)
        id = rresGetResourceId(dir, TextFormat("collision%d.wav", i+1));
        if (id == 0) printf("ERROR: Resource id not found for collision%d.wav", i+1);
        
        chunk = rresLoadResourceChunk("pong_resources.rres", id);
        // For wave files, unpack is necessary
        int result = UnpackResourceChunk(&chunk);

        if (result == RRES_SUCCESS) {
            Wave wave = LoadWaveFromResource(chunk);
            soundArray[i] = LoadSoundFromWave(wave);
            UnloadWave(wave);
        } else {
            printf("ERROR: Couldn't unpack the resource chunk for collision%d.wav\n", i+1);
        }

        // The chunk may be used next iteration of the loop, but it doesn't hurt to unload that part of memory as soon as it's no longer used
        rresUnloadResourceChunk(chunk);
    }

    // When done loading resources into local variables, the rres file is no longer needed during runtime.
    rresUnloadCentralDirectory(dir);
}
