# Pong (C + raylib)

A small Pong clone written in C using [raylib](https://www.raylib.com/), with a menu, keyboard/mouse-navigable UI, a simple AI opponent, sound, and a game-over screen.

## About this project

This is a first exploration of [raylib](https://www.raylib.com/) and C game programming, built intentionally as a **learning project rather than a polished product**. The goal was to get comfortable with the language, the library, and general game-loop structure — not to build a physically accurate or feature-complete Pong.

Because of that, a few deliberate trade-offs run through the whole codebase:

- **Simple over strictly correct.** Where a fully accurate simulation would add real complexity for little practical benefit (e.g. paddle-hit angles, sub-pixel collision), the simpler version was chosen on purpose. These spots are called out explicitly below so it's clear they're a choice, not an oversight.
- **No wasted resources, even when simple.** "Simple" doesn't mean "sloppy" — the code still avoids obvious waste: assets are loaded once and reused, memory is freed on exit, and hot-path logic (the per-frame game loop) avoids doing more work than it needs to.
- **Reasonably good practices throughout.** Game state is passed explicitly between modules instead of using globals, headers only declare what's shared, and each file has a single clear responsibility (see [Project structure](#project-structure) below).

This README exists mainly so that **future me** (or anyone else poking through this as a reference) can see not just *what* the code does, but *why* it was written that way — including a few bugs that came up during development and the reasoning behind how they were fixed.

## Screenshots

| Main Menu | Gameplay | Game Over |
|---|---|---|
| ![alt](<screenshots/Main menu.png>) | ![alt](screenshots/gameplay.png) | ![alt](<screenshots/Game over.png>) |

## Features

- Menu, in-game, and game-over screens, with a simple screen-management pattern (an enum + a switch)
- Keyboard **and** mouse menu navigation, without the two interfering with each other
- Custom pixel-art image buttons (no GUI library), with a hover/selection scale animation
- A basic reactive AI opponent with randomized behavior "moods" instead of always tracking the ball perfectly
- Multiple collision sound variations, picked at random per bounce
- Assets shipped as a single packaged `.rres` file rather than a loose folder of images/sounds

## Design notes & considerations

These are the non-obvious decisions and bugs encountered while building this, kept here so the reasoning isn't lost:

- **Shared game state, not globals.** All gameplay/UI data needed by more than one file lives in a single `GameState` struct (`entities.h`), passed by pointer into `game.c` and `ui.c`. This keeps the modules decoupled without resorting to global variables.
- **Persistent values must live in `GameState`, not as local variables.** Anything that needs to survive across frames (ball velocity, current menu selection) caused a hard-to-spot bug early on when kept as a local variable inside the per-frame update function — it was silently reset to its initial value every single frame. Lesson: if a value needs to persist, it needs to live somewhere that isn't re-declared every call.
- **Collision order matters for wall bounces.** The projectile's position is updated *before* checking for a bounce, and `Clamp()` is used to guarantee the ball is pushed back inside bounds. Checking-then-moving (in that order) caused a subtle one-frame-lag bug where the ball could get stuck oscillating exactly on a boundary line, retriggering the bounce sound every frame.
- **Paddle collisions always bounce horizontally, matching the original arcade Pong** (its paddle-hit mechanic only ever varies the *y*-speed based on where the paddle was struck — it doesn't reflect the *y*-axis the way a wall does). Since this project uses full rectangle-overlap collision (`CheckCollisionRecs`) rather than the original's more restrictive edge-crossing check, a paddle moving into the ball from above or below could otherwise get "stuck" retriggering collisions every frame. The fix: on any paddle collision, the ball is explicitly repositioned outside the paddle along the x-axis, guaranteeing separation regardless of which direction actually caused the overlap.
- **Asset paths are resolved via `GetApplicationDirectory()`**, not relative paths from the working directory. A relative path like `"../res/image.png"` only works if the program happens to be launched from a specific working directory — which isn't guaranteed (double-clicking the exe, launching from a shortcut, or running via an IDE can all set a different working directory). `GetApplicationDirectory()` anchors paths to the executable's own location instead, which is what actually matters here.
- **Assets are packaged with [rres](https://github.com/raysan5/rres)** rather than shipped as a loose folder sitting next to the executable. This means the game ships as a single resource file (`pong_resources.rres`) plus the executable, rather than exposing individual image/sound files a player could freely browse or extract.
- **Mouse and keyboard menu navigation are combined carefully.** Keyboard input sets the selected button directly. Mouse movement only takes over the selection once it moves past a small dead zone — otherwise a stationary cursor left over a button (from earlier mouse use) would silently steal the highlight back from keyboard navigation the instant any per-frame check ran.
- **No console window in the shipped build.** The executable is linked with `/subsystem:windows` (see [Building](#building) below) so no terminal window appears when a player launches it. The trade-off: any `printf`-based error/debug logging in the code becomes invisible once shipped this way, since there's no console attached to display it. Worth keeping a console-enabled debug build around separately if diagnosing a loading issue.
- **Only one AI behavior, on purpose.** An earlier version had two difficulty levels (a simple ball-tracking AI and a randomized-behavior one); the simpler one was removed since this project is meant to stay a straightforward single-difficulty game rather than a completeness exercise. The removed code is easy to reintroduce if a future project wants selectable difficulty.

## Requirements

- Windows (the current build setup is MSVC/Windows-specific — see below)
- [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community edition or Build Tools), for `cl.exe` and `vcvarsall.bat`
- A prebuilt [raylib](https://www.raylib.com/) distribution for MSVC (headers + `raylib.lib`)
- [rres](https://github.com/raysan5/rres) (`rres.h` / `rres-raylib.h`) if rebuilding the resource-loading code
- [rrespacker](https://github.com/raysan5/rres/tree/master/tools/rrespacker), used to build `pong_resources.rres` from the raw image/sound assets

## Building

Two batch files drive the build, since `cl.exe` isn't available on `PATH` outside of a proper Visual Studio developer environment:

- **`shell.bat`** — sets up the build environment. It maps a `subst` drive letter to the project folder, calls `vcvarsall.bat x64` to load MSVC's command-line tools (`cl.exe`, `link.exe`, and the required `INCLUDE`/`LIB` environment variables) into the current shell session, and switches into the project drive. Run this first, once per terminal session.
  > **Note:** the `subst` path in this file has a placeholder (`path/to/filesystem`). Update it to match wherever the project actually lives before running.
- **`build.bat`** — does the actual compile, run from inside the shell set up above. It compiles `main.c`, `game.c`, and `ui.c`, links against `raylib.lib` and the Windows system libraries raylib depends on (`gdi32`, `user32`, `shell32`, `winmm`, `opengl32`, `kernel32`, `msvcrt`), embeds the compiled resource script (`pong.res`, which carries the window icon), and passes `/subsystem:windows /entry:mainCRTStartup` to the linker so the shipped executable doesn't open a console window.

To build: run `shell.bat` once per terminal session, then `build.bat` any time afterward to recompile.

## Project structure

| File | Responsibility |
|---|---|
| `main.c` | Window/audio setup, asset loading, the main loop, and screen-transition logic |
| `game.c` / `game.h` | Gameplay logic only — player input, ball movement, collisions, scoring, AI |
| `ui.c` / `ui.h` | Drawing and menu/button logic for all three screens |
| `entities.h` | Shared struct definitions (`GameState`, `Enemy`, `ImageButton`) used across files |
| `style.h` | Shared layout/sizing constants (screen size, UI sizing) |

## Controls

**Menu / Game Over screens**
- `Up` / `Down` (main menu) or `Left` / `Right` (game over) — move selection
- `Enter` — confirm selection
- Mouse — hover to select, click to confirm

**In-game**
- `Up` / `Down` — move paddle

Arrow keys are used throughout rather than something like `W`/`S`, since arrow keys sit in the same physical position on every keyboard layout — letter keys can shift position between layouts (e.g. QWERTY vs AZERTY), which would make a hardcoded letter-key binding inconsistent depending on the player's keyboard. Key rebinding/customization isn't offered, since a game this small (one input axis, no simultaneous multi-key combos) doesn't have enough control complexity to meaningfully benefit from it.

## Credits

- Built with [raylib](https://www.raylib.com/) (zlib/libpng license)
- Assets packaged with [rres](https://github.com/raysan5/rres) (MIT license)
