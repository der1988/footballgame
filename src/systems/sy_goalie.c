#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../sim/simulation.h"
#include "assets.h"
#include "input.h"
#include "layers.h"
#include "steering.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

void systems_goalie(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentTransform *ball_transform = entities_component_get_typed(
        game_state->entity_repository, CT_TRANSFORM, game_state->vars.ball_entity_id);
    ComponentPhysics3d *ball_physics_3d = entities_component_get_typed(
        game_state->entity_repository, CT_PHYSICS_3D, game_state->vars.ball_entity_id);
    ComponentCollider *ball_collider = entities_component_get_typed(
        game_state->entity_repository, CT_COLLIDER, game_state->vars.ball_entity_id);
    ComponentBall *ball = entities_component_get_typed(game_state->entity_repository, CT_BALL,
                                                       game_state->vars.ball_entity_id);

    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(entity_repository, entity_id, 5,
                                                        CT_TRANSFORM, CT_PHYSICS, CT_SPRITE,
                                                        CT_GOALIE, CT_COLLIDER)) {

            ComponentTransform *transform = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_TRANSFORM, entity_id);
            ComponentPhysics *physics =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_PHYSICS, entity_id);
            ComponentSprite *sprite =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_SPRITE, entity_id);
            ComponentPlayer *player =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_PLAYER, entity_id);
            ComponentGoalie *goalie =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_GOALIE, entity_id);
            ComponentCollider *collider = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_COLLIDER, entity_id);

            StateMachineGoalie *state_machine = goalie->state_machine;

            v2 goal_center = {.x = game_state->vars.pitch_rectangle.x +
                                   game_state->vars.pitch_rectangle.width * 0.5f,
                              .y = 80};

            f32 distance_from_ball =
                Vector3Distance(V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
                                V2_TO_V3(transform->position, 0));

            f32 distance_from_goal =
                Vector3Distance(V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
                                V2_TO_V3(goal_center, 0));

            collider->colliders[0].props_circle.radius =
                game_state->constants.player_control_radius;
            Collider *save_collider = &collider->colliders[1];

            b32 is_colliding_with_ball =
                CheckCollisionCircles(transform->position, save_collider->props_circle.radius,
                                      ball_transform->position,
                                      ball_collider->colliders[0].props_circle.radius) &&
                // Player height
                ball_transform->virtual_height < 30;

            if (is_colliding_with_ball) {
                // We are the last player on the ball
                ball->last_player_on_the_ball_entity_id = entity_id;
            }

            if (player->state_machine->current_state == PS_GOALIE) {
                switch (state_machine->current_state) {
                case GS_NONE:
                    break;
                case GS_PROTECT_GOAL: {
                    if (is_colliding_with_ball) {
                        // hold ball in hands
                        entities_ball_held_by_goalie(game_state->vars.ball_entity_id, entity_id,
                                                     game_state);
                        entities_goalie_state_machine_change_state_to(goalie->state_machine,
                                                                      GS_HOLD_BALL, game_state);
                        break;
                    }

                    v2 destination;
                    v2 direction;
                    v2 steering;
                    if ((distance_from_goal < 100 || distance_from_ball < 10) &&
                        ball_transform->virtual_height < 20) {
                        // seek ball
                        destination = ball_transform->position;
                        direction =
                            Vector2Normalize(Vector2Subtract(destination, transform->position));
                        steering = steering_seek(transform->position, physics->velocity,
                                                 physics->max_velocity, destination);
                    } else {
                        // protect goal
                        direction = Vector2Normalize(
                            Vector2Subtract(ball_transform->position, goal_center));
                        destination = Vector2Add(goal_center, Vector2Scale(direction, 20));
                        steering = steering_arrive(transform->position, physics->velocity,
                                                   physics->max_velocity, destination, 10);
                    }

                    // Run force
                    v2 force_to_apply = Vector2Scale(steering, 10000);

                    entities_player_apply_force(entity_id, force_to_apply, game_state);
                    entities_player_apply_velocity(entity_id, game_state);

                    entities_player_calculate_running_frame(entity_id, direction, 5, game_state);

                    if (Vector3Length(ball_physics_3d->velocity) > 100) {
                        // Only dive if ball velocity is higher than 10km/h
                        for (u16 i = 0; i < 10; ++i) {
                            v3 pos_to_check = ball->lookahead_positions[i];
                            b32 is_colliding_with_ball =
                                CheckCollisionCircles(
                                    transform->position, save_collider->props_circle.radius,
                                    V3_TO_V2(pos_to_check),
                                    ball_collider->colliders[0].props_circle.radius) &&
                                pos_to_check.y < 30;
                            if (is_colliding_with_ball) {
                                // Ball is coming our direction, let's wait
                                break;
                            }
                            // Check the ball is still savable
                            f32 x_diff = pos_to_check.x - transform->position.x;
                            f32 y_diff = pos_to_check.z - transform->position.y;
                            if (y_diff > 0 && y_diff < 5 && fabsf(x_diff) < 60) {
                                printf("ball will take %d * dt to reach point of no return\n", i);
                                goalie->dive_direction = x_diff > 0 ? SD_RIGHT : SD_LEFT;
                                entities_goalie_state_machine_change_state_to(goalie->state_machine,
                                                                              GS_DIVE, game_state);
                                break;
                            }
                        }
                    }

                    break;
                }
                case GS_HOLD_BALL: {
                    if (HAS_TIME_PASSED(game_state->time,
                                        goalie->state_machine->state_data.hold_ball.hold_start_time,
                                        1)) {
                        entities_ball_back_in_play(game_state->vars.ball_entity_id, game_state);
                        v2 new_ball_position = Vector2Add(
                            transform->position,
                            Vector2Scale((v2){.y = 1},
                                         save_collider->props_circle.offset.y +
                                             save_collider->props_circle.radius +
                                             ball_collider->colliders[0].props_circle.radius + 7));
                        entities_ball_set_position_and_velocity(game_state->vars.ball_entity_id,
                                                                V2_TO_V3(new_ball_position, 10),
                                                                Vector3Zero(), game_state);
                        entities_ball_long_pass(game_state->vars.ball_entity_id, entity_id,
                                                (v2){.y = 1}, 0.7f, game_state);
                        entities_goalie_state_machine_change_state_to(goalie->state_machine,
                                                                      GS_PROTECT_GOAL, game_state);
                    }
                    break;
                }
                case GS_DIVE: {
                    SaveDirection direction = goalie->dive_direction;

                    StateMachineGoalieDiveData *dive_state_data =
                        &goalie->state_machine->state_data.dive;
                    AssetsAnimation *animation = assets_animation_get_by_id(
                        game_state->animation_cache, goalie->dive_animations[SS_UP][direction]);
                    if (!dive_state_data->animation_completed &&
                        (game_state->time - goalie->last_frame_update_time) >
                            (animation->delay *
                             (2 - Vector2Length(physics->velocity) / physics->max_velocity))) {
                        goalie->last_frame_update_time = game_state->time;
                        u16 new_frame_id = goalie->last_diving_frame_idx + 1;
                        if (new_frame_id == animation->frames_count) {
                            dive_state_data->animation_completed = true;
                            dive_state_data->animation_end_time = game_state->time;
                        } else {
                            sprite->frame_id = animation->frames[new_frame_id];
                            goalie->last_diving_frame_idx = new_frame_id;
                        }
                    }
                    v2 force_to_apply = Vector2Zero();
                    if (dive_state_data->animation_completed) {
                        // Apply friction
                        force_to_apply =
                            Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)), 50000);
                    } else {
                        v2 steering = direction == SD_LEFT ? Vector2Left() : Vector2Right();
                        force_to_apply = Vector2Scale(steering, 60000);
                    }
                    entities_player_apply_force(entity_id, force_to_apply, game_state);
                    entities_player_apply_velocity(entity_id, game_state);

                    static f64 last_save_time = 0;
                    if (HAS_TIME_PASSED(game_state->time, last_save_time, 0.1f) &&
                        is_colliding_with_ball) {
                        // Bounce ball
                        v3 new_ball_velocity =
                            Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1});
                        // new_ball_velocity = Vector3RotateByAxisAngle(
                        //     new_ball_velocity, (v3){.y = 1},
                        //     30 - 60 * GetRandomValue(0, 100) / 100.0f * DEG2RAD);
                        entities_ball_set_position_and_velocity(
                            game_state->vars.ball_entity_id,
                            V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
                            Vector3Scale(new_ball_velocity, 0.3f), game_state);
                        last_save_time = game_state->time;
                    } else if (dive_state_data->animation_completed &&
                               HAS_TIME_PASSED(game_state->time,
                                               dive_state_data->animation_end_time, 0.75f)) {
                        entities_goalie_state_machine_change_state_to(goalie->state_machine,
                                                                      GS_PROTECT_GOAL, game_state);
                    }

                    break;
                }
                case GS_COUNT:
                    break;
                }
            }
        }
    }
}
