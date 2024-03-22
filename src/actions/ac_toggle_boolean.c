#include "../actions/actions.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../utils/utils.h"
#include <raylib.h>

void actions_toggle_boolean_update(ActionRunner *action_runner, EntityId id,
                                   GameState *game_state) {
    ActionToggleBoolean *action = (ActionToggleBoolean *)action_runner->action;

    action_runner->completed = true;
    *action->value = !*action->value;
}

ActionToggleBoolean *actions_toggle_boolean_create(f32 duration, b32 loop, b32 *value,
                                                   MemoryArenaHandle *memory_arena_handle) {
    ActionToggleBoolean *action =
        memory_arena_push(memory_arena_handle, sizeof(ActionToggleBoolean));
    *action = (ActionToggleBoolean){
        .base = (struct Action){.type = AT_TOGGLE_BOOLEAN,
                                .duration = duration,
                                .loop = loop,
                                .update = actions_toggle_boolean_update},
        .value = value,
    };
    return action;
}
