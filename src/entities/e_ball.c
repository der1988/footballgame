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
    component->transform.scale = Vector2One();

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
    ComponentPhysics3d *physics_3d =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D, entity->id)->physics_3d;
    ComponentAction *action =
        &entities_component_get(game_state->entity_repository, CT_ACTION, entity->id)->action;
    ComponentSprite *sprite =
        &entities_component_get(game_state->entity_repository, CT_SPRITE, entity->id)->sprite;
    ComponentBall *ball =
        &entities_component_get(game_state->entity_repository, CT_BALL, entity->id)->ball;
    Component *component =
        entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, entity->id);
    StateMachineBall *state_machine = component->state_machine.ptr;

    Vector2 steering = (Vector2){.x = input->left_stick.x, .y = input->left_stick.y};

    switch (state_machine->current_state) {
    case BS_NONE:
        break;
    case BS_IN_PLAY: {

        Vector3 ball_position_3d = (Vector3) {.x = transform->position.x, .y = transform->virtual_height, .z = transform->position.y};
        if (ball_position_3d.y > 0) {
            // Gravity
            physics_3d->velocity = Vector3Add(physics_3d->velocity, Vector3Scale(game_state->constants.gravity_force, 1));
            // Drag in the air
            physics_3d->velocity.x += physics_3d->velocity.x * -game_state->constants.ball_drag_in_air * game_state->dt;
            physics_3d->velocity.z += physics_3d->velocity.z * -game_state->constants.ball_drag_in_air * game_state->dt;
        } else {
            // Drag on the pitch
            physics_3d->velocity.x += physics_3d->velocity.x * -game_state->constants.ball_drag_on_pitch * game_state->dt;
            physics_3d->velocity.z += physics_3d->velocity.z * -game_state->constants.ball_drag_on_pitch * game_state->dt;
            // if (is_colliding_with_player) {
            //     if (IsKeyDown(KEY_Z)) {
            //         // Shot
            //         ball_vel = Vector3Add(
            //             ball_vel,
            //             Vector3Scale(Vector3Add(Vector3Normalize(player_on_the_ball_force),
            //                                     (Vector3){.x = 0, .y = ball_shot_height, .z = 0}),
            //                          ball_shot_force));
            //         ball_shot = true;
            //         ball_shot_time = GetTime();
            //     } else {
            //         if (IsKeyDown(KEY_X)) {
            //             // Precision control
            //             ball_vel = Vector3Add(
            //                 player_on_the_ball_force,
            //                 Vector3Scale(
            //                     Vector3Normalize(Vector3Subtract(
            //                         (Vector3){.x = player_pos.x, .z = player_pos.y}, ball_3d_pos)),
            //                     ball_control_force));
            //         } else {
            //             // Run control
            //             ball_vel =
            //                 Vector3Add(player_on_the_ball_force,
            //                            Vector3Scale(Vector3Normalize(player_on_the_ball_force),
            //                                         ball_run_force));
            //         }
            //     }
            // }
        }
        // if (ball_shot && (GetTime() - ball_shot_time) > ball_after_touch_delay_time &&
        //     (GetTime() - ball_shot_time) < ball_after_touch_max_time) {
        //     // After touch
        //     Vector2 ball_2d_vel = {.x = ball_vel.x, .y = ball_vel.z};
        //     Vector2 ball_2d_vel_perp = {.x = ball_2d_vel.y, .y = -ball_2d_vel.x};
        //     ball_at_height = Clamp(Vector2ProjectScalar(ball_2d_vel, player_movement), -1.0f, 0.5f);
        //     ball_at_side = Vector2Project(ball_2d_vel_perp, player_movement);
        //     ball_after_touch_direction =
        //         (Vector3){.x = ball_at_side.x, .y = -ball_at_height, .z = ball_at_side.y};
        //     ball_vel = Vector3Add(ball_vel,
        //                           Vector3Scale(ball_after_touch_direction, ball_after_touch_force));
        // }
        //
        // if (is_colliding_with_post && ball_height < 60.0f && !post_already_collided) {
        //     ball_vel = Vector3Reflect(ball_vel,
        //                               Vector3Normalize((Vector3){.x = GetRandomValue(-100, 100),
        //                                                          .z = GetRandomValue(-100, 100)}));
        //     if (Vector3Length(ball_vel) < 10.0f) {
        //         ball_vel = Vector3Scale(Vector3Normalize((Vector3){.x = GetRandomValue(-100, 100),
        //                                                            .z = GetRandomValue(-100, 100)}),
        //                                 100.0f);
        //     }
        // } else {
        //     post_already_collided = false;
        // }

        // Apply velocity to ball
        ball_position_3d = Vector3Add(ball_position_3d, Vector3Scale(physics_3d->velocity, game_state->dt));

        // Make the ball bounce
        if (ball_position_3d.y < 0) {
            ball_position_3d.y = 0;
            physics_3d->velocity.y *= -game_state->constants.ball_cr;
            if (fabsf(physics_3d->velocity.y) < 10.0f) {
                physics_3d->velocity.y = 0;
            }
        }

        // Project position to 2D
        transform->position = (Vector2){.x = ball_position_3d.x, .y = ball_position_3d.z}; 
        transform->virtual_height = ball_position_3d.y;
        // // Ball shadow
        // ball_2d_shadow_pos =
        //     Vector2Add(ball_2d_pos, (Vector2){.x = shadow_x_displacement + ball_height * 0.1f,
        //                                       .y = ball_size + ball_height * 0.1f});
    } break;
    case BS_OUT_OF_PLAY: {
    } break;
    case BS_COUNT:
        break;
    }
}

void state_machine_ball_on_enter(struct Entity *entity, GameState *game_state) {
    ComponentPhysics3d *physics_3d =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D, entity->id)->physics_3d;
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
    ComponentPhysics3d *physics_3d =
        &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D, entity->id)->physics_3d;
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
