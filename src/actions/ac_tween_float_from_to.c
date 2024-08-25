#include "../actions/actions.h"
#include <raylib.h>

void actions_tween_float_from_to_update(ActionRunner *action_runner, EntityId id,
                                        GameState *game_state) {
    ActionTweenFloatFromTo *action = (ActionTweenFloatFromTo *)action_runner->action;
    f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
    if (h >= 1) {
        h = 1;
        action_runner->completed = true;
    }

    *action->value = action->from + (action->to - action->from) * h;
}

ActionTweenFloatFromTo *actions_tween_float_from_to_create(f32 duration, b32 loop, f32 *value,
                                                           f32 from, f32 to,
                                                           MemoryArenaHandle *memory_arena_handle) {
    ActionTweenFloatFromTo *action =
        memory_arena_push(memory_arena_handle, sizeof(ActionTweenFloatFromTo));
    *action = (ActionTweenFloatFromTo){
        .base = (struct Action){.type = AT_TWEEN_FLOAT_FROM_TO,
                                .duration = duration,
                                .loop = loop,
                                .update = actions_tween_float_from_to_update},
        .value = value,
        .from = from,
        .to = to,
    };
    return action;
}
