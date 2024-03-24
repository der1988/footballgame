#include "../actions/actions.h"
#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "game_constants.h"
#include "input/input.h"
#include "sprites/sprites.h"
#include "types.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

ImplementStateMachineFunctions(Player, player);

void player_apply_steering(Vector2 steering, ComponentPhysics *physics, ComponentHuman *human,
                           DeltaTime dt);
void player_apply_velocity(ComponentTransform *transform, ComponentPhysics *physics,
                           ComponentHuman *human, DeltaTime dt);
void player_prepare_gfx(ComponentHuman *human, GameState *game_state);

struct Entity *entities_player_create(GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_PLAYER, "player_%02d");

    // Initialise state machine
    StateMachinePlayer *state_machine =
        memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(StateMachinePlayer));
    state_machine->owner_entity = entity->id;
    state_machine->previous_state = PS_NONE;
    state_machine->current_state = PS_STAND;
    Component *component =
        entities_component_create(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    component->state_machine.ptr = state_machine;

    // Initialise transform
    component = entities_component_create(game_state->entity_repository, CT_TRANSFORM, entity->id);
    component->transform.position = (Vector2){(f32)GetScreenWidth() / 2, -10};
    component->transform.rotation = 0;
    component->transform.scale = Vector2One();

    // Initialise physics
    component = entities_component_create(game_state->entity_repository, CT_PHYSICS, entity->id);
    component->physics.mass = 1;
    component->physics.velocity = (Vector2){0};
    component->physics.accel = (Vector2){0};

    // Initialise sprite
    component = entities_component_create(game_state->entity_repository, CT_SPRITE, entity->id);
    char *player_front_0 = "player_front_0";
    AssetsSpriteFrame *sprite_frame =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, player_front_0);
    assert(sprite_frame != NULL);
    component->sprite.frame_id = sprite_frame->id;
    component->sprite.color = WHITE;
    component->sprite.flipX = false;
    component->sprite.flipY = false;
    component->sprite.layer = SL_PLAYERS;

    // Initialise human
    ComponentHuman *human =
        &entities_component_create(game_state->entity_repository, CT_HUMAN, entity->id)->human;
    human->max_velocity = 100.0;

    // Initialise action
    component = entities_component_create(game_state->entity_repository, CT_ACTION, entity->id);

    player_prepare_gfx(human, game_state);

    state_machine_player_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "player created");

    return entity;
}

void entities_player_update(Entity *entity, GameState *game_state, Input *input) {
    ComponentTransform *transform =
        &entities_component_get(game_state->entity_repository, CT_TRANSFORM, entity->id)->transform;
    ComponentPhysics *physics =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS, entity->id)->physics;
    ComponentAction *action =
        &entities_component_get(game_state->entity_repository, CT_ACTION, entity->id)->action;
    ComponentHuman *human =
        &entities_component_get(game_state->entity_repository, CT_HUMAN, entity->id)->human;
    ComponentSprite *sprite =
        &entities_component_get(game_state->entity_repository, CT_SPRITE, entity->id)->sprite;
    Component *component =
        entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    StateMachinePlayer *state_machine = component->state_machine.ptr;

    Vector2 steering = (Vector2){.x = input->left_stick.x, .y = input->left_stick.y};

    switch (state_machine->current_state) {
    case PS_NONE:
        break;
    case PS_STAND:
        break;
    case PS_RUN: {
        player_apply_steering(steering, physics, human, game_state->dt);

        player_apply_velocity(transform, physics, human, game_state->dt);
    } break;
    case PS_COUNT:
        break;
    }
}

void player_apply_steering(Vector2 steering, ComponentPhysics *physics, ComponentHuman *human,
                           DeltaTime dt) {
    Vector2 force_to_apply = Vector2Scale(steering, 200.0f);
    physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

    DrawText(TextFormat("steering (%.2f, %.2f)", force_to_apply.x, force_to_apply.y), 1200.0f,
             10.0f, 16, WHITE);

    // Calculate velocity
    physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, dt));
    // physics->velocity.x =
    //     Clamp(physics->velocity.x, -airborne->max_air_velocity, airborne->max_air_velocity);
    // physics->velocity.y =
    //     Clamp(physics->velocity.y, -airborne->max_climb_velocity, airborne->max_dive_velocity);
}

void player_apply_velocity(ComponentTransform *transform, ComponentPhysics *physics,
                           ComponentHuman *human, DeltaTime dt) {
    // Vector2 velocity = (Vector2){
    //     .x = airborne->movingLeft ? -airborne->base_velocity_x : airborne->base_velocity_x, .y =
    //     0};
    // airborne->real_velocity = Vector2Add(velocity, physics->velocity);
    //
    // transform->position =
    //     Vector2Add(transform->position, Vector2Scale(airborne->real_velocity, dt));
}

void state_machine_player_on_enter(struct Entity *entity, GameState *game_state) {
    ComponentPhysics *physics =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS, entity->id)->physics;
    ComponentSprite *sprite =
        &entities_component_get(game_state->entity_repository, CT_SPRITE, entity->id)->sprite;
    ComponentHuman *human =
        &entities_component_get(game_state->entity_repository, CT_HUMAN, entity->id)->human;
    Component *component =
        entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    StateMachinePlayer *state_machine = component->state_machine.ptr;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "entering state: %s",
                   StateMachinePlayerStateString[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(StateMachinePlayerStateData));

    switch (state_machine->current_state) {
    case PS_NONE:
        break;
    case PS_STAND: {
        sprite->frame_id = human->stand_frames[PD_DOWN];
    } break;
    case PS_RUN:
        break;
    case PS_COUNT:
        break;
    }
}

void state_machine_player_on_exit(struct Entity *entity, GameState *game_state) {
    Component *component =
        entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    StateMachinePlayer *state_machine = component->state_machine.ptr;
    ComponentPhysics *physics =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS, entity->id)->physics;
    ComponentSprite *sprite =
        &entities_component_get(game_state->entity_repository, CT_SPRITE, entity->id)->sprite;
    ComponentHuman *human =
        &entities_component_get(game_state->entity_repository, CT_HUMAN, entity->id)->human;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "exiting state: %s",
                   StateMachinePlayerStateString[state_machine->previous_state]);

    switch (state_machine->previous_state) {
    case PS_NONE:
        break;
    case PS_STAND:
        break;
    case PS_RUN: {
        break;
    }
    case PS_COUNT:
        break;
    }
}

struct Action *build_action_from_animation(char *animation_name, GameState *game_state) {
    MemoryArenaHandle *memory_handle_level = game_state->memory_by_lifetime[MAL_LEVEL];

    // Scratch buffer
    char *final_animation = memory_arena_push(game_state->memory_by_lifetime[MAL_FRAME], 30);

    snprintf(final_animation, 30, "player_%s", animation_name);
    AssetsAnimation *animation =
        assets_animation_get_by_name(game_state->animation_cache, final_animation);
    ActionPlayAnimation *animation_action =
        actions_play_animation_create(animation, true, 1.0f, memory_handle_level);

    return (struct Action *)animation_action;
}

void player_prepare_gfx(ComponentHuman *human, GameState *game_state) {
    // Stand frames
    human->stand_frames[PD_DOWN] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_0")->id;
    human->stand_frames[PD_UP] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_0")->id;
    human->stand_frames[PD_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_left_0")->id;
    human->stand_frames[PD_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_right_0")->id;
    human->stand_frames[PD_DOWN_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_left_0")->id;
    human->stand_frames[PD_DOWN_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_right_0")->id;
    human->stand_frames[PD_UP_LEFT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_left_0")->id;
    human->stand_frames[PD_UP_RIGHT] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_right_0")->id;

    // Running actions
    human->run_actions[PD_DOWN] = build_action_from_animation("run_front", game_state);
    human->run_actions[PD_UP] = build_action_from_animation("run_back", game_state);
    human->run_actions[PD_LEFT] = build_action_from_animation("run_left", game_state);
    human->run_actions[PD_RIGHT] = build_action_from_animation("run_right", game_state);
    human->run_actions[PD_DOWN_LEFT] = build_action_from_animation("run_front_left", game_state);
    human->run_actions[PD_DOWN_RIGHT] = build_action_from_animation("run_front_right", game_state);
    human->run_actions[PD_UP_LEFT] = build_action_from_animation("run_back_left", game_state);
    human->run_actions[PD_UP_RIGHT] = build_action_from_animation("run_back_right", game_state);
}
