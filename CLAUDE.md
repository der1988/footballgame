# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build the game (debug)
make

# Run the game
./build/debug/thegame

# Build an optimised release (no debug symbols, asserts stripped)
make release
./build/release/thegame

# Run all tests
make test

# Run individual tests
make test_memory
make test_caches
make test_actions
make test_entity_repo
make test_events

# Clean build artifacts
make clean
```

Cross-platform builds: `Makefile.osx` for macOS, `Makefile.w64` for Windows.

## Architecture Overview

This is a 2D football/soccer game written in C99 using raylib for rendering. The codebase follows an Entity-Component-System (ECS) pattern with custom memory management.

### Core Systems

**Game Loop** (`src/game/`): Fixed timestep physics loop with scene management. The main state machine handles scenes (play, title, settings). `game_state` is the central struct passed to all systems.

**Entity-Component System** (`src/entities/`, `src/components/`):
- Entities are structs containing all possible components (not sparse)
- Components are accessed via offset macros: `C_TRANSFORM`, `C_PHYSICS`, `C_PLAYER`, etc.
- Entity iteration uses `FOR_EACH_ENTITY(item, entity_repo)` macro
- Entity types: camera, ball, player, goal, goal_net

**Systems** (`src/systems/`): Process entities each frame. Key systems:
- `sy_player_physics.c` - Player movement/forces
- `sy_ball.c` - Ball physics with lookahead predictions
- `sy_player_control_human.c` / `sy_player_control_ai.c` - Input handling
- `sy_goalie.c` - Goalkeeper AI and diving
- `sy_render.c` - Sprite rendering sorted by layer

**Memory Management** (`src/memory/`):
- Arena allocators for different lifetimes: `MAL_LEVEL`, `MAL_UPDATE`, `MAL_DRAW`, `MAL_TIME_MACHINE`
- Fixed-size allocators for pooled objects
- Memory is allocated upfront (256MB main arena)

**State Machines** (`src/entities/state_machine.h`, `e_player.h`, `e_ball.h`):
- Players use `state_machine_player` with states: normal, header, sliding_tackle, fall, goalie
- Goalies have separate `state_machine_goalie`: protect_goal, dive, hold_ball, jump
- Ball uses `state_machine_ball`: in_play, out_of_play, gk_hands

### Football Logic

**Tactics** (`src/football/f_tactics.c`): Team formations (4-3-3, 4-4-2, etc.) with normalized pitch positions that scale based on team momentum (defending/default/attacking).

**Match State** (`src/football/football.h`): Handles kickoff, play, goal, throw-in, corners, penalties.

**AI** (`src/ai/`): Player AI decides between keeping position, seeking, arriving, or kicking. Goalie AI handles positioning and diving decisions.

### Input System

`src/input/` handles both keyboard and gamepad with:
- Abstracted game controls (MOVE_UP, SHOOT, PASS, etc.)
- Charge mechanics for shot/pass power
- Support for up to 4 controllers

### Time Machine

Debug feature (`src/time_machine/`) that records game state capsules (30/second for 30 seconds) enabling rewind/replay during development.

### Actions System

`src/actions/` provides composable animations and tweens that run on entities (color changes, rotations, sequences, spawns).

### Key Type Aliases

From `src/types.h`:
- `v2`, `v3` - Vector2/Vector3 from raylib
- `f32`, `f64` - float/double
- `b32` - boolean (int32)
- `entity_id_t`, `fg_time_t`, `fg_time_low_prec_t`, `delta_time_t` - game-specific types

### Testing

Uses Unity test framework (`test/unity/`). Tests are compiled with `-DRUNNING_TEST` flag.
