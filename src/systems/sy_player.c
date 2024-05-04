#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "assets.h"
#include "input.h"
#include "layers/layers.h"
#include "memory.h"
#include "steering.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

typedef struct {
    v2 steering;
    b32 sprint;
    f32 shot;
    f32 pass;
    f32 long_pass;
} PlayerControls;

typedef struct {
    b32 exists;
    EntityId id;
    f32 distance_from_the_ball;
    ComponentPlayer *player;
    v2 position;
    v2 velocity;
    PlayerControls *controls;
    ComponentCollider *collider;
} PlayerOnTheBall;

void systems_player(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentTransform *ball_transform = entities_component_get_typed(
        game_state->entity_repository, CT_TRANSFORM, game_state->vars.ball_entity_id);
    ComponentPhysics3d *ball_physics_3d = entities_component_get_typed(
        game_state->entity_repository, CT_PHYSICS_3D, game_state->vars.ball_entity_id);
    ComponentCollider *ball_collider = entities_component_get_typed(
        game_state->entity_repository, CT_COLLIDER, game_state->vars.ball_entity_id);
    ComponentBall *ball = entities_component_get_typed(game_state->entity_repository, CT_BALL,
                                                       game_state->vars.ball_entity_id);

    PlayerOnTheBall player_on_the_ball = {0};
    player_on_the_ball.distance_from_the_ball = 9999999.0f;

    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(entity_repository, entity_id, 5,
                                                        CT_TRANSFORM, CT_PHYSICS, CT_SPRITE,
                                                        CT_PLAYER, CT_COLLIDER)) {

            ComponentTransform *transform = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_TRANSFORM, entity_id);
            ComponentPhysics *physics =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_PHYSICS, entity_id);
            ComponentSprite *sprite =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_SPRITE, entity_id);
            ComponentPlayer *player =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_PLAYER, entity_id);
            ComponentCollider *collider = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_COLLIDER, entity_id);

            StateMachinePlayer *state_machine = player->state_machine;

            collider->props.circle.radius = game_state->constants.player_control_radius;

            PlayerControls *player_controls = memory_arena_push(
                game_state->memory_by_lifetime[MAL_UPDATE], sizeof(PlayerControls));

            if (game_state->vars.p1.player_entity_id == entity_id) {
                player_controls->steering = game_state->vars.p1.controls.normalised_direction;
                player_controls->sprint = game_state->vars.p1.controls.sprint;

                if (game_state->vars.p1.controls.charge.ready) {
                    switch (game_state->vars.p1.controls.charge.type) {
                    case ACT_SHOOT: {
                        player_controls->shot = game_state->vars.p1.controls.charge.power;
                        break;
                    }
                    case ACT_PASS: {
                        player_controls->pass = game_state->vars.p1.controls.charge.power;
                        break;
                    }
                    case ACT_LONG_PASS: {
                        player_controls->long_pass = game_state->vars.p1.controls.charge.power;
                        break;
                    }
                    }
                }
            } else {
                if (game_state->constants.ai_enabled) {
                    player_controls->steering = steering_pursuit(
                        transform->position, physics->velocity, physics->max_velocity,
                        ball_transform->position, V3_TO_V2(ball_physics_3d->velocity));
                }
            }

            v2 effective_steering = player_controls->steering;
            // If sprint locked or charging kick we seek the ball
            if (player->sprint_locked || game_state->vars.p1.controls.charge.charging) {
                effective_steering = Vector2Normalize(
                    Vector2Subtract(ball_transform->position, transform->position));
            }
            physics->max_velocity = game_state->constants.player_run_max_velocity;
            v2 force_to_apply = Vector2Zero();
            v2 normalized_velocity = Vector2Normalize(physics->velocity);
            f32 steering_magnitude = Vector2Length(effective_steering);
            f32 current_velocity = Vector2Length(physics->velocity);

            switch (state_machine->current_state) {
            case PS_NONE:
                break;
            case PS_RUN: {
                // Run force
                force_to_apply =
                    Vector2Scale(effective_steering, game_state->constants.player_movement_force);

                // Apply drag if no steering applied
                if (steering_magnitude < 0.1f) {
                    if (current_velocity < 3) {
                        physics->velocity = Vector2Zero();
                    } else {
                        Vector2 drag = Vector2Scale(Vector2Negate(normalized_velocity),
                                                    game_state->constants.player_movement_drag);
                        force_to_apply = Vector2Add(force_to_apply, drag);
                    }
                }

                // Bring speed back smoothly if decreasing sprint
                if (current_velocity > game_state->constants.player_control_max_velocity +
                                           (game_state->constants.player_run_max_velocity -
                                            game_state->constants.player_control_max_velocity) *
                                               (player_controls->sprint || player->sprint_locked)) {
                    Vector2 parachute =
                        Vector2Scale(Vector2Negate(normalized_velocity),
                                     game_state->constants.player_movement_force * 1.2f);
                    force_to_apply = Vector2Add(force_to_apply, parachute);
                }

                entities_player_apply_force(force_to_apply, physics, game_state->dt);
                entities_player_apply_velocity(transform, physics, game_state->dt);

                // Velocity has been applied to player after this point

                layers_debug_queue_draw_text(game_state->layer_debug,
                                             TextFormat("Player %02d pos: [%0.2f, %0.2f] vel: %.2f",
                                                        entity_id, transform->position.x,
                                                        transform->position.y,
                                                        Vector2Length(physics->velocity)));

                b32 is_colliding_with_ball =
                    CheckCollisionCircles(transform->position, collider->props.circle.radius,
                                          ball_transform->position,
                                          ball_collider->props.circle.radius) &&
                    // Player height
                    ball_transform->virtual_height < game_state->constants.player_control_height;

                if (is_colliding_with_ball && (game_state->time - player->last_shot) > 0.1f) {
                    f32 distance_from_the_ball =
                        Vector2DistanceSqr(transform->position, ball_transform->position);
                    if (distance_from_the_ball < player_on_the_ball.distance_from_the_ball) {
                        player_on_the_ball.id = entity_id;
                        player_on_the_ball.exists = true;
                        player_on_the_ball.player = player;
                        player_on_the_ball.distance_from_the_ball = distance_from_the_ball;
                        player_on_the_ball.position = transform->position;
                        player_on_the_ball.velocity = physics->velocity;
                        player_on_the_ball.controls = player_controls;
                        player_on_the_ball.collider = collider;
                    }
                }

                // // After touch
                // // TODO: this has to be refactored to use acceleration, if we decide to keep
                // // it...
                // if (ball->subject_to_after_touch &&
                //     (game_state->time - ball->shot_time) >
                //         game_state->constants.ball_after_touch_delay_time &&
                //     (game_state->time - ball->shot_time) <
                //         game_state->constants.ball_after_touch_max_time) {
                //     v2 after_touch_input = player_controls->steering;
                //     v2 ball_2d_vel = V3_TO_V2(ball_physics_3d->velocity);
                //     v2 ball_2d_vel_perp = {.x = ball_2d_vel.y, .y = -ball_2d_vel.x};
                //     f32 after_touch_height =
                //         Clamp(Vector2ProjectScalar(ball_2d_vel, after_touch_input), -1.0f, 0.4f);
                //     v2 after_touch_side = Vector2Project(ball_2d_vel_perp, after_touch_input);
                //     v3 after_touch_direction = (v3){
                //         .x = after_touch_side.x, .y = -after_touch_height, .z =
                //         after_touch_side.y};
                //     ball_physics_3d->velocity =
                //         Vector3Add(ball_physics_3d->velocity,
                //                    Vector3Scale(after_touch_direction,
                //                                 game_state->constants.ball_after_touch_force));
                // }

                // Calculate frame to display
                if (Vector2Length(physics->velocity) > 3) {
                    Direction pd = entities_common_calculate_direction(physics->velocity);
                    AssetsAnimation *animation = assets_animation_get_by_id(
                        game_state->animation_cache, player->run_animations[pd]);
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

                if (entity_id != ball->last_player_on_the_ball_entity_id ||
                    game_state->vars.someone_scored || !game_state->vars.ball_is_playable) {
                    // If this player wasn't the last controlling the ball or sprint is disabled we
                    // can reset sprint lock
                    player->sprint_locked = false;
                }

                break;
            }
            case PS_COUNT:
                break;
            }
        }
    }

    if (player_on_the_ball.exists) {

        b32 kicked_ball = false;
        v2 normalized_velocity = Vector2Normalize(player_on_the_ball.velocity);
        f32 steering_magnitude = Vector2Length(player_on_the_ball.controls->steering);
        ball->last_player_on_the_ball_entity_id = player_on_the_ball.id;
        player_on_the_ball.player->sprint_locked = false;

        if (player_on_the_ball.controls->shot > 0 && steering_magnitude > 0.1f) {
            // Shot
            player_on_the_ball.player->last_shot = game_state->time;
            v2 shot_direction = Vector2Normalize(player_on_the_ball.controls->steering);
            // Limit angle between requested shoot direction and current velocity
            f32 angle = Vector2Angle(player_on_the_ball.velocity, shot_direction);
            f32 max_angle = 100 * DEG2RAD;
            if (angle > max_angle) {
                shot_direction =
                    Vector2Normalize(Vector2Rotate(player_on_the_ball.velocity, max_angle));
            } else if (angle < -max_angle) {
                shot_direction =
                    Vector2Normalize(Vector2Rotate(player_on_the_ball.velocity, -max_angle));
            }
            entities_ball_shoot(game_state->vars.ball_entity_id, shot_direction,
                                player_on_the_ball.controls->shot, game_state);
            kicked_ball = true;
        }
        if (player_on_the_ball.controls->pass > 0 && steering_magnitude > 0.1f) {
            // Pass
            player_on_the_ball.player->last_shot = game_state->time;
            v3 pass_direction =
                Vector3Normalize(V2_TO_V3(player_on_the_ball.controls->steering, 0));
            entities_ball_short_pass(game_state->vars.ball_entity_id, pass_direction,
                                     player_on_the_ball.controls->pass, game_state);
            kicked_ball = true;
        }
        if (player_on_the_ball.controls->long_pass && steering_magnitude > 0.1f) {
            // Long pass
            player_on_the_ball.player->last_shot = game_state->time;
            v2 long_pass_direction = Vector2Normalize(player_on_the_ball.controls->steering);
            // Limit angle between requested shoot direction and current velocity
            f32 angle = Vector2Angle(player_on_the_ball.velocity, long_pass_direction);
            f32 max_angle = 120 * DEG2RAD;
            if (angle > max_angle) {
                long_pass_direction =
                    Vector2Normalize(Vector2Rotate(player_on_the_ball.velocity, max_angle));
            } else if (angle < -max_angle) {
                long_pass_direction =
                    Vector2Normalize(Vector2Rotate(player_on_the_ball.velocity, -max_angle));
            }
            entities_ball_long_pass(game_state->vars.ball_entity_id, long_pass_direction,
                                    player_on_the_ball.controls->long_pass, game_state);
            kicked_ball = true;
        } else if (steering_magnitude > 0.01f) {
            // For ball control we use velocity directly because it's easier and produce
            // better results
            if (player_on_the_ball.controls->sprint) {
                // Run control
                player_on_the_ball.player->sprint_locked = true;
                v2 control_direction = player_on_the_ball.controls->steering;
                // Limit angle between requested run direction and current velocity
                if (Vector2Length(player_on_the_ball.velocity) >
                    // Only limit if velocity is at least 70% of max
                    0.7f * game_state->constants.player_run_max_velocity) {
                    f32 angle = Vector2Angle(player_on_the_ball.velocity,
                                             player_on_the_ball.controls->steering);
                    f32 max_angle = game_state->constants.player_run_control_max_angle * DEG2RAD;
                    if (angle > max_angle) {
                        control_direction = Vector2Rotate(player_on_the_ball.velocity, max_angle);
                    } else if (angle < -max_angle) {
                        control_direction = Vector2Rotate(player_on_the_ball.velocity, -max_angle);
                    }
                }
                entities_ball_control_running(game_state->vars.ball_entity_id, control_direction,
                                              player_on_the_ball.velocity, 0.0f, game_state);
            } else {
                // Precision control
                v2 position_to_seek = Vector2Add(
                    player_on_the_ball.position,
                    Vector2Scale(normalized_velocity,
                                 player_on_the_ball.collider->props.circle.radius * 0.9f));
                entities_ball_control_precision(game_state->vars.ball_entity_id,
                                                player_on_the_ball.velocity, position_to_seek, 1.0f,
                                                game_state);
            }
        }

        // We can reset charge now: we already kicked the ball if needed
        if (kicked_ball && player_on_the_ball.id == game_state->vars.p1.player_entity_id &&
            game_state->vars.p1.controls.charge.ready) {
            game_state->vars.p1.controls.charge.ready =
                game_state->vars.p1.controls.charge.charging = false;
        }
    }
}
