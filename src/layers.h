#ifndef LAYERS_H
#define LAYERS_H

typedef struct layers_debug layers_debug_t;

struct game_state;

layers_debug_t *layers_debug_create(struct game_state *game_state);
void layers_debug_destroy(layers_debug_t *handle);
void layers_debug_reset(layers_debug_t *handle);
void layers_debug_queue_draw_text(layers_debug_t *handle, const char *text);
void layers_debug_draw(layers_debug_t *handle);

void layers_hud_draw(struct game_state *game_state);
#endif // !LAYERS_H
