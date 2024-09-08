#include "actions.h"
#include "entities.h"
#include "utils.h"
#include <raylib.h>

void actions_change_color_from_to_update(ActionRunner *action_runner, EntityId id,
                                         GameState *game_state) {
    ActionChangeColorFromTo *action = (ActionChangeColorFromTo *)action_runner->action;
    f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
    if (h >= 1) {
        h = 1;
        action_runner->completed = true;
    }
    ComponentSprite *sprite = &entities_entity_get(game_state->entity_repository, id)->sprite;
    sprite->color = utils_color_lerp(action->from, action->to, h);
}

ActionChangeColorFromTo *
actions_change_color_from_to_create(f32 duration, b32 loop, Color from, Color to,
                                    MemoryArenaHandle *memory_arena_handle) {
    ActionChangeColorFromTo *action =
        memory_arena_push(memory_arena_handle, sizeof(ActionChangeColorFromTo));
    *action = (ActionChangeColorFromTo){
        .base = (struct Action){.type = AT_CHANGE_COLOR_FROM_TO,
                                .duration = duration,
                                .loop = loop,
                                .update = actions_change_color_from_to_update},
        .from = from,
        .to = to,
    };
    return action;
}
