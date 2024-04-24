#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../utils/utils.h"
#include "input.h"
#include "layers/layers.h"
#include "sprites.h"
#include <raylib.h>
#include <raymath.h>

void systems_ball(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    EntityId ball_id = game_state->vars.ball_entity_id;
    if (entity_repository->entities[ball_id].alive) {
        ComponentTransform *transform =
            entities_component_get_internal(game_state->entity_repository, CT_TRANSFORM, ball_id);
        ComponentPhysics3d *physics_3d =
            entities_component_get_internal(game_state->entity_repository, CT_PHYSICS_3D, ball_id);
        ComponentCollider *collider =
            entities_component_get_internal(game_state->entity_repository, CT_COLLIDER, ball_id);
        ComponentSprite *sprite =
            entities_component_get_internal(game_state->entity_repository, CT_SPRITE, ball_id);
        ComponentBall *ball =
            entities_component_get_internal(game_state->entity_repository, CT_BALL, ball_id);
        ComponentStateMachine *component_state_machine = entities_component_get_internal(
            game_state->entity_repository, CT_STATE_MACHINE, ball_id);
        StateMachineBall *state_machine = component_state_machine->ptr;

        if (input->buttons[IBT_BUTTON_START].pressed) {
            ComponentTransform *p1_transform =
                &entities_component_get(game_state->entity_repository, CT_TRANSFORM,
                                        game_state->vars.p1_entity_id)
                     ->transform;
            transform->position = p1_transform->position;
            transform->virtual_height = 0;
            physics_3d->accel = Vector3Zero();
            physics_3d->velocity = Vector3Zero();
            game_state->vars.ball_is_playable = true;
            game_state->vars.someone_scored = false;
            return;
        }

        // Sprite draw ordering when ball is in upper goal
        sprite->layer = transform->position.y < game_state->vars.pitch_rectangle.y
                            ? SL_UPPER_GOAL
                            : SL_PLAYERS_AND_BALL;

        layers_debug_queue_draw_text(game_state->layer_debug,
                                     TextFormat("Ball height: %.2f", transform->virtual_height));

        layers_debug_queue_draw_text(
            game_state->layer_debug,
            TextFormat("Ball vel: %.2f", Vector3Length(physics_3d->velocity)));

        switch (state_machine->current_state) {
        case BS_NONE:
            break;
        case BS_IN_PLAY: {

            Vector3 ball_position_3d = V2_TO_V3(transform->position, transform->virtual_height);

            /*
             * Drag in the air calculation
             *
             * Formula for drag is FD=1/2CρAv^2
             * A (soccer ball) = 0.038 m^2
             * C (sphere) = 0.45
             * p (air) = 1.21 kg/m3
             *
             * Formula needs meters so we need to convert from pixels to meters and back
             */
            f32 drag_force =
                0.5f * game_state->constants.ball_drag_coefficient *
                game_state->constants.air_density *
                game_state->constants.ball_cross_sectional_area *
                powf(Vector3Length(physics_3d->velocity) / game_state->constants.pixels_per_meter,
                     2) *
                game_state->constants.pixels_per_meter;
            physics_3d->accel =
                Vector3Add(physics_3d->accel,
                           Vector3Scale(Vector3Negate(Vector3Normalize(physics_3d->velocity)),
                                        drag_force / physics_3d->mass));

            // If ball is in the air apply gravity
            b32 on_the_ground = true;
            if (ball_position_3d.y > 0) {
                on_the_ground = false;
                physics_3d->accel =
                    Vector3Add(physics_3d->accel, game_state->constants.gravity_force);
            } else {
                // If it's on the pitch we apply a hacky drag
                physics_3d->velocity.x += physics_3d->velocity.x *
                                          -game_state->constants.ball_friction_on_pitch *
                                          game_state->dt;
                physics_3d->velocity.z += physics_3d->velocity.z *
                                          -game_state->constants.ball_friction_on_pitch *
                                          game_state->dt;
            }

            // Apply acceleration...
            physics_3d->velocity =
                Vector3Add(physics_3d->velocity, Vector3Scale(physics_3d->accel, game_state->dt));
            // ...and velocity
            ball_position_3d =
                Vector3Add(ball_position_3d, Vector3Scale(physics_3d->velocity, game_state->dt));

            // Make the ball bounce
            if (ball_position_3d.y < 0) {
                ball_position_3d.y = 0;
                physics_3d->accel.y *= game_state->constants.ball_cr;
                physics_3d->velocity.y *= -game_state->constants.ball_cr;
                if (fabsf(physics_3d->velocity.y) < 10.0f) {
                    physics_3d->velocity.y = 0;
                }
            }

            // Project position to 2D
            transform->position = V3_TO_V2(ball_position_3d);
            transform->virtual_height = ball_position_3d.y;

            // This has to be done after all forces have been applied
            if (transform->virtual_height == 0) {
                ball->subject_to_after_touch = false;
            }

            // Acceleration does not accumulate
            physics_3d->accel = Vector3Zero();

            // Calculate frame to display
            Vector2 ball_velocity = V3_TO_V2(physics_3d->velocity);
            if (Vector2Length(ball_velocity) > 3) {
                Direction pd = entities_common_calculate_direction(ball_velocity);
                AssetsAnimation *animation = assets_animation_get_by_id(game_state->animation_cache,
                                                                        ball->roll_animations[pd]);
                f32 ball_max_velocity = game_state->constants.ball_max_velocity / 10.0f;
                f32 delay =
                    (animation->delay *
                     (5 - 4 * Clamp(Vector2Length(ball_velocity) / ball_max_velocity, 0, 1)));
                if ((game_state->time - ball->last_frame_update_time) > delay) {
                    ball->last_frame_update_time = game_state->time;
                    u16 new_frame_id = (ball->last_running_frame_idx + 1) % animation->frames_count;
                    sprite->frame_id = animation->frames[new_frame_id];
                    ball->last_running_frame_idx = new_frame_id;
                }
                ball->last_direction = pd;
            }
        } break;
        case BS_OUT_OF_PLAY: {
        } break;
        case BS_COUNT:
            break;
        }
    }
}
