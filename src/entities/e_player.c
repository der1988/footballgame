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

struct Entity *entities_player_create(GameState *game_state, PlayerConfig config) {
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
    collider->shape = CS_CIRCLE;
    collider->props.circle.radius = game_state->constants.player_control_radius;

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
    StateMachinePlayer *state_machine =
        memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(StateMachinePlayer));
    state_machine->owner_entity = entity->id;
    state_machine->previous_state = PS_NONE;
    state_machine->current_state = PS_NORMAL;
    player->state_machine = state_machine;

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

        prepare_goalie_gfx(player, goalie, game_state);
    } else {
        prepare_gfx(player, game_state);
    }

    entities_player_state_machine_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "player created");

    return entity;
}

void entities_player_apply_force(Vector2 force_to_apply, ComponentPhysics *physics, DeltaTime dt) {
    physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

    // Calculate velocity
    physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, dt));
    if (Vector2Length(physics->velocity) > physics->max_velocity) {
        physics->velocity =
            Vector2Scale(Vector2Normalize(physics->velocity), physics->max_velocity);
    }
}

void entities_player_apply_velocity(ComponentTransform *transform, ComponentPhysics *physics,
                                    DeltaTime dt) {
    transform->position = Vector2Add(transform->position, Vector2Scale(physics->velocity, dt));
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
}
