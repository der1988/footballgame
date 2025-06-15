#include "components.h"
#include "entities.h"
#include "e_ball.h"
#include "game.h"
#include "input.h"
#include "physics/physics.h"
#include "math_ext.h"
#include "sprite.h"

static b32 is_applied_after_touch_same_side(v2 stored_side, v2 selected_side)
{
	f32 projection = Vector2ProjectScalar(stored_side, selected_side);
	if (projection > 0) {
		return true;
	}

	return false;
}

void systems_ball(struct game_state *game_state, struct input *input)
{
	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_ENTITY_COMPONENT(ball_entity, transform);
	DECLARE_ENTITY_COMPONENT(ball_entity, physics3d);
	DECLARE_ENTITY_COMPONENT(ball_entity, sprite);
	DECLARE_ENTITY_COMPONENT(ball_entity, ball);
	DECLARE_ENTITY_COMPONENT(ball_entity, collider);

	struct state_machine_ball *state_machine = &ball->state_machine;

	if (input->devices[0].buttons[IDBT_BUTTON_START].released) {
		struct component_transform *p1_transform =
			&entity_get(game_state->entity_repository,
						game_state->human_players[0].controlled_player_entity_id)
				 ->transform;
		entity_ball_set_position_and_velocity(
			ball_entity,
			V2_TO_V3(Vector2Add(p1_transform->position, (v2){.y = -20}), 50),
			Vector3Zero());
		football_match_change_state(game_state, MS_PLAY);
		return;
	}

	// Test cross-bar
	if (input->devices[0].buttons[IDBT_BUTTON_DPAD_DOWN].released) {
		entity_ball_set_position_and_velocity(ball_entity,
											  (v3){.x = game_state->vars.pitch.field.x + 340,
												   .z = 300},
											  Vector3Scale((v3){.x = 0, .y = 1, .z = -1}, 290.5f));
		football_match_change_state(game_state, MS_PLAY);
		return;
	}

	// Test cross
	if (input->devices[0].buttons[IDBT_BUTTON_DPAD_UP].released) {
		entity_ball_set_position_and_velocity(
			ball_entity,
			(v3){.x = game_state->vars.pitch.field.x + 600, .z = 100},
			Vector3Scale((v3){.x = -1, .y = 1.0f, .z = 0.2}, 280.5f));
		football_match_change_state(game_state, MS_PLAY);
		return;
	}

	// Test post
	if (input->devices[0].buttons[IDBT_BUTTON_DPAD_RIGHT].released) {
		entity_ball_set_position_and_velocity(ball_entity,
											  (v3){.x = game_state->vars.pitch.field.x + 465.97,
												   .z = 1050},
											  Vector3Scale((v3){.x = -1, .y = 1.67, .z = 1}, 260));
		football_match_change_state(game_state, MS_PLAY);
		return;
	}

	// Test goalie jump
	if (input->devices[0].buttons[IDBT_BUTTON_DPAD_LEFT].released) {
		entity_ball_set_position_and_velocity(
			ball_entity,
			(v3){.x = game_state->vars.pitch.goal[PSI_NORTH].penalty_spot.x,
				 .y = 200,
				 .z = game_state->vars.pitch.goal[PSI_NORTH].penalty_spot.y},
			Vector3Zero());
		football_match_change_state(game_state, MS_PLAY);
		return;
	}

	// Sprite draw ordering when ball is in upper goal
	sprite->layer = transform->position.y < game_state->vars.pitch.field.y ? SL_NORTH_GOAL :
																			 SL_PLAYERS_AND_BALL;

	layer_debug_queue_draw_text(
		game_state->layer_debug,
		TextFormat(
			"%-8s %02d state: %s pos: [%7.2f, %7.2f, %7.2f] vel: [%6.2f, %6.2f, %6.2f] speed: %6.2f",
			"Ball",
			game_state->vars.ball_entity->header.id,
			state_machine_ball_state_string[ball_entity->ball.state_machine.current_state],
			transform->position.x,
			transform->virtual_height,
			transform->position.y,
			physics3d->velocity.x,
			physics3d->velocity.y,
			physics3d->velocity.z,
			VELOCITY_TO_KMH(Vector3Length(physics3d->velocity))));

	switch (state_machine->current_state) {
	case BS_NONE:
		break;
	case BS_IN_PLAY: {
		v3 position = V2_TO_V3(transform->position, transform->virtual_height);
		v3 velocity = physics3d->velocity;
		v3 accel = physics3d->accel;

		// After touch
		f32 after_touch_time = (game_state->time - ball->last_kick_time);
		// TODO: we'd like CPU controlled players to also be able to control after touch in some way.
		if (ball->subject_to_after_touch &&
			after_touch_time > game_state->constants.ball_after_touch_delay_time) {
			struct human_player *human_player =
				game_input_human_player(game_state, ball->last_kicker_entity_id);
			if (human_player) {
				v2 input = human_player->controls.steering;
				v2 ball_2d_vel = V3_TO_V2(velocity);
				v2 ball_2d_vel_perp = Vector2Perp(ball_2d_vel);
				v2 after_touch_side = Vector2Zero();
				f32 after_touch_pressure =
					Vector2ProjectExt(ball_2d_vel_perp, input, &after_touch_side);
				b32 apply_after_touch = true;

				if (ball->selected_after_touch_side.x == 0 &&
					ball->selected_after_touch_side.y == 0) {
					// After touch side not selected yet
					if (fabsf(after_touch_pressure) > 0.25f) {
						// We need some after touch to be actually applied
						ball->selected_after_touch_side = after_touch_side;
					} else {
						apply_after_touch = false;
					}
				} else if (!is_applied_after_touch_same_side(ball->selected_after_touch_side,
															 after_touch_side)) {
					// If player changed side of after touch we keep applying the first one
					// we've seen
					after_touch_side = ball->selected_after_touch_side;
				}

				if (apply_after_touch) {
					// Make it 3D
					v3 after_touch_direction = V2_TO_V3(after_touch_side, 0);

					// After touch force decays exponentially with time.
					// Formula is:
					// 1 - clamp((time_since_ball_kicked - after_touch_delay)^2 /
					// (max_after_touch_time - after_touch_delay)^2, 0, 1)
					f32 force =
						1 - Clamp(powf((after_touch_time -
										game_state->constants.ball_after_touch_delay_time),
									   2) /
									  powf((game_state->constants.ball_after_touch_max_time -
											game_state->constants.ball_after_touch_delay_time),
										   2),
								  0,
								  1);

					// Apply it
					accel = Vector3Add(
						accel,
						Vector3Scale(after_touch_direction,
									 game_state->constants.ball_after_touch_force * force));
				}
			}
		}

		b32 on_the_ground;
		b32 bounced;
		f32 ball_radius = collider->colliders[0].props.circle.radius;
		physics_ball_simulation(&position,
								&velocity,
								accel,
								physics3d->mass,
								ball_radius,
								&on_the_ground,
								&bounced,
								game_state);
		ball->future_bounce.available = false;
		ball->future_reachable.available = false;
		ball->is_on_the_ground = on_the_ground && !bounced;
		if (bounced) {
			ball->last_bounce_time = game_state->time;
		}
		if (ball->is_on_the_ground) {
			ball->future_bounce.available = true;
			ball->future_bounce.position = V3_TO_V2(position);
			ball->future_bounce.dts = 0;
		}
		if (position.y < game_state->constants.crossbar_height) {
			ball->future_reachable.available = true;
			ball->future_reachable.position = position;
			ball->future_reachable.dts = 0;
		}

		// Calculate next 'ball_lookaheads' look ahead positions, ball_lookahead_dts dts apart from each other
		b32 lookahead_on_the_ground;
		b32 lookahead_bounced;
		v3 lookahead_position = position;
		v3 lookahead_velocity = velocity;
		u16 lookaheads = game_state->constants.ball_lookaheads;
		u32 lookahead_dts =
			(game_state->vars.slow_motion ? 10 : 1) * game_state->constants.ball_lookahead_dts;
		for (u16 i = 0; i < lookaheads; ++i) {
			// 10 dt steps (100 in slow motion)
			for (u16 j = 0; j < lookahead_dts; ++j) {
				physics_ball_simulation(&lookahead_position,
										&lookahead_velocity,
										Vector3Zero(),
										physics3d->mass,
										ball_radius,
										&lookahead_on_the_ground,
										&lookahead_bounced,
										game_state);
				if (!ball->future_bounce.available && lookahead_bounced) {
					ball->future_bounce.available = true;
					ball->future_bounce.position = V3_TO_V2(lookahead_position);
					ball->future_bounce.dts = i * game_state->constants.ball_lookaheads + j + 1;
				}
				if (!ball->future_reachable.available &&
					lookahead_position.y < game_state->constants.crossbar_height) {
					ball->future_reachable.available = true;
					ball->future_reachable.position = lookahead_position;
					ball->future_reachable.dts = i * game_state->constants.ball_lookaheads + j + 1;
				}
			}
			ball->lookahead_positions[i] = lookahead_position;
		}

		// Project position to 2D
		transform->position = V3_TO_V2(position);
		transform->virtual_height = position.y;

		physics3d->velocity = velocity;
		// Acceleration does not accumulate
		physics3d->accel = Vector3Zero();

		if (bounced) {
			ball->subject_to_after_touch = false;
		}

		// Calculate frame to display
		v2 ball_velocity = V3_TO_V2(physics3d->velocity);
		if (Vector2Length(ball_velocity) > 3) {
			enum sprite_orientation sprite_orientation =
				sprite_calculate_orientation(ball_velocity);
			struct asset_animation *animation = asset_animation_get_by_id(
				game_state->assets.animation_cache, ball->roll_animations[sprite_orientation]);
			f32 ball_max_velocity = game_state->constants.ball_max_velocity / 10.0f;
			f32 delay = (animation->delay *
						 (5 - 4 * Clamp(Vector2Length(ball_velocity) / ball_max_velocity, 0, 1)));
			if ((game_state->time - ball->last_frame_update_time) > delay) {
				ball->last_frame_update_time = game_state->time;
				u16 new_frame_id = (ball->last_running_frame_idx + 1) % animation->frames_count;
				sprite->frame_id = animation->frames[new_frame_id];
				ball->last_running_frame_idx = new_frame_id;
			}
			sprite->orientation = sprite_orientation;
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
