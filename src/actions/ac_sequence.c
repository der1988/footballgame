#include "../actions/actions.h"
#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../utils/utils.h"
#include <math.h>
#include <raylib.h>
#include <stdarg.h>

void actions_sequence_start(ActionRunner *action_runner, EntityId id, GameState *game_state) {
    ActionSequence *action_sequence = (ActionSequence *)action_runner->action;

    // Reset all children actions
    for (usize i = 0; i < action_sequence->count; ++i) {
        action_sequence->actions[i].started = false;
    }
}

void actions_sequence_update(ActionRunner *action_runner, EntityId id, GameState *game_state) {
    ActionSequence *action_sequence = (ActionSequence *)action_runner->action;

    usize i = 0;
    for (i = 0; i < action_sequence->count; ++i) {
        actions_action_run(&action_sequence->actions[i], id, game_state);
        if (!action_sequence->actions[i].completed) {
            break;
        }
    }

    if (i == action_sequence->count) {
        action_runner->completed = true;
    }
}

ActionSequence *actions_sequence_create(MemoryArenaHandle *memory_arena_handle, u32 count, ...) {
    ActionSequence *action_sequence = memory_arena_push(
        memory_arena_handle, sizeof(ActionSequence) + sizeof(ActionRunner) * count);
    *action_sequence = (ActionSequence){
        .base = (struct Action){.type = AT_SEQUENCE,
                                .loop = false,
                                .start = actions_sequence_start,
                                .update = actions_sequence_update},
    };

    f32 total_duration = 0;
    va_list ap;
    va_start(ap, count);
    for (usize i = 0; i < count; ++i) {
        action_sequence->actions[i] = (ActionRunner){.action = va_arg(ap, struct Action *)};
        total_duration += action_sequence->actions[i].action->duration;
    }
    va_end(ap);

    action_sequence->count = count;
    action_sequence->base.duration = total_duration;

    return action_sequence;
}
