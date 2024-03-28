#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include <raylib.h>
#include <raymath.h>

void systems_ball(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    EntityId ball_id = game_state->vars.ball_entity_id;
    if (entity_repository->entities[ball_id].alive) {
        ComponentTransform *transform =
            &entities_component_get(game_state->entity_repository, CT_TRANSFORM, ball_id)
                 ->transform;
        ComponentPhysics3d *physics_3d =
            &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D, ball_id)
                 ->physics_3d;
        ComponentSprite *sprite =
            &entities_component_get(game_state->entity_repository, CT_SPRITE, ball_id)->sprite;
        ComponentBall *ball =
            &entities_component_get(game_state->entity_repository, CT_BALL, ball_id)->ball;
        Component *component =
            entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, ball_id);
        StateMachineBall *state_machine = component->state_machine.ptr;

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
                // Drag on the pitch
                physics_3d->velocity.x += physics_3d->velocity.x *
                                          -game_state->constants.ball_drag_on_pitch *
                                          game_state->dt;
                physics_3d->velocity.z += physics_3d->velocity.z *
                                          -game_state->constants.ball_drag_on_pitch *
                                          game_state->dt;

                // Determine which player is controlling the ball
                EntityForEach(game_state->entity_repository, player_entity, idx) {
                    if (player_entity->alive &&
                        EntityTypeCheckMask(player_entity->type, EntityTypeToBitMask(ET_PLAYER))) {
                    }
                }

                // b32 is_colliding_with_ball = CheckCollisionCircles(transform->position,
                // collider->props.circle.radius, ball_transform.position,
                // ball_collider.props.circle.radius) && ball_transform.virtual_height < 40.0f;
                //
                // if (is_colliding_with_ball) {
                //         if (input->buttons[IBT_BUTTON_DOWN].released) {
                //             // Shot
                //             ball_physics_3d.velocity = Vector3Add(
                //                 ball_physics_3d.velocity,
                //                 Vector3Scale(Vector3Add(Vector3Normalize(player_on_the_ball_force),
                //                                         (Vector3){.x = 0, .y = ball_shot_height,
                //                                         .z = 0}),
                //                              ball_shot_force));
                //             ball_shot = true;
                //             ball_shot_time = GetTime();
                //         } else {
                //             if (IsKeyDown(KEY_X)) {
                //                 // Precision control
                //                 ball_vel = Vector3Add(
                //                     player_on_the_ball_force,
                //                     Vector3Scale(
                //                         Vector3Normalize(Vector3Subtract(
                //                             (Vector3){.x = player_pos.x, .z = player_pos.y},
                //                             ball_3d_pos)),
                //                         ball_control_force));
                //             } else {
                //                 // Run control
                //                 ball_vel =
                //                     Vector3Add(player_on_the_ball_force,
                //                                Vector3Scale(Vector3Normalize(player_on_the_ball_force),
                //                                             ball_run_force));
                //             }
                //         }
                // }
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
