#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "input.h"
#include "layers/layers.h"
#include "memory.h"
#include "steering.h"
#include "types.h"
#include "utils.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

typedef struct {
    v2 steering;
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

EntityId find_nearest_teammate(GameState *game_state, PlayerOnTheBall *player_on_the_ball,
                               v2 position) {
    EntityRepository *entity_repository = game_state->entity_repository;
    EntityId selected_entity_id = EID_NOT_VALID;
    f32 selected_distance_sqr = UINT32_MAX;
    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(entity_repository, entity_id, 2,
                                                        CT_TRANSFORM, CT_PLAYER)) {

            ComponentTransform *transform = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_TRANSFORM, entity_id);
            Component *goalie =
                entities_component_get(game_state->entity_repository, CT_GOALIE, entity_id);

            if (player_on_the_ball->id == entity_id || goalie->header.enabled) {
                // We don't want to pass to ourselves or to our goalie, right?
                continue;
            }

            f32 distance_sqr = Vector2DistanceSqr(position, transform->position);
            if (distance_sqr < selected_distance_sqr) {
                selected_entity_id = entity_id;
            }
        }
    }
    return selected_entity_id;
}

EntityId find_pass_target(GameState *game_state, PlayerOnTheBall *player_on_the_ball,
                          f32 detection_angle, f32 max_distance) {
    EntityRepository *entity_repository = game_state->entity_repository;
    EntityId selected_entity_id = EID_NOT_VALID;
    f32 selected_distance = UINT32_MAX;
    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(entity_repository, entity_id, 3,
                                                        CT_TRANSFORM, CT_PHYSICS, CT_PLAYER)) {

            ComponentTransform *transform = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_TRANSFORM, entity_id);
            ComponentPhysics *physics =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_PHYSICS, entity_id);
            ComponentPlayer *player =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_PLAYER, entity_id);

            if (player_on_the_ball->id == entity_id) {
                // We don't want to pass to ourselves, right?
                continue;
            }

            v2 selected_direction = player_on_the_ball->controls->steering;
            v2 to_player = Vector2Subtract(transform->position, player_on_the_ball->position);
            f32 distance_from_player = Vector2Length(to_player);
            f32 angle = Vector2Angle(selected_direction, to_player) * RAD2DEG;
            if (distance_from_player < max_distance && distance_from_player < selected_distance &&
                fabs(angle) < detection_angle) {
                selected_entity_id = entity_id;
                printf("pass target is valid: %zu, %.2f, %.2f\n", entity_id, distance_from_player,
                       angle);
            }
        }
    }
    return selected_entity_id;
}

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
    EntityId new_p1_entity_id = EID_NOT_VALID;

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

            collider->colliders[0].props_circle.radius =
                game_state->constants.player_control_radius;

            switch (state_machine->current_state) {
            case PS_NONE:
                break;
            case PS_NORMAL: {
                PlayerControls *player_controls = memory_arena_push(
                    game_state->memory_by_lifetime[MAL_UPDATE], sizeof(PlayerControls));

                if (game_state->vars.p1.player_entity_id == entity_id) {
                    player_controls->steering = game_state->vars.p1.controls.steering;

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
                        if (player->should_receive_ball) {
                            player_controls->steering =
                                steering_seek(transform->position, physics->velocity,
                                              physics->max_velocity, ball_transform->position);
                            layers_debug_queue_draw_text(game_state->layer_debug,
                                                         TextFormat("steering: %.2f, %.2f",
                                                                    player_controls->steering.x,
                                                                    player_controls->steering.y));
                        }
                    }
                }

                v2 effective_steering = player_controls->steering;
                // If charging kick we seek the ball
                if (entity_id == game_state->vars.p1.player_entity_id &&
                    game_state->vars.p1.controls.charge.charging) {
                    effective_steering = Vector2Normalize(
                        Vector2Subtract(ball_transform->position, transform->position));
                }
                v2 force_to_apply = Vector2Zero();
                v2 normalized_velocity = Vector2Normalize(physics->velocity);
                f32 steering_magnitude = Vector2Length(effective_steering);
                f32 current_velocity = Vector2Length(physics->velocity);

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

                // Handle max velocity
                f32 player_run_max_velocity = game_state->constants.player_run_max_velocity;
                f32 initial_max_velocity = player_run_max_velocity * 0.5f;
                if (current_velocity >= player->current_max_velocity &&
                    player->current_max_velocity < player_run_max_velocity) {
                    player->current_max_velocity += 50 * game_state->dt;
                    player->current_max_velocity =
                        fminf(player->current_max_velocity, player_run_max_velocity);
                } else if (current_velocity < initial_max_velocity) {
                    player->current_max_velocity = initial_max_velocity;
                }
                physics->max_velocity = player->current_max_velocity;

                entities_player_apply_force(entity_id, force_to_apply, game_state);
                entities_player_apply_velocity(entity_id, game_state);

                // Velocity has been applied to player after this point

                b32 is_colliding_with_ball =
                    CheckCollisionCircles(transform->position,
                                          collider->colliders[0].props_circle.radius,
                                          ball_transform->position,
                                          ball_collider->colliders[0].props_circle.radius) &&
                    // Player height
                    ball_transform->virtual_height < game_state->constants.player_control_height;

                if (is_colliding_with_ball && (game_state->time - player->last_kick_time) > 0.1f) {
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

                        new_p1_entity_id = entity_id;
                        player->should_receive_ball = false;
                    }
                } else {
                    player->control_ball = true;
                    player->time_spent_in_precision_control = 0;
                }

                // Calculate frame to display
                entities_player_calculate_running_frame(entity_id, physics->velocity, 3,
                                                        game_state);

                break;
            }
            case PS_GOALIE:
                break;
            case PS_COUNT:
                break;
            }
            layers_debug_queue_draw_text(game_state->layer_debug,
                                         TextFormat("Player %02d pos: [%0.2f, %0.2f] vel: %.2f",
                                                    entity_id, transform->position.x,
                                                    transform->position.y,
                                                    Vector2Length(physics->velocity)));
        }
    }

    if (player_on_the_ball.exists) {

        b32 kicked_ball = false;
        f32 velocity_length = Vector2Length(player_on_the_ball.velocity);
        v2 normalized_velocity = Vector2Normalize(player_on_the_ball.velocity);
        f32 steering_magnitude = Vector2Length(player_on_the_ball.controls->steering);
        ball->last_player_on_the_ball_entity_id = player_on_the_ball.id;

        if (player_on_the_ball.controls->shot > 0 && steering_magnitude > 0.1f) {
            // Shot
            player_on_the_ball.player->last_kick_time = game_state->time;
            v2 shot_direction;

            if (!game_state->constants.shoot_aid_enabled) {
                shot_direction = Vector2Normalize(player_on_the_ball.controls->steering);
            } else {
                v2 center_of_goal = (Vector2){game_state->vars.pitch.rectangle.x +
                                                  game_state->vars.pitch.rectangle.width / 2.0f,
                                              game_state->vars.pitch.rectangle.y};
                v2 shot_direction_to_center =
                    Vector2Normalize(Vector2Subtract(center_of_goal, ball_transform->position));
                v2 shot_direction_to_right_post = Vector2Normalize(Vector2Subtract(
                    Vector2Add(center_of_goal, (v2){.x = 50}), ball_transform->position));
                f32 angle_center_to_post =
                    Vector2Angle(shot_direction_to_center, shot_direction_to_right_post);

                shot_direction =
                    Vector2Rotate(shot_direction_to_center,
                                  angle_center_to_post * player_on_the_ball.controls->steering.x);
            }

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
            entities_ball_shoot(game_state->vars.ball_entity_id, player_on_the_ball.id,
                                shot_direction, player_on_the_ball.controls->shot, game_state);
            kicked_ball = true;
        } else if (player_on_the_ball.controls->pass > 0 && steering_magnitude > 0.1f) {
            // Pass
            player_on_the_ball.player->last_kick_time = game_state->time;
            v3 pass_direction;
            EntityId teammate_entity_id =
                find_pass_target(game_state, &player_on_the_ball, 30, 300);
            b32 auto_pass = teammate_entity_id != EID_NOT_VALID;
            if (auto_pass) {
                ComponentTransform *teammate_transform = entities_component_get_typed(
                    game_state->entity_repository, CT_TRANSFORM, teammate_entity_id);
                ComponentPlayer *teammate_player = entities_component_get_typed(
                    game_state->entity_repository, CT_PLAYER, teammate_entity_id);
                pass_direction = Vector3Normalize(V2_TO_V3(
                    Vector2Subtract(teammate_transform->position, player_on_the_ball.position), 0));
                teammate_player->should_receive_ball = true;
            } else {
                pass_direction =
                    Vector3Normalize(V2_TO_V3(player_on_the_ball.controls->steering, 0));
                EntityId nearest_teammate =
                    find_nearest_teammate(game_state, &player_on_the_ball,
                                          Vector2Add(player_on_the_ball.position,
                                                     Vector2Scale(V3_TO_V2(pass_direction), 300)));
                new_p1_entity_id = nearest_teammate;
            }
            entities_ball_short_pass(game_state->vars.ball_entity_id, player_on_the_ball.id,
                                     pass_direction, player_on_the_ball.controls->pass, auto_pass,
                                     game_state);
            kicked_ball = true;
        } else if (player_on_the_ball.controls->long_pass && steering_magnitude > 0.1f) {
            // Long pass
            player_on_the_ball.player->last_kick_time = game_state->time;
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
            entities_ball_long_pass(game_state->vars.ball_entity_id, player_on_the_ball.id,
                                    long_pass_direction, player_on_the_ball.controls->long_pass,
                                    game_state);
            kicked_ball = true;
        } else if (!kicked_ball && steering_magnitude > 0.01f) {
            // For ball control we use velocity directly because it's easier and produce
            // better results
            if (velocity_length < game_state->constants.player_run_max_velocity * 0.7f) {
                player_on_the_ball.player->control_ball = true;
                player_on_the_ball.player->time_spent_in_run_control = 0;
            }
            if (player_on_the_ball.player->control_ball) {
                // Precision control
                player_on_the_ball.player->time_spent_in_precision_control += game_state->dt;
                player_on_the_ball.player->time_spent_in_run_control = 0;
                if (player_on_the_ball.player->time_spent_in_precision_control > 0.25f) {
                    player_on_the_ball.player->control_ball = false;
                }
                layers_debug_queue_draw_text(
                    game_state->layer_debug,
                    TextFormat("prec_ctrl: %.2f",
                               player_on_the_ball.player->time_spent_in_precision_control));
                v2 position_to_seek = Vector2Add(
                    player_on_the_ball.position,
                    Vector2Scale(normalized_velocity,
                                 player_on_the_ball.collider->colliders[0].props_circle.radius *
                                     1));
                // TODO: this should depend on player technical abilities
                if (player_on_the_ball.player->time_spent_in_precision_control < 1) {
                    entities_ball_control_precision(
                        game_state->vars.ball_entity_id, player_on_the_ball.id,
                        player_on_the_ball.velocity, position_to_seek, 1, game_state);
                }
            } else {
                // Run control
                player_on_the_ball.player->time_spent_in_run_control += game_state->dt;
                player_on_the_ball.player->time_spent_in_precision_control = 0;
                if (player_on_the_ball.player->time_spent_in_run_control > 0.25f) {
                    player_on_the_ball.player->control_ball = true;
                }
                v2 control_direction = player_on_the_ball.controls->steering;

                // TODO: this should depend on player technical abilities
                f32 control_ratio = 1 - powf(velocity_length, 4) /
                                            powf(game_state->constants.player_run_max_velocity, 4);
                entities_ball_control_running(
                    game_state->vars.ball_entity_id, player_on_the_ball.id, control_direction,
                    player_on_the_ball.velocity, control_ratio, game_state);
            }
        }

        // Let's change the controller player if needed
        if (new_p1_entity_id != EID_NOT_VALID) {
            game_state->vars.p1.player_entity_id = new_p1_entity_id;
        }

        // We can reset charge now: we already kicked the ball if needed
        if (kicked_ball && player_on_the_ball.id == game_state->vars.p1.player_entity_id &&
            game_state->vars.p1.controls.charge.ready) {
            game_state->vars.p1.controls.charge.ready =
                game_state->vars.p1.controls.charge.charging = false;
        }
    }
}
