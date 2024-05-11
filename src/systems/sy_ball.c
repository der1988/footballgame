#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../sim/simulation.h"
#include "../utils/utils.h"
#include "input.h"
#include "layers/layers.h"
#include "sprites.h"
#include <raylib.h>
#include <raymath.h>

void systems_ball(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    EntityId ball_id = game_state->vars.ball_entity_id;
    if (entities_entity_is_alive_and_has_components(game_state->entity_repository, ball_id, 4,
                                                    CT_TRANSFORM, CT_PHYSICS_3D, CT_SPRITE,
                                                    CT_BALL)) {
        ComponentTransform *transform =
            entities_component_get_typed(game_state->entity_repository, CT_TRANSFORM, ball_id);
        ComponentPhysics3d *physics_3d =
            entities_component_get_typed(game_state->entity_repository, CT_PHYSICS_3D, ball_id);
        ComponentSprite *sprite =
            entities_component_get_typed(game_state->entity_repository, CT_SPRITE, ball_id);
        ComponentBall *ball =
            entities_component_get_typed(game_state->entity_repository, CT_BALL, ball_id);
        StateMachineBall *state_machine = ball->state_machine;

        if (input->buttons[IBT_BUTTON_START].released) {
            ComponentTransform *p1_transform =
                &entities_component_get(game_state->entity_repository, CT_TRANSFORM,
                                        game_state->vars.p1.player_entity_id)
                     ->transform;
            transform->position = p1_transform->position;
            transform->virtual_height = 50;
            physics_3d->accel = Vector3Zero();
            physics_3d->velocity = Vector3Zero();
            game_state->vars.ball_is_playable = true;
            game_state->vars.someone_scored = false;
            ball->subject_to_after_touch = false;
            ball->last_player_on_the_ball_entity_id = -1;
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
            Vector3 position = V2_TO_V3(transform->position, transform->virtual_height);
            Vector3 velocity = physics_3d->velocity;
            Vector3 accel = physics_3d->accel;

            // After touch
            f32 after_touch_time = (game_state->time - ball->kick_time);
            if (ball->last_kicker_entity_id == game_state->vars.p1.player_entity_id &&
                ball->subject_to_after_touch &&
                after_touch_time > game_state->constants.ball_after_touch_delay_time) {
                v2 input = game_state->vars.p1.controls.steering;
                f32 force = 1 - Clamp(powf((after_touch_time -
                                            game_state->constants.ball_after_touch_delay_time),
                                           2) /
                                          powf((game_state->constants.ball_after_touch_max_time -
                                                game_state->constants.ball_after_touch_delay_time),
                                               2),
                                      0, 1);
                v2 ball_2d_vel = V3_TO_V2(velocity);
                v2 ball_2d_vel_perp = {.x = ball_2d_vel.y, .y = -ball_2d_vel.x};
                v2 after_touch_side = Vector2Project(ball_2d_vel_perp, input);
                v3 after_touch_direction = V2_TO_V3(after_touch_side, 0);
                accel = Vector3Add(
                    accel, Vector3Scale(after_touch_direction,
                                        game_state->constants.ball_after_touch_force * force));
            }

            b32 on_the_ground;
            b32 bounced;
            sim_ball_physics(&position, &velocity, accel, physics_3d->mass, &on_the_ground,
                             &bounced, game_state);

            // Project position to 2D
            transform->position = V3_TO_V2(position);
            transform->virtual_height = position.y;

            physics_3d->velocity = velocity;
            // Acceleration does not accumulate
            physics_3d->accel = Vector3Zero();

            if (bounced) {
                ball->subject_to_after_touch = false;
            }

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
            break;
        }
        case BS_GK_HANDS: {
            break;
        }
        case BS_COUNT:
            break;
        }
    }
}
