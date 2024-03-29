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
    component->transform.virtual_height = 300;
    component->transform.scale = (v2){.x = 1.5f, .y = 1.5f};

    // Initialise 3d physics
    component = entities_component_create(game_state->entity_repository, CT_PHYSICS_3D, entity->id);
    component->physics_3d.mass = 0.2f;
    component->physics_3d.velocity = (Vector3){0};
    component->physics_3d.accel = (Vector3){0};

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
    component->sprite.pivot_override = (v2){.x = 0.5, .y = 0.5};
    component->sprite.layer = SL_PLAYERS + 1;

    // Initialise shadow
    component = entities_component_create(game_state->entity_repository, CT_SHADOW, entity->id);
    component->shadow.color = BLACK;
    component->shadow.color.a = 100;
    component->shadow.offset = (v2){.x = 3, .y = 3};
    component->shadow.scale = (v2){.x = 1, .y = 1};

    // Initialise collider
    component = entities_component_create(game_state->entity_repository, CT_COLLIDER, entity->id);
    component->collider.shape = CS_CIRCLE;
    component->collider.props.circle.radius = 5;

    // Initialise action
    component = entities_component_create(game_state->entity_repository, CT_ACTION, entity->id);

    // Initialise ball
    component = entities_component_create(game_state->entity_repository, CT_BALL, entity->id);

    ball_prepare_gfx(&component->ball, game_state);

    entities_ball_state_machine_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "ball created");

    return entity;
}

void entities_ball_state_machine_on_enter(struct Entity *entity, GameState *game_state) {
    ComponentPhysics3d *physics_3d =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D, entity->id)
             ->physics_3d;
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

void entities_ball_state_machine_on_exit(struct Entity *entity, GameState *game_state) {
    Component *component =
        entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    StateMachineBall *state_machine = component->state_machine.ptr;
    ComponentPhysics3d *physics_3d =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D, entity->id)
             ->physics_3d;
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
