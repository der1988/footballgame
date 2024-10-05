#include "actions.h"
#include <raylib.h>
#include <stdarg.h>

void actions_sequence_start(struct ActionRunner *action_runner, EntityId id,
                            struct GameState *game_state) {
    struct ActionSequence *action_sequence = (struct ActionSequence *)action_runner->action;

    // Reset all children actions
    for (usize i = 0; i < action_sequence->count; ++i) {
        action_sequence->actions[i].started = false;
    }
}

void actions_sequence_update(struct ActionRunner *action_runner, EntityId id,
                             struct GameState *game_state) {
    struct ActionSequence *action_sequence = (struct ActionSequence *)action_runner->action;

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

struct ActionSequence *actions_sequence_create(MemoryArenaHandle *memory_arena_handle, u32 count,
                                               ...) {
    struct ActionSequence *action_sequence = memory_arena_push(
        memory_arena_handle, sizeof(struct ActionSequence) + sizeof(struct ActionRunner) * count);
    *action_sequence = (struct ActionSequence){
        .base = (struct Action){.type = AT_SEQUENCE,
                                .loop = false,
                                .start = actions_sequence_start,
                                .update = actions_sequence_update},
    };

    f32 total_duration = 0;
    va_list ap;
    va_start(ap, count);
    for (usize i = 0; i < count; ++i) {
        action_sequence->actions[i] = (struct ActionRunner){.action = va_arg(ap, struct Action *)};
        total_duration += action_sequence->actions[i].action->duration;
    }
    va_end(ap);

    action_sequence->count = count;
    action_sequence->base.duration = total_duration;

    return action_sequence;
}
