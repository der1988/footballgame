#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "assets.h"
#include "input.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>

void systems_player(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentForEach(entity_repository, base_component, idx) {
        Component *base_transform =
            entities_component_get(game_state->entity_repository, CT_TRANSFORM, idx);
        Component *base_physics =
            entities_component_get(game_state->entity_repository, CT_PHYSICS, idx);
        Component *base_sprite =
            entities_component_get(game_state->entity_repository, CT_SPRITE, idx);
        Component *base_human =
            entities_component_get(game_state->entity_repository, CT_HUMAN, idx);
        Component *base_collider =
            entities_component_get(game_state->entity_repository, CT_COLLIDER, idx);
        Component *base_state_machine =
            entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, idx);
        if (entity_repository->entities[idx].alive && base_transform->header.enabled &&
            base_physics->header.enabled && base_human->header.enabled &&
            base_collider->header.enabled && base_state_machine->header.enabled &&
            base_sprite->header.enabled) {

            ComponentTransform *transform = &base_transform->transform;
            ComponentPhysics *physics = &base_physics->physics;
            ComponentCollider *collider = &base_collider->collider;
            ComponentSprite *sprite = &base_sprite->sprite;
            ComponentHuman *human = &base_human->human;

            StateMachinePlayer *state_machine = base_state_machine->state_machine.ptr;

            Vector2 steering = Vector2ClampValue(input->left_stick, -1, 1);
            Vector2 force_to_apply = Vector2Zero();

            switch (state_machine->current_state) {
            case PS_NONE:
                break;
            case PS_RUN: {
                f32 sprint = (input->right_trigger + 1) / 2.0f;
                physics->max_velocity = game_state->constants.player_control_max_velocity +
                                        (game_state->constants.player_run_max_velocity -
                                         game_state->constants.player_control_max_velocity) *
                                            sprint;

                // Run force
                force_to_apply = Vector2Scale(Vector2Normalize(steering),
                                              game_state->constants.player_movement_force);

                if (Vector2Length(physics->velocity) > 5) {
                    // Drag force
                    Vector2 drag = Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
                                                game_state->constants.player_movement_drag);
                    force_to_apply = Vector2Add(force_to_apply, drag);
                } else {
                    physics->velocity = Vector2Zero();
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

                if (is_colliding_with_ball && (game_state->time - human->last_shot) > 0.1f) {
                    Vector3 ball_position_3d =
                        V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
                    Vector3 player_on_the_ball_force = V2_TO_V3(physics->velocity, 0);
                    if (input->buttons[IBT_BUTTON_LEFT].down && Vector2Length(steering) > 0.1f) {
                        // Shot
                        human->last_shot = game_state->time;
                        ball_physics_3d->velocity = Vector3Add(
                            ball_physics_3d->velocity,
                            Vector3Scale(
                                Vector3Add(Vector3Normalize(V2_TO_V3(steering, 0)),
                                           (Vector3){.x = 0,
                                                     .y = game_state->constants.ball_shot_height,
                                                     .z = 0}),
                                game_state->constants.ball_shot_force));
                        ball->subject_to_after_touch = true;
                        ball->shot_time = game_state->time;
                    } else if (Vector2Length(physics->velocity) > 0.05f) {
                        f32 sprint = (input->right_trigger + 1) / 2.0f;
                        if (sprint > 0.1f) {
                            // Run control
                            ball_physics_3d->velocity = Vector3Add(
                                player_on_the_ball_force,
                                Vector3Scale(Vector3Add(Vector3Normalize(V2_TO_V3(steering, 0)),
                                                        (v3){.y = 0.5f}),
                                             game_state->constants.ball_run_force *
                                                 (0.4f + 0.6f * sprint)));
                        } else {
                            // Precision control
                            ball_physics_3d->velocity =
                                Vector3Add(player_on_the_ball_force,
                                           Vector3Scale(Vector3Normalize(Vector3Subtract(
                                                            V2_TO_V3(transform->position, 0),
                                                            ball_position_3d)),
                                                        game_state->constants.ball_control_force));
                        }
                    }
                }

                // After touch
                if (ball->subject_to_after_touch &&
                    (game_state->time - ball->shot_time) >
                        game_state->constants.ball_after_touch_delay_time &&
                    (game_state->time - ball->shot_time) <
                        game_state->constants.ball_after_touch_max_time) {
                    v2 after_touch_input = Vector2Normalize(steering);
                    Vector2 ball_2d_vel = V3_TO_V2(ball_physics_3d->velocity);
                    Vector2 ball_2d_vel_perp = {.x = ball_2d_vel.y, .y = -ball_2d_vel.x};
                    f32 after_touch_height =
                        Clamp(Vector2ProjectScalar(ball_2d_vel, after_touch_input), -1.0f, 0.4f);
                    v2 after_touch_side = Vector2Project(ball_2d_vel_perp, after_touch_input);
                    v3 after_touch_direction = (Vector3){
                        .x = after_touch_side.x, .y = -after_touch_height, .z = after_touch_side.y};
                    ball_physics_3d->velocity =
                        Vector3Add(ball_physics_3d->velocity,
                                   Vector3Scale(after_touch_direction,
                                                game_state->constants.ball_after_touch_force));
                }

                // Calculate frame to display
                if (Vector2Length(physics->velocity) > 0.01f) {
                    Direction pd = entities_player_calculate_direction(physics->velocity);
                    AssetsAnimation *animation = assets_animation_get_by_id(
                        game_state->animation_cache, base_human->human.run_animations[pd]);
                    f32 max_player_velocity = game_state->constants.player_run_max_velocity;
                    if ((game_state->time - human->last_frame_update_time) >
                        (animation->delay *
                         (2 - Vector2Length(physics->velocity) / max_player_velocity))) {
                        human->last_frame_update_time = game_state->time;
                        u16 new_frame_id =
                            (human->last_running_frame_idx + 1) % animation->frames_count;
                        sprite->frame_id = animation->frames[new_frame_id];
                        human->last_running_frame_idx = new_frame_id;
                    }
                    human->last_direction = pd;
                } else {
                    sprite->frame_id = human->stand_frames[human->last_direction];
                }

                break;
            }
            case PS_COUNT:
                break;
            }
        }
    }
}
