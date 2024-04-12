#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "assets.h"
#include "input.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

typedef struct {
    v2 steering;
    b32 shot;
    b32 pass;
    f32 sprint;
} PlayerControls;

void systems_player(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentForEach(entity_repository, base_component, idx) {
        Component *base_transform =
            entities_component_get(game_state->entity_repository, CT_TRANSFORM, idx);
        Component *base_physics =
            entities_component_get(game_state->entity_repository, CT_PHYSICS, idx);
        Component *base_sprite =
            entities_component_get(game_state->entity_repository, CT_SPRITE, idx);
        Component *base_player =
            entities_component_get(game_state->entity_repository, CT_PLAYER, idx);
        Component *base_collider =
            entities_component_get(game_state->entity_repository, CT_COLLIDER, idx);
        Component *base_state_machine =
            entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, idx);
        if (entity_repository->entities[idx].alive && base_transform->header.enabled &&
            base_physics->header.enabled && base_player->header.enabled &&
            base_collider->header.enabled && base_state_machine->header.enabled &&
            base_sprite->header.enabled) {

            ComponentTransform *transform = &base_transform->transform;
            ComponentPhysics *physics = &base_physics->physics;
            ComponentCollider *collider = &base_collider->collider;
            ComponentSprite *sprite = &base_sprite->sprite;
            ComponentPlayer *player = &base_player->player;

            StateMachinePlayer *state_machine = base_state_machine->state_machine.ptr;

            PlayerControls player_controls = {0};
            switch (player->controller_type) {
            case PCT_HUMAN: {
                player_controls.steering = Vector2ClampValue(input->left_stick, -1, 1);
                player_controls.shot = input->buttons[IBT_BUTTON_LEFT].down;
                player_controls.pass = input->buttons[IBT_BUTTON_DOWN].down;
                player_controls.sprint = (input->right_trigger + 1) / 2.0f;
                break;
            }
            case PCT_AI: {
                player_controls.steering = (v2){.y = 0};
                break;
            }
            }
            Vector2 force_to_apply = Vector2Zero();

            switch (state_machine->current_state) {
            case PS_NONE:
                break;
            case PS_RUN: {
                physics->max_velocity = game_state->constants.player_run_max_velocity;

                // Run force
                force_to_apply = Vector2Scale(Vector2Normalize(player_controls.steering),
                                              game_state->constants.player_movement_force);

                f32 current_velocity = Vector2Length(physics->velocity);

                // Apply drag if no steering applied
                f32 steering_magnitude = Vector2Length(player_controls.steering);
                if (steering_magnitude < 0.1f) {
                    if (current_velocity < 5) {
                        physics->velocity = Vector2Zero();
                    } else {
                        Vector2 drag =
                            Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
                                         game_state->constants.player_movement_drag);
                        force_to_apply = Vector2Add(force_to_apply, drag);
                    }
                }

                // Bring speed back smoothly if decreasing sprint
                if (current_velocity > game_state->constants.player_control_max_velocity +
                                           (game_state->constants.player_run_max_velocity -
                                            game_state->constants.player_control_max_velocity) *
                                               player_controls.sprint) {
                    Vector2 parachute =
                        Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
                                     game_state->constants.player_movement_parachute);
                    force_to_apply = Vector2Add(force_to_apply, parachute);
                }

                entities_player_apply_force(force_to_apply, physics, game_state->dt);
                entities_player_apply_velocity(&base_transform->transform, physics, game_state->dt);

                ComponentTransform *ball_transform =
                    &entities_component_get(game_state->entity_repository, CT_TRANSFORM,
                                            game_state->vars.ball_entity_id)
                         ->transform;
                ComponentPhysics3d *ball_physics_3d =
                    &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D,
                                            game_state->vars.ball_entity_id)
                         ->physics_3d;
                ComponentCollider *ball_collider =
                    &entities_component_get(game_state->entity_repository, CT_COLLIDER,
                                            game_state->vars.ball_entity_id)
                         ->collider;
                ComponentBall *ball =
                    &entities_component_get(game_state->entity_repository, CT_BALL,
                                            game_state->vars.ball_entity_id)
                         ->ball;

                b32 is_colliding_with_ball =
                    CheckCollisionCircles(transform->position, collider->props.circle.radius,
                                          ball_transform->position,
                                          ball_collider->props.circle.radius) &&
                    ball_transform->virtual_height < 40.0f;

                // TODO: this logic needs to be moved to ball system
                // Here we should just call some ball entity functions to book a shoot or assert
                // control over the ball or something
                if (is_colliding_with_ball && (game_state->time - player->last_shot) > 0.1f) {
                    Vector3 ball_position_3d =
                        V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
                    Vector3 player_on_the_ball_velocity = V2_TO_V3(physics->velocity, 0);
                    if (player_controls.shot && Vector2Length(player_controls.steering) > 0.1f) {
                        // Shot
                        // We use acceleration
                        player->last_shot = game_state->time;
                        ball_physics_3d->accel = Vector3Add(
                            ball_physics_3d->accel,
                            Vector3Scale(
                                Vector3Add(Vector3Normalize(V2_TO_V3(player_controls.steering, 0)),
                                           (Vector3){.x = 0,
                                                     .y = game_state->constants.ball_shot_height,
                                                     .z = 0}),
                                game_state->constants.ball_shot_force));
                        ball->subject_to_after_touch = true;
                        ball->shot_time = game_state->time;
                    } else if (Vector2Length(player_controls.steering) > 0.01f) {
                        // For ball control we use velocity directly because it's easier and produce
                        // better results
                        if (player_controls.sprint > 0.1f) {
                            // Run control
                            ball_physics_3d->velocity = Vector3Add(
                                player_on_the_ball_velocity,
                                Vector3Scale(Vector3Add(Vector3Normalize(
                                                            V2_TO_V3(player_controls.steering, 0)),
                                                        (v3){.y = 0.5f}),
                                             game_state->constants.ball_run_force *
                                                 (0.2f + 0.8f * player_controls.sprint)));
                        } else {
                            // Precision control
                            ball_physics_3d->velocity = Vector3Add(
                                player_on_the_ball_velocity,
                                Vector3Scale(
                                    Vector3Normalize(Vector3Subtract(
                                        V2_TO_V3(
                                            Vector2Add(
                                                transform->position,
                                                Vector2Scale(Vector2Normalize(physics->velocity),
                                                             collider->props.circle.radius * 0.9f)),
                                            0),
                                        ball_position_3d)),
                                    game_state->constants.ball_control_force));
                        }
                    }
                }

                // After touch
                // TODO: this has to be refactored to use acceleration, if we decide to keep
                // it...
                if (ball->subject_to_after_touch &&
                    (game_state->time - ball->shot_time) >
                        game_state->constants.ball_after_touch_delay_time &&
                    (game_state->time - ball->shot_time) <
                        game_state->constants.ball_after_touch_max_time) {
                    v2 after_touch_input = player_controls.steering;
                    v2 ball_2d_vel = V3_TO_V2(ball_physics_3d->velocity);
                    v2 ball_2d_vel_perp = {.x = ball_2d_vel.y, .y = -ball_2d_vel.x};
                    f32 after_touch_height =
                        Clamp(Vector2ProjectScalar(ball_2d_vel, after_touch_input), -1.0f, 0.4f);
                    v2 after_touch_side = Vector2Project(ball_2d_vel_perp, after_touch_input);
                    v3 after_touch_direction = (v3){
                        .x = after_touch_side.x, .y = -after_touch_height, .z = after_touch_side.y};
                    ball_physics_3d->velocity =
                        Vector3Add(ball_physics_3d->velocity,
                                   Vector3Scale(after_touch_direction,
                                                game_state->constants.ball_after_touch_force));
                }

                // Calculate frame to display
                if (Vector2Length(physics->velocity) > 0.01f) {
                    Direction pd = entities_common_calculate_direction(physics->velocity);
                    AssetsAnimation *animation = assets_animation_get_by_id(
                        game_state->animation_cache, base_player->player.run_animations[pd]);
                    f32 max_player_velocity = game_state->constants.player_run_max_velocity;
                    if ((game_state->time - player->last_frame_update_time) >
                        (animation->delay *
                         (2 - Vector2Length(physics->velocity) / max_player_velocity))) {
                        player->last_frame_update_time = game_state->time;
                        u16 new_frame_id =
                            (player->last_running_frame_idx + 1) % animation->frames_count;
                        sprite->frame_id = animation->frames[new_frame_id];
                        player->last_running_frame_idx = new_frame_id;
                    }
                    player->last_direction = pd;
                } else {
                    sprite->frame_id = player->stand_frames[player->last_direction];
                }

                break;
            }
            case PS_COUNT:
                break;
            }
        }
    }
}
