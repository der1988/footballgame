#ifndef TWEAKUI_H
#define TWEAKUI_H

#include "game.h"

typedef struct TweakUI TweakUI;

TweakUI *tweakui_create(MemoryArenaHandle *memory_arena_handle);
void tweakui_destroy(TweakUI *handle);
void tweakui_draw(TweakUI *handle, struct GameState *game_state);

#endif // !TWEAKUI_H
