#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "layers.h"
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

struct Entity *entities_ball_create(GameState *game_state, BallConfig config) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_BALL, "ball_%02d");

    // Initialise transform
    ComponentTransform *transform =
        entities_component_create(game_state->entity_repository, CT_TRANSFORM, entity->id);
    transform->position = config.position;
    transform->rotation = 0;
    transform->virtual_height = 100;
    transform->scale = (v2){.x = 0.625, .y = 0.625};

    // Initialise 3d physics
    ComponentPhysics3d *physics_3d =
        entities_component_create(game_state->entity_repository, CT_PHYSICS_3D, entity->id);
    physics_3d->mass = 0.43f;
    physics_3d->velocity = (Vector3){0};
    physics_3d->accel = (Vector3){0};
    physics_3d->max_velocity = game_state->constants.ball_max_velocity;

    // Initialise sprite
    ComponentSprite *sprite =
        entities_component_create(game_state->entity_repository, CT_SPRITE, entity->id);
    char *ball_frame_name = "ball_left_0";
    AssetsSpriteFrame *sprite_frame =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, ball_frame_name);
    assert(sprite_frame != NULL);
    sprite->frame_id = sprite_frame->id;
    sprite->color = WHITE;
    sprite->flipX = false;
    sprite->flipY = false;
    sprite->pivot_override = (v2){.x = 0.5, .y = 0.5};
    sprite->layer = SL_PLAYERS_AND_BALL;
    sprite->draw_shadow = true;
    sprite->shadow_color = BLACK;
    sprite->shadow_color.a = 100;
    sprite->shadow_offset = (v2){.x = 0.05, .y = 0.1};
    sprite->shadow_scale = (v2){.x = 1, .y = 1};

    // Initialise collider
    ComponentCollider *collider =
        entities_component_create(game_state->entity_repository, CT_COLLIDER, entity->id);
    collider->shape = CS_CIRCLE;
    collider->props.circle.radius = 2.5f;

    // Initialise action
    entities_component_create(game_state->entity_repository, CT_ACTION, entity->id);

    // Initialise ball
    ComponentBall *ball =
        entities_component_create(game_state->entity_repository, CT_BALL, entity->id);
    StateMachineBall *state_machine =
        memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(StateMachineBall));
    state_machine->owner_entity = entity->id;
    state_machine->previous_state = BS_NONE;
    state_machine->current_state = BS_IN_PLAY;
    ball->state_machine = state_machine;

    ball_prepare_gfx(ball, game_state);

    entities_ball_state_machine_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "ball created");

    return entity;
}

void entities_ball_state_machine_on_enter(struct Entity *entity, GameState *game_state) {
    ComponentBall *ball =
        entities_component_get_typed(game_state->entity_repository, CT_BALL, entity->id);
    StateMachineBall *state_machine = ball->state_machine;

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
    ComponentBall *ball =
        entities_component_get_typed(game_state->entity_repository, CT_BALL, entity->id);
    StateMachineBall *state_machine = ball->state_machine;

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
    ComponentTransform *ball_transform =
        entities_component_get_typed(game_state->entity_repository, CT_TRANSFORM, ball_entity_id);
    ComponentPhysics3d *ball_physics_3d =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);

    ball_physics_3d->velocity = Vector3Add(
        V2_TO_V3(player_velocity, 0),
        Vector3Scale(Vector3Normalize(Vector3Subtract(
                         V2_TO_V3(position_to_seek, 0),
                         V2_TO_V3(ball_transform->position, ball_transform->virtual_height))),
                     game_state->constants.ball_control_force * control_quality));
}

void entities_ball_control_running(EntityId ball_entity_id, v2 control_direction,
                                   v2 player_velocity, f32 control_quality, GameState *game_state) {
    ComponentPhysics3d *ball_physics_3d =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);
    // Limit angle between requested run direction and current velocity
    if (Vector2Length(player_velocity) > 0.7f * game_state->constants.player_run_max_velocity) {
        f32 angle = Vector2Angle(control_direction, player_velocity) * RAD2DEG;
        if (angle > 5) {
            control_direction = Vector2Rotate(player_velocity, 5);
        } else if (angle < -5) {
            control_direction = Vector2Rotate(player_velocity, -5);
        }
    }
    v3 velocity_to_add =
        Vector3Scale(Vector3Add(Vector3Normalize(V2_TO_V3(control_direction, 0)), (v3){.y = 0.2f}),
                     // TODO: some ball_run_force has to always be applied
                     game_state->constants.ball_run_force * (1 - control_quality));
    ball_physics_3d->velocity = Vector3Add(V2_TO_V3(player_velocity, 0), velocity_to_add);
}

void entities_ball_shoot(EntityId ball_entity_id, v3 direction, f32 power, GameState *game_state) {
    ComponentPhysics3d *ball_physics_3d =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);
    ComponentBall *ball =
        entities_component_get_typed(game_state->entity_repository, CT_BALL, ball_entity_id);
    ball_physics_3d->accel =
        Vector3Add(ball_physics_3d->accel,
                   Vector3Scale(direction, (game_state->constants.ball_shot_force * 0.5f +
                                            power * 0.5f * game_state->constants.ball_shot_force) /
                                               game_state->dt));
    ball->shot_time = game_state->time;
}

void entities_ball_short_pass(EntityId ball_entity_id, v3 direction, f32 power,
                              GameState *game_state) {
    ComponentPhysics3d *ball_physics_3d =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);
    ball_physics_3d->accel =
        Vector3Add(ball_physics_3d->accel,
                   Vector3Scale(direction, (game_state->constants.ball_pass_force * 0.5f +
                                            power * 0.5f * game_state->constants.ball_pass_force) /
                                               game_state->dt));
}

void entities_ball_long_pass(EntityId ball_entity_id, v3 direction, f32 power,
                             GameState *game_state) {
    ComponentPhysics3d *ball_physics_3d =
        entities_component_get_typed(game_state->entity_repository, CT_PHYSICS_3D, ball_entity_id);
    ball_physics_3d->accel = Vector3Add(
        ball_physics_3d->accel,
        Vector3Scale(direction, (game_state->constants.ball_long_pass_force * 0.75f +
                                 power * 0.25f * game_state->constants.ball_long_pass_force) /
                                    game_state->dt));
}
