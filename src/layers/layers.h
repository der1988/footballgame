#ifndef LAYERS_H
#define LAYERS_H

typedef struct layer_debug layer_debug_t;

struct game_state;

layer_debug_t *layer_debug_create(struct game_state *game_state);
void layer_debug_destroy(layer_debug_t *handle);
void layer_debug_reset(layer_debug_t *handle);
void layer_debug_queue_draw_text(layer_debug_t *handle, const char *text);
void layer_debug_draw(layer_debug_t *handle, struct game_state *game_state);

void layer_hud_draw(struct game_state *game_state);
#endif // !LAYERS_H
