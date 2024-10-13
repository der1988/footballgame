#include "components.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "layers.h"
#include "render.h"
#include "simulation.h"
#include <raylib.h>
#include <raymath.h>
#include "math_ext.h"

static b32 is_applied_after_touch_same_side(v2 stored_side, v2 selected_side)
{
    f32 projection = Vector2ProjectScalar(stored_side, selected_side);
    if (projection > 0)
    {
	return true;
    }

    return false;
}

void systems_ball(struct GameState *game_state, struct input *input)
{
    for_each_entity(entity, game_state->entity_repository)
    {
	if (!entities_is_entity_alive_and_has_components(entity, 5, C_TRANSFORM, C_PHYSICS_3D,
							 C_SPRITE, C_BALL, C_COLLIDER))
	{
	    continue;
	}

	declare_entity_component(entity, Transform, transform);
	declare_entity_component(entity, Physics3d, physics_3d);
	declare_entity_component(entity, Sprite, sprite);
	declare_entity_component(entity, Ball, ball);
	declare_entity_component(entity, Collider, collider);

	struct StateMachineBall *state_machine = ball->state_machine;

	if (input->buttons[IBT_BUTTON_START].released)
	{
	    struct ComponentTransform *p1_transform =
		&entities_entity_get(game_state->entity_repository,
				     game_state->vars.p1.player_entity_id)
		     ->transform;
	    entities_ball_set_position_and_velocity(
		entity_id, V2_TO_V3(Vector2Add(p1_transform->position, (v2){.y = -20}), 50),
		Vector3Zero(), game_state);
	    football_match_change_state(game_state, MS_PLAY);
	    ball->last_player_on_the_ball_entity_id = -1;
	    return;
	}

	// Test cross-bar
	if (input->buttons[IBT_BUTTON_SELECT].released)
	{
	    entities_ball_set_position_and_velocity(
		entity_id, (v3){.x = game_state->vars.pitch.field.x + 340, .z = 300},
		Vector3Scale((v3){.x = 0, .y = 1, .z = -1}, 413.5f), game_state);
	    football_match_change_state(game_state, MS_PLAY);
	    ball->last_player_on_the_ball_entity_id = -1;
	    return;
	}

	// Test cross
	if (input->buttons[IBT_BUTTON_UP].released)
	{
	    entities_ball_set_position_and_velocity(
		entity_id, (v3){.x = game_state->vars.pitch.field.x + 600, .z = 100},
		Vector3Scale((v3){.x = -1, .y = 1.0f, .z = 0.2}, 280.5f), game_state);
	    football_match_change_state(game_state, MS_PLAY);
	    ball->last_player_on_the_ball_entity_id = -1;
	    return;
	}

	// Test goalie
	if (input->buttons[IBT_BUTTON_TRIGGER_RIGHT].released)
	{
	    entities_ball_set_position_and_velocity(
		entity_id, (v3){.x = game_state->vars.pitch.field.x + 340, .z = 300},
		Vector3Scale((v3){.x = 0, .y = 1, .z = -1}, 75.0f), game_state);
	    football_match_change_state(game_state, MS_PLAY);
	    ball->last_player_on_the_ball_entity_id = -1;
	    return;
	}

	// Sprite draw ordering when ball is in upper goal
	sprite->layer = transform->position.y < game_state->vars.pitch.field.y ?
			    SL_NORTH_GOAL :
			    SL_PLAYERS_AND_BALL;

	layers_debug_queue_draw_text(
	    game_state->layer_debug,
	    TextFormat(
		"%-8s %02d pos: [%7.2f, %7.2f, %7.2f] vel: [%6.2f, %6.2f, %6.2f] speed: %6.2f",
		"Ball", game_state->vars.ball_entity_id, transform->position.x,
		transform->virtual_height, transform->position.y, physics_3d->velocity.x,
		physics_3d->velocity.y, physics_3d->velocity.z,
		Vector3Length(physics_3d->velocity)));

	switch (state_machine->current_state)
	{
	case BS_NONE:
	    break;
	case BS_IN_PLAY:
	{
	    v3 position = V2_TO_V3(transform->position, transform->virtual_height);
	    v3 velocity = physics_3d->velocity;
	    v3 accel = physics_3d->accel;

	    // After touch
	    f32 after_touch_time = (game_state->time - ball->last_kick_time);
	    // TODO: the following has to be fixed but I can't be bothered to do that now:
	    // we want the human player that kicked the ball to be able to control after touch.
	    // Also we'd like CPU controlled players to be able to control after touch in some
	    // way.
	    if (ball->subject_to_after_touch &&
		after_touch_time > game_state->constants.ball_after_touch_delay_time)
	    {
		v2 input = game_state->vars.p1.controls.steering;
		v2 ball_2d_vel = V3_TO_V2(velocity);
		v2 ball_2d_vel_perp = {.x = ball_2d_vel.y, .y = -ball_2d_vel.x};
		v2 after_touch_side = Vector2Zero();
		f32 after_touch_pressure =
		    Vector2ProjectExt(ball_2d_vel_perp, input, &after_touch_side);
		b32 apply_after_touch = true;

		if (ball->selected_after_touch_side.x == 0 &&
		    ball->selected_after_touch_side.y == 0)
		{
		    // After touch side not selected yet
		    if (fabsf(after_touch_pressure) > 0.25f)
		    {
			// We need some after touch to be actually applied
			ball->selected_after_touch_side = after_touch_side;
		    }
		    else
		    {
			apply_after_touch = false;
		    }
		}
		else if (!is_applied_after_touch_same_side(ball->selected_after_touch_side,
							   after_touch_side))
		{
		    // If player changed side of after touch we keep applying the first one
		    // we've seen
		    after_touch_side = ball->selected_after_touch_side;
		}

		if (apply_after_touch)
		{
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
				  0, 1);

		    // Apply it
		    accel = Vector3Add(
			accel, Vector3Scale(after_touch_direction,
					    game_state->constants.ball_after_touch_force * force));
		}
	    }

	    b32 on_the_ground;
	    b32 bounced;
	    f32 ball_radius = collider->colliders[0].props.circle.radius;
	    sim_ball_physics(&position, &velocity, accel, physics_3d->mass, ball_radius,
			     &on_the_ground, &bounced, game_state);
	    ball->is_on_the_ground = on_the_ground && !bounced;
	    if (bounced)
	    {
		ball->last_bounce_time = game_state->time;
	    }
	    if (ball->is_on_the_ground)
	    {
		ball->future_bounce_position = V3_TO_V2(position);
		ball->dts_to_future_bounce = 0;
	    }

	    // Calculate next 'ball_lookaheads' look ahead positions, ball_lookahead_dts dts apart from each other
	    b32 lookahead_on_the_ground;
	    b32 lookahead_bounced;
	    v3 lookahead_position = position;
	    v3 lookahead_velocity = velocity;
	    u16 lookaheads = game_state->constants.ball_lookaheads;
	    u32 lookahead_dts =
		(game_state->vars.slow_motion ? 10 : 1) * game_state->constants.ball_lookahead_dts;
	    b32 found_first_bounce = false;
	    for (u16 i = 0; i < lookaheads; ++i)
	    {
		// 10 dt steps (100 in slow motion)
		for (u16 j = 0; j < lookahead_dts; ++j)
		{
		    sim_ball_physics(&lookahead_position, &lookahead_velocity, Vector3Zero(),
				     physics_3d->mass, ball_radius, &lookahead_on_the_ground,
				     &lookahead_bounced, game_state);
		    if (!found_first_bounce && lookahead_bounced)
		    {
			ball->future_bounce_position = V3_TO_V2(lookahead_position);
			ball->dts_to_future_bounce =
			    i * game_state->constants.ball_lookaheads + j + 1;
			found_first_bounce = true;
		    }
		}
		ball->lookahead_positions[i] = lookahead_position;
	    }

	    // Project position to 2D
	    transform->position = V3_TO_V2(position);
	    transform->virtual_height = position.y;

	    physics_3d->velocity = velocity;
	    // Acceleration does not accumulate
	    physics_3d->accel = Vector3Zero();

	    if (bounced)
	    {
		ball->subject_to_after_touch = false;
	    }

	    // Calculate frame to display
	    Vector2 ball_velocity = V3_TO_V2(physics_3d->velocity);
	    if (Vector2Length(ball_velocity) > 3)
	    {
		enum Direction pd = entities_common_calculate_direction(ball_velocity);
		struct AssetsAnimation *animation = assets_animation_get_by_id(
		    game_state->animation_cache, ball->roll_animations[pd]);
		f32 ball_max_velocity = game_state->constants.ball_max_velocity / 10.0f;
		f32 delay =
		    (animation->delay *
		     (5 - 4 * Clamp(Vector2Length(ball_velocity) / ball_max_velocity, 0, 1)));
		if ((game_state->time - ball->last_frame_update_time) > delay)
		{
		    ball->last_frame_update_time = game_state->time;
		    u16 new_frame_id = (ball->last_running_frame_idx + 1) % animation->frames_count;
		    sprite->frame_id = animation->frames[new_frame_id];
		    ball->last_running_frame_idx = new_frame_id;
		}
		ball->last_direction = pd;
	    }
	}
	break;
	case BS_OUT_OF_PLAY:
	{
	    break;
	}
	case BS_GK_HANDS:
	{
	    break;
	}
	case BS_COUNT:
	    break;
	}
    }
}
