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
    sprite->pivot_override = (v2){.x = 0.5, .y = 0.65};
    sprite->layer = SL_PLAYERS_AND_BALL;
    sprite->draw_shadow = false;
    // sprite->shadow_color = BLACK;
    // sprite->shadow_color.a = 100;
    // sprite->shadow_offset = (v2){.x = 0.5, .y = 0.5};
    // sprite->shadow_scale = (v2){.x = 0.6, .y = 1};

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

        collider->num_colliders = 3;
        collider->colliders = memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL],
                                                sizeof(Collider) * collider->num_colliders);
        collider->colliders[1].shape = CS_CIRCLE;
        collider->colliders[1].props_circle.offset = (v2){.y = 0};
        collider->colliders[1].props_circle.radius = 6;

        collider->colliders[2].shape = CS_RECTANGLE;
        collider->colliders[2].props_rectangle.offset = (v2){.x = -10, .y = -5};
        collider->colliders[2].props_rectangle.size.x = 22;
        collider->colliders[2].props_rectangle.size.y = 10;
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
    ComponentTransform *transform =
        entities_component_get_typed(game_state->entity_repository, CT_TRANSFORM, entity->id);
    ComponentPhysics *physics =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS, entity->id);
    ComponentSprite *sprite =
        entities_component_get_typed(game_state->entity_repository, CT_SPRITE, entity->id);
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
        physics->max_velocity = game_state->constants.player_run_max_velocity * 1.25f;
        break;
    }
    case GS_DIVE: {
        physics->max_velocity = goalie->dive_speed * game_state->constants.goalie_dive_max_velocity;
        state_machine->state_data.dive.animation_start_time = game_state->time;
        break;
    }
    case GS_HOLD_BALL: {
        physics->velocity = Vector2Zero();
        physics->accel = Vector2Zero();
        goalie->state_machine->state_data.hold_ball.hold_start_time = game_state->time;
        sprite->frame_id = goalie->save_frames[SS_UP][ST_WITH_BALL];
        break;
    }
    case GS_COUNT:
        break;
    }
}

void entities_goalie_state_machine_on_exit(struct Entity *entity, GameState *game_state) {
    ComponentGoalie *goalie =
        entities_component_get_typed(game_state->entity_repository, CT_GOALIE, entity->id);
    ComponentPhysics *physics =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS, entity->id);
    StateMachineGoalie *state_machine = goalie->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "exiting state: %s",
                   StateMachineGoalieStateString[state_machine->previous_state]);

    switch (state_machine->previous_state) {
    case GS_NONE:
        break;
    case GS_PROTECT_GOAL: {
        break;
    }
    case GS_DIVE: {
        physics->max_velocity = game_state->constants.player_run_max_velocity;
        break;
    }
    case GS_HOLD_BALL: {
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
    goalie->save_frames[SS_UP][ST_WITHOUT_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_0")->id;
    goalie->save_frames[SS_UP][ST_WITH_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_ball_0")
            ->id;
    goalie->save_frames[SS_DOWN][ST_WITHOUT_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_0")->id;
    goalie->save_frames[SS_DOWN][ST_WITH_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_ball_0")
            ->id;

    // Dash frames
    goalie->dash_frames[SS_UP][SD_LEFT][ST_WITHOUT_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
                                        "goalkeeper_dash_front_left_0")
            ->id;
    goalie->dash_frames[SS_UP][SD_LEFT][ST_WITH_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
                                        "goalkeeper_dash_front_left_1")
            ->id;
    goalie->dash_frames[SS_UP][SD_RIGHT][ST_WITHOUT_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
                                        "goalkeeper_dash_front_right_0")
            ->id;
    goalie->dash_frames[SS_UP][SD_RIGHT][ST_WITH_BALL] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
                                        "goalkeeper_dash_front_right_1")
            ->id;

    // Diving animations
    goalie->dive_animations_start[SS_UP][SD_LEFT][SH_HIGH] = entities_common_animation_id_from_name(
        "goalkeeper", "dive_front_high_left_start", game_state);
    goalie->dive_animations_start[SS_UP][SD_RIGHT][SH_HIGH] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_high_right_start",
                                               game_state);
    goalie->dive_animations_start[SS_UP][SD_LEFT][SH_LOW] = entities_common_animation_id_from_name(
        "goalkeeper", "dive_front_low_left_start", game_state);
    goalie->dive_animations_start[SS_UP][SD_RIGHT][SH_LOW] = entities_common_animation_id_from_name(
        "goalkeeper", "dive_front_low_right_start", game_state);

    goalie->dive_animations_end[SS_UP][SD_LEFT][SH_HIGH][ST_WITHOUT_BALL] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_high_left_end",
                                               game_state);
    goalie->dive_animations_end[SS_UP][SD_RIGHT][SH_HIGH][ST_WITHOUT_BALL] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_high_right_end",
                                               game_state);
    goalie->dive_animations_end[SS_UP][SD_LEFT][SH_LOW][ST_WITHOUT_BALL] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_low_left_end", game_state);
    goalie->dive_animations_end[SS_UP][SD_RIGHT][SH_LOW][ST_WITHOUT_BALL] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_low_right_end",
                                               game_state);
    goalie->dive_animations_end[SS_UP][SD_LEFT][SH_HIGH][ST_WITH_BALL] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_high_left_ball_end",
                                               game_state);
    goalie->dive_animations_end[SS_UP][SD_RIGHT][SH_HIGH][ST_WITH_BALL] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_high_right_ball_end",
                                               game_state);
    goalie->dive_animations_end[SS_UP][SD_LEFT][SH_LOW][ST_WITH_BALL] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_low_left_ball_end",
                                               game_state);
    goalie->dive_animations_end[SS_UP][SD_RIGHT][SH_LOW][ST_WITH_BALL] =
        entities_common_animation_id_from_name("goalkeeper", "dive_front_low_right_ball_end",
                                               game_state);
    // goalie->dive_animations[SS_DOWN][SD_LEFT] =
    //     entities_common_animation_id_from_name("goalkeeper", "save_back_left", game_state);
    // goalie->dive_animations[SS_DOWN][SD_RIGHT] =
    //     entities_common_animation_id_from_name("goalkeeper", "save_back_right", game_state);
}
