#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../utils/utils.h"
#include <raylib.h>
#include <raymath.h>

void systems_ball(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    EntityId ball_id = game_state->vars.ball_entity_id;
    if (entity_repository->entities[ball_id].alive) {
        ComponentTransform *transform =
            &entities_component_get(game_state->entity_repository, CT_TRANSFORM, ball_id)
                 ->transform;
        ComponentPhysics3d *physics_3d =
            &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D, ball_id)
                 ->physics_3d;
        ComponentCollider *collider =
            &entities_component_get(game_state->entity_repository, CT_COLLIDER, ball_id)->collider;
        ComponentSprite *sprite =
            &entities_component_get(game_state->entity_repository, CT_SPRITE, ball_id)->sprite;
        ComponentBall *ball =
            &entities_component_get(game_state->entity_repository, CT_BALL, ball_id)->ball;
        Component *component =
            entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, ball_id);
        StateMachineBall *state_machine = component->state_machine.ptr;

        if (IsKeyDown(KEY_Q)) {
            ComponentTransform *p1_transform =
                &entities_component_get(game_state->entity_repository, CT_TRANSFORM,
                                        game_state->vars.p1_entity_id)
                     ->transform;
            transform->position = p1_transform->position;
            return;
        }

        switch (state_machine->current_state) {
        case BS_NONE:
            break;
        case BS_IN_PLAY: {

            Vector3 ball_position_3d = (Vector3){.x = transform->position.x,
                                                 .y = transform->virtual_height,
                                                 .z = transform->position.y};
            if (ball_position_3d.y > 0) {
                // Gravity
                physics_3d->velocity = Vector3Add(
                    physics_3d->velocity, Vector3Scale(game_state->constants.gravity_force, 1));
                // Drag in the air
                physics_3d->velocity.x += physics_3d->velocity.x *
                                          -game_state->constants.ball_drag_in_air * game_state->dt;
                physics_3d->velocity.z += physics_3d->velocity.z *
                                          -game_state->constants.ball_drag_in_air * game_state->dt;
            } else {
                ball->subject_to_after_touch = false;

                // Drag on the pitch
                physics_3d->velocity.x += physics_3d->velocity.x *
                                          -game_state->constants.ball_drag_on_pitch *
                                          game_state->dt;
                physics_3d->velocity.z += physics_3d->velocity.z *
                                          -game_state->constants.ball_drag_on_pitch *
                                          game_state->dt;

                EntityForEach(game_state->entity_repository, player_entity, idx) {
                    if (player_entity->alive &&
                        EntityTypeCheckMask(player_entity->type, EntityTypeToBitMask(ET_PLAYER))) {
                        ComponentTransform *player_transform =
                            &entities_component_get(game_state->entity_repository, CT_TRANSFORM,
                                                    idx)
                                 ->transform;
                        ComponentPhysics *player_physics =
                            &entities_component_get(game_state->entity_repository, CT_PHYSICS, idx)
                                 ->physics;
                        ComponentHuman *player_human =
                            &entities_component_get(game_state->entity_repository, CT_HUMAN, idx)
                                 ->human;
                        ComponentCollider *player_collider =
                            &entities_component_get(game_state->entity_repository, CT_COLLIDER, idx)
                                 ->collider;

                        b32 is_colliding_with_ball =
                            CheckCollisionCircles(
                                player_transform->position, player_collider->props.circle.radius,
                                transform->position, collider->props.circle.radius) &&
                            transform->virtual_height < 40.0f;

                        if (is_colliding_with_ball) {
                            Vector3 player_on_the_ball_force = (Vector3){
                                .x = player_physics->velocity.x, .z = player_physics->velocity.y};
                            if (input->buttons[IBT_BUTTON_DOWN].released) {
                                // Shot
                                physics_3d->velocity = Vector3Add(
                                    physics_3d->velocity,
                                    Vector3Scale(
                                        Vector3Add(
                                            Vector3Normalize(player_on_the_ball_force),
                                            (Vector3){.x = 0,
                                                      .y = game_state->constants.ball_shot_height,
                                                      .z = 0}),
                                        game_state->constants.ball_shot_force));
                                ball->subject_to_after_touch = true;
                                ball->shot_time = GetTime();
                            } else {
                                if (input->buttons[IBT_BUTTON_LEFT].down) {
                                    // Precision control
                                    physics_3d->velocity = Vector3Add(
                                        player_on_the_ball_force,
                                        Vector3Scale(
                                            Vector3Normalize(Vector3Subtract(
                                                (Vector3){.x = player_transform->position.x,
                                                          .z = player_transform->position.y},
                                                ball_position_3d)),
                                            game_state->constants.ball_control_force));
                                } else {
                                    // Run control
                                    physics_3d->velocity = Vector3Add(
                                        player_on_the_ball_force,
                                        Vector3Scale(Vector3Normalize(player_on_the_ball_force),
                                                     game_state->constants.ball_run_force));
                                }
                            }
                        }
                    }
                }
            }

            // After touch
            if (ball->subject_to_after_touch &&
                (game_state->time - ball->shot_time) >
                    game_state->constants.ball_after_touch_delay_time &&
                (game_state->time - ball->shot_time) <
                    game_state->constants.ball_after_touch_max_time) {
                v2 after_touch_input = Vector2Normalize(input->left_stick);
                Vector2 ball_2d_vel = {.x = physics_3d->velocity.x, .y = physics_3d->velocity.z};
                Vector2 ball_2d_vel_perp = {.x = ball_2d_vel.y, .y = -ball_2d_vel.x};
                f32 after_touch_height =
                    Clamp(Vector2ProjectScalar(ball_2d_vel, after_touch_input), -1.0f, 0.4f);
                v2 after_touch_side = Vector2Project(ball_2d_vel_perp, after_touch_input);
                v3 after_touch_direction = (Vector3){
                    .x = after_touch_side.x, .y = -after_touch_height, .z = after_touch_side.y};
                physics_3d->velocity =
                    Vector3Add(physics_3d->velocity,
                               Vector3Scale(after_touch_direction,
                                            game_state->constants.ball_after_touch_force));
            }

            // Apply velocity to ball
            ball_position_3d =
                Vector3Add(ball_position_3d, Vector3Scale(physics_3d->velocity, game_state->dt));

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
            //     Vector2Add(ball_2d_pos, (Vector2){.x = shadow_x_displacement + ball_height *
            //     0.1f,
            //                                       .y = ball_size + ball_height * 0.1f});
        } break;
        case BS_OUT_OF_PLAY: {
        } break;
        case BS_COUNT:
            break;
        }
    }
}
