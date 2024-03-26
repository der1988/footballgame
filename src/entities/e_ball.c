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
void ball_prepare_gfx(ComponentBall *ball, GameState *game_state);

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

    // Initialise ball
    component = entities_component_create(game_state->entity_repository, CT_BALL, entity->id);

    ball_prepare_gfx(&component->ball, game_state);

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
    } break;
    case BS_OUT_OF_PLAY: {
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

void ball_prepare_gfx(ComponentBall *ball, GameState *game_state) {
    // Roll frames
    ball->roll_frames[0] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "ball_0")->id;
    ball->roll_frames[1] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "ball_1")->id;
    ball->roll_frames[2] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "ball_2")->id;
    ball->roll_frames[3] =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "ball_3")->id;
}
