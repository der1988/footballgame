#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "config.h"
#include "sprites/sprites.h"
#include "types.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ImplementStateMachineFunctions(Player, player);
ImplementStateMachineFunctions(Goalie, goalie);

static void prepare_gfx(ComponentPlayer *player, GameState *game_state);
static void prepare_goalie_gfx(ComponentPlayer *player, ComponentGoalie *goalie,
                               GameState *game_state);

struct Entity *entities_player_create(PlayerConfig config, GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_PLAYER, "player_%02d");

    // Initialise transform
    ComponentTransform *transform =
        entities_component_create(game_state->entity_repository, CT_TRANSFORM, entity->id);
    transform->position = config.position;
    transform->rotation = 0;
    transform->scale = (v2){.x = 1, .y = 1};

    // Initialise physics
    ComponentPhysics *physics =
        entities_component_create(game_state->entity_repository, CT_PHYSICS, entity->id);
    physics->mass = 75.0f;
    physics->velocity = (Vector2){0};
    physics->accel = (Vector2){0};
    physics->max_velocity = game_state->constants.player_control_max_velocity;

    // Initialise collider
    ComponentCollider *collider =
        entities_component_create(game_state->entity_repository, CT_COLLIDER, entity->id);

    // Initialise sprite
    ComponentSprite *sprite =
        entities_component_create(game_state->entity_repository, CT_SPRITE, entity->id);
    char *player_frame_name = "player_front_0";
    AssetsSpriteFrame *sprite_frame =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, player_frame_name);
    assert(sprite_frame != NULL);
    sprite->frame_id = sprite_frame->id;
    sprite->color = WHITE;
    sprite->flipX = false;
    sprite->flipY = false;
    sprite->pivot_override = (v2){.x = 0.5, .y = 0.8};
    sprite->layer = SL_PLAYERS_AND_BALL;
    sprite->draw_shadow = true;
    sprite->shadow_color = BLACK;
    sprite->shadow_color.a = 100;
    sprite->shadow_offset = (v2){.x = 0.5, .y = 0.5};
    sprite->shadow_scale = (v2){.x = 0.6, .y = 1};

    // Initialise player
    ComponentPlayer *player =
        entities_component_create(game_state->entity_repository, CT_PLAYER, entity->id);
    StateMachinePlayer *player_state_machine =
        memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(StateMachinePlayer));
    player_state_machine->owner_entity = entity->id;
    player_state_machine->previous_state = PS_NONE;
    player->state_machine = player_state_machine;

    // Initialise action
    entities_component_create(game_state->entity_repository, CT_ACTION, entity->id);

    if (config.is_goalie) {
        // Initialise goalie
        ComponentGoalie *goalie =
            entities_component_create(game_state->entity_repository, CT_GOALIE, entity->id);
        StateMachineGoalie *state_machine = memory_arena_push(
            game_state->memory_by_lifetime[MAL_LEVEL], sizeof(StateMachineGoalie));
        state_machine->owner_entity = entity->id;
        state_machine->previous_state = GS_NONE;
        state_machine->current_state = GS_PROTECT_GOAL;
        goalie->state_machine = state_machine;
        player_state_machine->current_state = PS_GOALIE;

        entities_goalie_state_machine_on_enter(entity, game_state);

        prepare_goalie_gfx(player, goalie, game_state);

        collider->colliders =
            memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(Collider) * 2);
        collider->num_colliders = 2;
        collider->colliders[1].shape = CS_CIRCLE;
        collider->colliders[1].props_circle.offset = (v2){.y = -7};
        collider->colliders[1].props_circle.radius = 10;
    } else {
        player_state_machine->current_state = PS_NORMAL;

        prepare_gfx(player, game_state);

        collider->colliders =
            memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(Collider));
        collider->num_colliders = 1;
    }

    collider->colliders[0].shape = CS_CIRCLE;
    collider->colliders[0].props_circle.radius = game_state->constants.player_control_radius;

    entities_player_state_machine_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "player created");

    return entity;
}

void entities_player_apply_force(EntityId player_entity_id, Vector2 force_to_apply,
                                 GameState *game_state) {
    ComponentPhysics *physics =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS, player_entity_id);

    physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

    // Calculate velocity
    physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, game_state->dt));
    if (Vector2Length(physics->velocity) > physics->max_velocity) {
        physics->velocity =
            Vector2Scale(Vector2Normalize(physics->velocity), physics->max_velocity);
    }
}

void entities_player_apply_velocity(EntityId player_entity_id, GameState *game_state) {
    ComponentTransform *transform =
        entities_component_get_typed(game_state->entity_repository, CT_TRANSFORM, player_entity_id);
    ComponentPhysics *physics =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS, player_entity_id);
    transform->position =
        Vector2Add(transform->position, Vector2Scale(physics->velocity, game_state->dt));
}

void entities_player_calculate_running_frame(EntityId player_entity_id, v2 direction,
                                             f32 velocity_threshold, GameState *game_state) {
    ComponentPhysics *physics =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS, player_entity_id);
    ComponentPlayer *player =
        entities_component_get_typed(game_state->entity_repository, CT_PLAYER, player_entity_id);
    ComponentSprite *sprite =
        entities_component_get_typed(game_state->entity_repository, CT_SPRITE, player_entity_id);

    if (Vector2Length(physics->velocity) > velocity_threshold) {
        Direction pd = entities_common_calculate_direction(direction);
        AssetsAnimation *animation =
            assets_animation_get_by_id(game_state->animation_cache, player->run_animations[pd]);
        f32 max_player_velocity = game_state->constants.player_run_max_velocity;
        if ((game_state->time - player->last_frame_update_time) >
            (animation->delay * (2 - Vector2Length(physics->velocity) / max_player_velocity))) {
            player->last_frame_update_time = game_state->time;
            u16 new_frame_id = (player->last_running_frame_idx + 1) % animation->frames_count;
            sprite->frame_id = animation->frames[new_frame_id];
            player->last_running_frame_idx = new_frame_id;
        }
        player->last_direction = pd;
    } else {
        sprite->frame_id = player->stand_frames[player->last_direction];
    }
}

void entities_player_state_machine_on_enter(struct Entity *entity, GameState *game_state) {
    ComponentPlayer *player =
        entities_component_get_typed(game_state->entity_repository, CT_PLAYER, entity->id);
    StateMachinePlayer *state_machine = player->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "entering state: %s",
                   StateMachinePlayerStateString[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(StateMachinePlayerStateData));

    switch (state_machine->current_state) {
    case PS_NONE:
        break;
    case PS_NORMAL: {
    } break;
    case PS_GOALIE: {
    } break;
    case PS_COUNT:
        break;
    }
}

void entities_player_state_machine_on_exit(struct Entity *entity, GameState *game_state) {
    ComponentPlayer *player =
        entities_component_get_typed(game_state->entity_repository, CT_PLAYER, entity->id);
    StateMachinePlayer *state_machine = player->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "exiting state: %s",
                   StateMachinePlayerStateString[state_machine->previous_state]);

    switch (state_machine->previous_state) {
    case PS_NONE:
        break;
    case PS_NORMAL: {
        break;
    }
    case PS_GOALIE: {
    } break;
    case PS_COUNT:
        break;
    }
}

void entities_goalie_state_machine_on_enter(struct Entity *entity, GameState *game_state) {
    ComponentGoalie *goalie =
        entities_component_get_typed(game_state->entity_repository, CT_GOALIE, entity->id);
    StateMachineGoalie *state_machine = goalie->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "entering state: %s",
                   StateMachineGoalieStateString[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(StateMachineGoalieStateData));

    switch (state_machine->current_state) {
    case GS_NONE:
        break;
    case GS_PROTECT_GOAL: {
    } break;
    case GS_COUNT:
        break;
    }
}

void entities_goalie_state_machine_on_exit(struct Entity *entity, GameState *game_state) {
    ComponentGoalie *goalie =
        entities_component_get_typed(game_state->entity_repository, CT_GOALIE, entity->id);
    StateMachineGoalie *state_machine = goalie->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "exiting state: %s",
                   StateMachineGoalieStateString[state_machine->previous_state]);

    switch (state_machine->previous_state) {
    case GS_NONE:
        break;
    case GS_PROTECT_GOAL: {
        break;
    }
    case GS_COUNT:
        break;
    }
}

static void prepare_gfx(ComponentPlayer *player, GameState *game_state) {
    // Stand frames
    player->stand_frames[D_DOWN] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_0")->id;
    player->stand_frames[D_UP] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_0")->id;
    player->stand_frames[D_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_left_0")->id;
    player->stand_frames[D_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_right_0")->id;
    player->stand_frames[D_DOWN_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_left_0")->id;
    player->stand_frames[D_DOWN_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_right_0")->id;
    player->stand_frames[D_UP_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_left_0")->id;
    player->stand_frames[D_UP_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_right_0")->id;

    // Running animations
    player->run_animations[D_DOWN] =
        entities_common_animation_id_from_name("player", "run_front", game_state);
    player->run_animations[D_UP] =
        entities_common_animation_id_from_name("player", "run_back", game_state);
    player->run_animations[D_LEFT] =
        entities_common_animation_id_from_name("player", "run_left", game_state);
    player->run_animations[D_RIGHT] =
        entities_common_animation_id_from_name("player", "run_right", game_state);
    player->run_animations[D_DOWN_LEFT] =
        entities_common_animation_id_from_name("player", "run_front_left", game_state);
    player->run_animations[D_DOWN_RIGHT] =
        entities_common_animation_id_from_name("player", "run_front_right", game_state);
    player->run_animations[D_UP_LEFT] =
        entities_common_animation_id_from_name("player", "run_back_left", game_state);
    player->run_animations[D_UP_RIGHT] =
        entities_common_animation_id_from_name("player", "run_back_right", game_state);
}

static void prepare_goalie_gfx(ComponentPlayer *player, ComponentGoalie *goalie,
                               GameState *game_state) {
    // Stand frames
    player->stand_frames[D_DOWN] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_0")->id;
    player->stand_frames[D_UP] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_0")->id;
    player->stand_frames[D_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_left_0")->id;
    player->stand_frames[D_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_right_0")->id;
    player->stand_frames[D_DOWN_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_left_0")
            ->id;
    player->stand_frames[D_DOWN_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_right_0")
            ->id;
    player->stand_frames[D_UP_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_left_0")
            ->id;
    player->stand_frames[D_UP_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_right_0")
            ->id;

    // Running animations
    player->run_animations[D_DOWN] =
        entities_common_animation_id_from_name("goalkeeper", "run_front", game_state);
    player->run_animations[D_UP] =
        entities_common_animation_id_from_name("goalkeeper", "run_back", game_state);
    player->run_animations[D_LEFT] =
        entities_common_animation_id_from_name("goalkeeper", "run_left", game_state);
    player->run_animations[D_RIGHT] =
        entities_common_animation_id_from_name("goalkeeper", "run_right", game_state);
    player->run_animations[D_DOWN_LEFT] =
        entities_common_animation_id_from_name("goalkeeper", "run_front_left", game_state);
    player->run_animations[D_DOWN_RIGHT] =
        entities_common_animation_id_from_name("goalkeeper", "run_front_right", game_state);
    player->run_animations[D_UP_LEFT] =
        entities_common_animation_id_from_name("goalkeeper", "run_back_left", game_state);
    player->run_animations[D_UP_RIGHT] =
        entities_common_animation_id_from_name("goalkeeper", "run_back_right", game_state);

    // Save frames
    goalie->save_frames[SFS_UP][SFT_WITHOUT_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_save_front_0")
            ->id;
    goalie->save_frames[SFS_UP][SFT_WITH_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
                                        "goalkeeper_save_ball_front_0")
            ->id;
    goalie->save_frames[SFS_DOWN][SFT_WITHOUT_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_save_back_0")
            ->id;
    goalie->save_frames[SFS_DOWN][SFT_WITH_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
                                        "goalkeeper_save_ball_back_0")
            ->id;

    // Diving animations
    goalie->dive_animations[SFS_UP][SFD_LEFT] =
        entities_common_animation_id_from_name("goalkeeper", "save_front_left", game_state);
    goalie->dive_animations[SFS_UP][SFD_RIGHT] =
        entities_common_animation_id_from_name("goalkeeper", "save_front_right", game_state);
    goalie->dive_animations[SFS_DOWN][SFD_LEFT] =
        entities_common_animation_id_from_name("goalkeeper", "save_back_left", game_state);
    goalie->dive_animations[SFS_DOWN][SFD_RIGHT] =
        entities_common_animation_id_from_name("goalkeeper", "save_back_right", game_state);
}
