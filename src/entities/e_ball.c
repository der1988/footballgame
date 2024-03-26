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

ImplementStateMachineFunctions(Ball, ball);

void ball_apply_steering(Vector2 steering, ComponentPhysics *physics, DeltaTime dt);
void ball_apply_velocity(ComponentTransform *transform, ComponentPhysics *physics, DeltaTime dt);

struct Entity *entities_ball_create(GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_BALL, "ball_%02d");

    // Initialise state machine
    StateMachineBall *state_machine =
        memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(StateMachineBall));
    state_machine->owner_entity = entity->id;
    state_machine->previous_state = BS_NONE;
    state_machine->current_state = BS_IN_PLAY;
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
    component->physics.mass = 0.2f;
    component->physics.velocity = (Vector2){0};
    component->physics.accel = (Vector2){0};

    // Initialise sprite
    component = entities_component_create(game_state->entity_repository, CT_SPRITE, entity->id);
    char *ball = "ball_0";
    AssetsSpriteFrame *sprite_frame =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, ball);
    assert(sprite_frame != NULL);
    component->sprite.frame_id = sprite_frame->id;
    component->sprite.color = WHITE;
    component->sprite.flipX = false;
    component->sprite.flipY = false;
    component->sprite.layer = SL_PLAYERS + 1;

    // Initialise action
    component = entities_component_create(game_state->entity_repository, CT_ACTION, entity->id);

    float ball_height = 0.0f;
    Vector3 ball_3d_pos = Vector3Add(Vector3Zero(), (Vector3){.y = ball_height});
    Vector2 ball_2d_pos = {.x = 0, .y = 0};
    Vector2 ball_2d_shadow_pos = {.x = 0, .y = 0};
    float ball_size = 5.0f;
    Vector3 gravity_force = {.y = -9.81f};
    Vector3 ball_vel = Vector3Zero();
    Vector3 ball_accel = Vector3Zero();
    // Coefficient of restitution
    float ball_cr = 0.4f;
    float ball_drag_in_air = 0.6f;
    float ball_drag_on_pitch = 1.5f;
    float ball_run_force = 175.0f;
    float ball_control_force = 20.0f;
    float ball_shot_force = 850.0f;
    float ball_shot_height = 0.3f;
    bool ball_shot = false;
    float ball_shot_time = 0;
    float ball_after_touch_delay_time = 0.1f;
    float ball_after_touch_max_time = 0.5f;
    float ball_after_touch_force = 15.0f;
    Vector3 ball_after_touch_direction = {0};
    float ball_at_height = 0;
    Vector2 ball_at_side = {0};

    state_machine_ball_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "ball created");

    return entity;
}

void entities_ball_update(Entity *entity, GameState *game_state, Input *input) {
    ComponentTransform *transform =
        &entities_component_get(game_state->entity_repository, CT_TRANSFORM, entity->id)->transform;
    ComponentPhysics *physics =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS, entity->id)->physics;
    ComponentAction *action =
        &entities_component_get(game_state->entity_repository, CT_ACTION, entity->id)->action;
    ComponentSprite *sprite =
        &entities_component_get(game_state->entity_repository, CT_SPRITE, entity->id)->sprite;
    Component *component =
        entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    StateMachineBall *state_machine = component->state_machine.ptr;

    Vector2 steering = (Vector2){.x = input->left_stick.x, .y = input->left_stick.y};

    switch (state_machine->current_state) {
    case BS_NONE:
        break;
    case BS_IN_PLAY: {
        // if (steering.x != 0.0f) {
        //     state_machine_ball_change_state_to(state_machine, SS_RUN, game_state);
        // }
        // physics->velocity.x *= 0.9f;
        // sprite->flipX = physics->velocity.x > 0.0f;
        // ball_apply_velocity(transform, physics, game_state->dt);
    } break;
    case BS_OUT_OF_PLAY: {
        // if (steering.x == 0.0f) {
        //     state_machine_ball_change_state_to(state_machine, SS_STAND, game_state);
        // }
        // sprite->flipX = steering.x > 0.0f;
        // ball_apply_steering(steering, physics, human, game_state->dt);
        // ball_apply_velocity(transform, physics, game_state->dt);
    } break;
    case BS_COUNT:
        break;
    }
}

void ball_apply_steering(Vector2 steering, ComponentPhysics *physics, DeltaTime dt) {
    Vector2 force_to_apply = (Vector2){.x = steering.x * 200.0f, .y = 0};
    physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

    // Calculate velocity
    physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, dt));
    // physics->velocity.x = Clamp(physics->velocity.x, -human->max_velocity, human->max_velocity);
    // physics->velocity.y = Clamp(physics->velocity.y, -human->max_velocity, human->max_velocity);
}

void ball_apply_velocity(ComponentTransform *transform, ComponentPhysics *physics, DeltaTime dt) {

    transform->position = Vector2Add(transform->position, Vector2Scale(physics->velocity, dt));
}

void state_machine_ball_on_enter(struct Entity *entity, GameState *game_state) {
    ComponentPhysics *physics =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS, entity->id)->physics;
    ComponentSprite *sprite =
        &entities_component_get(game_state->entity_repository, CT_SPRITE, entity->id)->sprite;
    Component *component =
        entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    StateMachineBall *state_machine = component->state_machine.ptr;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "entering state: %s",
                   StateMachineBallStateString[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(StateMachineBallStateData));

    switch (state_machine->current_state) {
    case BS_NONE:
        break;
    case BS_IN_PLAY: {
        // RunningActionIdx running_action_idx =
        //     components_action_run(entity->id, human->action_stand, game_state);
        // assert(running_action_idx >= 0);
        // state_machine->state_data.stand.running_action_idx = running_action_idx;
    } break;
    case BS_OUT_OF_PLAY: {
        // RunningActionIdx running_action_idx =
        //     components_action_run(entity->id, human->action_run, game_state);
        // assert(running_action_idx >= 0);
        // state_machine->state_data.run.running_action_idx = running_action_idx;
    } break;
    case BS_COUNT:
        break;
    }
}

void state_machine_ball_on_exit(struct Entity *entity, GameState *game_state) {
    Component *component =
        entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    StateMachineBall *state_machine = component->state_machine.ptr;
    ComponentPhysics *physics =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS, entity->id)->physics;
    ComponentSprite *sprite =
        &entities_component_get(game_state->entity_repository, CT_SPRITE, entity->id)->sprite;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "exiting state: %s",
                   StateMachineBallStateString[state_machine->previous_state]);

    switch (state_machine->previous_state) {
    case BS_NONE:
        break;
    case BS_IN_PLAY: {
        // components_action_stop(entity->id, state_machine->state_data.stand.running_action_idx,
        //                        game_state);
    } break;
    case BS_OUT_OF_PLAY: {
        // components_action_stop(entity->id, state_machine->state_data.run.running_action_idx,
        //                        game_state);
        break;
    }
    case BS_COUNT:
        break;
    }
}

// void player_prepare_gfx(ComponentHuman *human, GameState *game_state) {
//     // Stand frames
//     human->stand_frames[PD_DOWN] =
//         assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_0")->id;
//     human->stand_frames[PD_UP] =
//         assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_0")->id;
//     human->stand_frames[PD_LEFT] =
//         assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_left_0")->id;
//     human->stand_frames[PD_RIGHT] =
//         assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_right_0")->id;
//     human->stand_frames[PD_DOWN_LEFT] =
//         assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
//         "player_front_left_0")->id;
//     human->stand_frames[PD_DOWN_RIGHT] =
//         assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
//         "player_front_right_0")->id;
//     human->stand_frames[PD_UP_LEFT] =
//         assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
//         "player_back_left_0")->id;
//     human->stand_frames[PD_UP_RIGHT] =
//         assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
//         "player_back_right_0")->id;
//
//     // Running actions
//     human->run_actions[PD_DOWN] = build_action_from_animation("run_front", game_state);
//     human->run_actions[PD_UP] = build_action_from_animation("run_back", game_state);
//     human->run_actions[PD_LEFT] = build_action_from_animation("run_left", game_state);
//     human->run_actions[PD_RIGHT] = build_action_from_animation("run_right", game_state);
//     human->run_actions[PD_DOWN_LEFT] = build_action_from_animation("run_front_left", game_state);
//     human->run_actions[PD_DOWN_RIGHT] = build_action_from_animation("run_front_right",
//     game_state); human->run_actions[PD_UP_LEFT] = build_action_from_animation("run_back_left",
//     game_state); human->run_actions[PD_UP_RIGHT] = build_action_from_animation("run_back_right",
//     game_state);
// }
