#ifndef TWEAKUI_H
#define TWEAKUI_H

#include "game.h"

#ifndef TWEAKUI_INTERNAL_H
typedef void TweakUI;
#endif // !TWEAKUI_INTERNAL_H

TweakUI *tweakui_create(MemoryArenaHandle *memory_arena_handle);
void tweakui_destroy(TweakUI *handle);
void tweakui_draw(TweakUI *handle, struct GameState *game_state);

#endif // !TWEAKUI_H
