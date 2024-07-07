#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../sim/simulation.h"
#include "../utils/utils.h"
#include "football/football.h"
#include "input.h"
#include "layers/layers.h"
#include "sprites.h"
#include <raylib.h>
#include <raymath.h>

void systems_ball(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    for (usize ball_id = 0; ball_id < entity_repository->max_slots; ++ball_id) {
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
            ComponentCollider *collider =
                entities_component_get_typed(game_state->entity_repository, CT_COLLIDER, ball_id);
            StateMachineBall *state_machine = ball->state_machine;

            if (input->buttons[IBT_BUTTON_START].released) {
                ComponentTransform *p1_transform =
                    &entities_component_get(game_state->entity_repository, CT_TRANSFORM,
                                            game_state->vars.p1.player_entity_id)
                         ->transform;
                entities_ball_set_position_and_velocity(
                    ball_id, V2_TO_V3(Vector2Add(p1_transform->position, (v2){.y = -20}), 50),
                    Vector3Zero(), game_state);
                football_match_change_state(game_state, MS_PLAY);
                ball->subject_to_after_touch = false;
                ball->last_player_on_the_ball_entity_id = -1;
                return;
            }

            // Test cross-bar
            if (input->buttons[IBT_BUTTON_SELECT].released) {
                entities_ball_set_position_and_velocity(
                    ball_id, (v3){.x = game_state->vars.pitch.rectangle.x + 340, .z = 300},
                    Vector3Scale((v3){.x = 0, .y = 1, .z = -1}, 413.5f), game_state);
                football_match_change_state(game_state, MS_PLAY);
                ball->subject_to_after_touch = false;
                ball->last_player_on_the_ball_entity_id = -1;
                return;
            }

            // Test cross
            if (input->buttons[IBT_BUTTON_UP].released) {
                transform->position = (v2){.x = game_state->vars.pitch.rectangle.x + 600, .y = 100};
                transform->virtual_height = 0;
                physics_3d->accel = Vector3Zero();
                physics_3d->velocity = Vector3Scale((v3){.x = -1, .y = 1.0f, .z = 0.2}, 280.5f);
                football_match_change_state(game_state, MS_PLAY);
                ball->subject_to_after_touch = false;
                ball->last_player_on_the_ball_entity_id = -1;
                return;
            }

            // Sprite draw ordering when ball is in upper goal
            sprite->layer = transform->position.y < game_state->vars.pitch.rectangle.y
                                ? SL_NORTH_GOAL
                                : SL_PLAYERS_AND_BALL;

            layers_debug_queue_draw_text(
                game_state->layer_debug,
                TextFormat("Ball pos: %.2f, %.2f", transform->position.x, transform->position.y));

            layers_debug_queue_draw_text(
                game_state->layer_debug,
                TextFormat("Ball height: %.2f", transform->virtual_height));

            layers_debug_queue_draw_text(game_state->layer_debug,
                                         TextFormat("Ball vel: %.2f, %.2f, %.2f",
                                                    physics_3d->velocity.x, physics_3d->velocity.y,
                                                    physics_3d->velocity.z));

            layers_debug_queue_draw_text(
                game_state->layer_debug,
                TextFormat("Ball speed: %.2f", Vector3Length(physics_3d->velocity)));

            switch (state_machine->current_state) {
            case BS_NONE:
                break;
            case BS_IN_PLAY: {
                Vector3 position = V2_TO_V3(transform->position, transform->virtual_height);
                Vector3 velocity = physics_3d->velocity;
                Vector3 accel = physics_3d->accel;

                // After touch
                f32 after_touch_time = (game_state->time - ball->last_kick_time);
                if (ball->last_kicker_entity_id == game_state->vars.p1.player_entity_id &&
                    ball->subject_to_after_touch &&
                    after_touch_time > game_state->constants.ball_after_touch_delay_time) {
                    v2 input = game_state->vars.p1.controls.steering;
                    f32 force =
                        1 - Clamp(powf((after_touch_time -
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
                f32 ball_radius = collider->colliders[0].props_circle.radius;
                sim_ball_physics(&position, &velocity, accel, physics_3d->mass, ball_radius,
                                 &on_the_ground, &bounced, game_state);
                ball->is_on_the_ground = on_the_ground && !bounced;
                if (bounced) {
                    ball->last_bounce_time = game_state->time;
                }

                // Calculate next 10 look ahead positions, 10 dts apart from each other
                b32 lookahead_on_the_ground;
                b32 lookahead_bounced;
                v3 lookahead_position = position;
                v3 lookahead_velocity = velocity;
                u16 lookaheads = game_state->constants.ball_lookaheads;
                u32 lookahead_dts = (game_state->vars.slow_motion ? 10 : 1) *
                                    game_state->constants.ball_lookahead_dts;
                for (u16 i = 0; i < lookaheads; ++i) {
                    // 10 dt steps (100 in slow motion)
                    for (u16 j = 0; j < lookahead_dts; ++j) {
                        sim_ball_physics(&lookahead_position, &lookahead_velocity, Vector3Zero(),
                                         physics_3d->mass, ball_radius, &lookahead_on_the_ground,
                                         &lookahead_bounced, game_state);
                    }
                    ball->lookahead_positions[i] = lookahead_position;
                }

                // Calculate bounce position
                lookahead_position = position;
                lookahead_velocity = velocity;
                lookahead_on_the_ground = false;
                lookahead_bounced = false;
                while (!lookahead_bounced && !lookahead_on_the_ground) {
                    sim_ball_physics(&lookahead_position, &lookahead_velocity, Vector3Zero(),
                                     physics_3d->mass, ball_radius, &lookahead_on_the_ground,
                                     &lookahead_bounced, game_state);
                    if (lookahead_bounced) {
                        ball->future_bounce_position = V3_TO_V2(lookahead_position);
                    }
                }

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
                    AssetsAnimation *animation = assets_animation_get_by_id(
                        game_state->animation_cache, ball->roll_animations[pd]);
                    f32 ball_max_velocity = game_state->constants.ball_max_velocity / 10.0f;
                    f32 delay =
                        (animation->delay *
                         (5 - 4 * Clamp(Vector2Length(ball_velocity) / ball_max_velocity, 0, 1)));
                    if ((game_state->time - ball->last_frame_update_time) > delay) {
                        ball->last_frame_update_time = game_state->time;
                        u16 new_frame_id =
                            (ball->last_running_frame_idx + 1) % animation->frames_count;
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
}
