#ifndef LAYERS_H
#define LAYERS_H

#ifndef LAYERS_INTERNAL_H
typedef void LayersDebug;
#endif // !LAYERS_INTERNAL_H

struct GameState;

LayersDebug *layers_debug_create(struct GameState *game_state);
void layers_debug_destroy(LayersDebug *handle);
void layers_debug_reset(LayersDebug *handle);
void layers_debug_queue_draw_text(LayersDebug *handle, const char *text);
void layers_debug_draw(LayersDebug *handle);

void layers_hud_draw(struct GameState *game_state);
#endif // !LAYERS_H
