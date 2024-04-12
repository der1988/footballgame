#include "../actions/actions.h"
#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include <math.h>
#include <raylib.h>

void actions_play_animation_update(ActionRunner *action_runner, EntityId id,
                                   GameState *game_state) {
    ActionPlayAnimation *action = (ActionPlayAnimation *)action_runner->action;
    f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
    if (h >= 1) {
        h = 1;
        action_runner->completed = true;
    }
    Component *component = entities_component_get(game_state->entity_repository, CT_SPRITE, id);
    AssetsAnimation *animation =
        assets_animation_get_by_id(game_state->animation_cache, action->animation_id);
    u16 frame_to_show_idx = floorf(h * (animation->frames_count - 1));
    AssetsSpriteFrameId frame_id = animation->frames[frame_to_show_idx];
    component->sprite.frame_id = frame_id;
}

ActionPlayAnimation *actions_play_animation_create(AssetsAnimation *animation, b32 loop,
                                                   float speed,
                                                   MemoryArenaHandle *memory_arena_handle) {
    ActionPlayAnimation *action =
        memory_arena_push(memory_arena_handle, sizeof(ActionPlayAnimation));
    *action = (ActionPlayAnimation){
        .base = (struct Action){.type = AT_PLAY_ANIMATION,
                                .duration = (animation->delay * animation->frames_count / speed),
                                .loop = loop,
                                .update = actions_play_animation_update},
        .animation_id = animation->id,
        .speed = speed};
    return action;
}
