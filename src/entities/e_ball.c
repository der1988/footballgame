#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "sprites/sprites.h"
#include "types.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ImplementStateMachineFunctions(Ball, ball);

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
    component->transform.scale = (v2){.x = 1.0f, .y = 1.0f};

    // Initialise 3d physics
    component = entities_component_create(game_state->entity_repository, CT_PHYSICS_3D, entity->id);
    component->physics_3d.mass = 0.43f;
    component->physics_3d.velocity = (Vector3){0};
    component->physics_3d.accel = (Vector3){0};
    component->physics_3d.max_velocity = game_state->constants.ball_max_velocity;

    // Initialise sprite
    component = entities_component_create(game_state->entity_repository, CT_SPRITE, entity->id);
    char *ball = "ball_left_0";
    AssetsSpriteFrame *sprite_frame =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, ball);
    assert(sprite_frame != NULL);
    component->sprite.frame_id = sprite_frame->id;
    component->sprite.color = WHITE;
    component->sprite.flipX = false;
    component->sprite.flipY = false;
    component->sprite.pivot_override = (v2){.x = 0.5, .y = 0.5};
    component->sprite.layer = SL_PLAYERS_AND_BALL;
    component->sprite.draw_shadow = true;
    component->sprite.shadow_color = BLACK;
    component->sprite.shadow_color.a = 100;
    component->sprite.shadow_offset = (v2){.x = 0.05, .y = 0.1};
    component->sprite.shadow_scale = (v2){.x = 1, .y = 1};

    // Initialise collider
    component = entities_component_create(game_state->entity_repository, CT_COLLIDER, entity->id);
    component->collider.shape = CS_CIRCLE;
    component->collider.props.circle.radius = 9;

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
    } break;
    case BS_OUT_OF_PLAY: {
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
        break;
    }
    case BS_COUNT:
        break;
    }
}

void ball_prepare_gfx(ComponentBall *ball, GameState *game_state) {
    // Roll frames
    ball->roll_animations[D_UP] = entities_common_animation_id_from_name("ball", "up", game_state);
    ball->roll_animations[D_DOWN] =
        entities_common_animation_id_from_name("ball", "down", game_state);
    ball->roll_animations[D_LEFT] =
        entities_common_animation_id_from_name("ball", "left", game_state);
    ball->roll_animations[D_RIGHT] =
        entities_common_animation_id_from_name("ball", "right", game_state);
    ball->roll_animations[D_UP_LEFT] =
        entities_common_animation_id_from_name("ball", "up_left", game_state);
    ball->roll_animations[D_UP_RIGHT] =
        entities_common_animation_id_from_name("ball", "up_right", game_state);
    ball->roll_animations[D_DOWN_LEFT] =
        entities_common_animation_id_from_name("ball", "down_left", game_state);
    ball->roll_animations[D_DOWN_RIGHT] =
        entities_common_animation_id_from_name("ball", "down_right", game_state);
}

void entities_ball_control_precision(EntityId ball_entity_id, v2 player_velocity,
                                     v2 position_to_seek, f32 control_quality,
                                     GameState *game_state) {
    ComponentTransform *ball_transform = entities_component_get_internal(
        game_state->entity_repository, CT_TRANSFORM, ball_entity_id);
    ComponentPhysics3d *ball_physics_3d = entities_component_get_internal(
        game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);

    ball_physics_3d->velocity = Vector3Add(
        V2_TO_V3(player_velocity, 0),
        Vector3Scale(Vector3Normalize(Vector3Subtract(
                         V2_TO_V3(position_to_seek, 0),
                         V2_TO_V3(ball_transform->position, ball_transform->virtual_height))),
                     game_state->constants.ball_control_force * control_quality));
}

void entities_ball_control_running(EntityId ball_entity_id, v2 control_direction,
                                   v2 player_velocity, f32 control_quality, f32 sprint,
                                   GameState *game_state) {
    ComponentPhysics3d *ball_physics_3d = entities_component_get_internal(
        game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);
    v3 velocity_to_add = Vector3Scale(
        Vector3Add(Vector3Normalize(V2_TO_V3(control_direction, 0)), (v3){.y = 0.5f}),
        game_state->constants.ball_run_force * (1 - control_quality) * (0.2f + 0.8f * sprint));
    ball_physics_3d->velocity = Vector3Add(V2_TO_V3(player_velocity, 0), velocity_to_add);
}

void entities_ball_shoot(EntityId ball_entity_id, v3 direction, f32 power, GameState *game_state) {
    ComponentPhysics3d *ball_physics_3d = entities_component_get_internal(
        game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);
    ComponentBall *ball =
        entities_component_get_internal(game_state->entity_repository, CT_BALL, ball_entity_id);
    ball_physics_3d->accel = Vector3Add(
        ball_physics_3d->accel,
        Vector3Scale(direction, game_state->constants.ball_shot_force * 0.5f +
                                    power * 0.5f * game_state->constants.ball_shot_force));
    ball->subject_to_after_touch = true;
    ball->shot_time = game_state->time;
}

void entities_ball_short_pass(EntityId ball_entity_id, v3 direction, f32 power,
                              GameState *game_state) {
    ComponentPhysics3d *ball_physics_3d = entities_component_get_internal(
        game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);
    ComponentBall *ball =
        entities_component_get_internal(game_state->entity_repository, CT_BALL, ball_entity_id);
    ball_physics_3d->accel = Vector3Add(
        ball_physics_3d->accel,
        Vector3Scale(direction, game_state->constants.ball_pass_force * 0.5f +
                                    power * 0.5f * game_state->constants.ball_pass_force));
    ball->subject_to_after_touch = false;
}

void entities_ball_long_pass(EntityId ball_entity_id, v3 direction, f32 power,
                             GameState *game_state) {
    ComponentPhysics3d *ball_physics_3d = entities_component_get_internal(
        game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);
    ComponentBall *ball =
        entities_component_get_internal(game_state->entity_repository, CT_BALL, ball_entity_id);
    ball_physics_3d->accel = Vector3Add(
        ball_physics_3d->accel,
        Vector3Scale(direction, game_state->constants.ball_long_pass_force * 0.5f +
                                    power * 0.5f * game_state->constants.ball_long_pass_force));
    ball->subject_to_after_touch = false;
}
