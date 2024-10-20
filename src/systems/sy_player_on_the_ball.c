#include "components.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "layers.h"
#include "types.h"
#include "macro.h"
#include "log.h"
#include <math.h>
#include <math_ext.h>
#include <raylib.h>
#include <raymath.h>
#include "ai.h"

static entity_id_t find_pass_target(struct game_state *game_state,
				    struct player_on_the_ball *player_on_the_ball,
				    f32 detection_angle,
				    f32 max_distance)
{
    struct entity_repository *entity_repository = game_state->entity_repository;
    entity_id_t selected_entity_id = EID_NOT_VALID;
    f32 selected_distance = UINT32_MAX;
    struct entity *entity;
    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
	entity = entities_entity_get(game_state->entity_repository, entity_id);
	if (entity->header.alive && entity->header.type == ET_PLAYER) {

	    struct component_transform *transform = &entity->transform;

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
		selected_distance = distance_from_player;

		GLOGF_DEBUG(LS_GAME,
			    "found valid pass target: %zu, %.2f, %.2f",
			    entity_id,
			    distance_from_player,
			    angle);
	    }
	}
    }
    if (selected_entity_id != EID_NOT_VALID) {
	GLOGF_DEBUG(
	    LS_GAME, "selected pass target %zu at %.2f", selected_entity_id, selected_distance);
    }
    return selected_entity_id;
}

void systems_player_on_the_ball(struct game_state *game_state, struct input *input)
{
    UNUSED(input);

    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    declare_ball_entity_components(ball_entity);
    UNUSED(ball_physics_3d);
    UNUSED(ball_collider);

    struct player_on_the_ball *player_on_the_ball = &game_state->vars.player_on_the_ball;

    b32 kicked_ball = false;
    if (player_on_the_ball->id != EID_NOT_VALID) {
	f32 player_speed = Vector2Length(player_on_the_ball->velocity);
	v2 normalized_velocity = Vector2Normalize(player_on_the_ball->velocity);
	f32 steering_magnitude = Vector2Length(player_on_the_ball->controls->steering);
	// b32 just_got_control = ball->last_player_on_the_ball_entity_id != player_on_the_ball->id;
	ball->last_player_on_the_ball_entity_id = player_on_the_ball->id;

	if (player_on_the_ball->controls->shot > 0 && steering_magnitude > 0.1f) {
	    // Shot
	    player_on_the_ball->player->last_kick_time = game_state->time;
	    v2 shot_direction;

	    if (!game_state->constants.shoot_aid_enabled) {
		shot_direction = Vector2Normalize(player_on_the_ball->controls->steering);
	    } else {
		v2 center_of_goal = (Vector2){game_state->vars.pitch.field.x +
						  game_state->vars.pitch.field.width / 2.0f,
					      game_state->vars.pitch.field.y};
		v2 shot_direction_to_center =
		    Vector2Normalize(Vector2Subtract(center_of_goal, ball_transform->position));
		v2 shot_direction_to_right_post = Vector2Normalize(Vector2Subtract(
		    Vector2Add(center_of_goal, (v2){.x = 50}), ball_transform->position));
		f32 angle_center_to_post =
		    Vector2Angle(shot_direction_to_center, shot_direction_to_right_post);

		shot_direction =
		    Vector2Rotate(shot_direction_to_center,
				  angle_center_to_post * player_on_the_ball->controls->steering.x);
	    }

	    // Limit angle between requested shoot direction and current velocity
	    f32 angle = Vector2Angle(player_on_the_ball->velocity, shot_direction);
	    f32 max_angle = 100 * DEG2RAD;
	    if (angle > max_angle) {
		shot_direction =
		    Vector2Normalize(Vector2Rotate(player_on_the_ball->velocity, max_angle));
	    } else if (angle < -max_angle) {
		shot_direction =
		    Vector2Normalize(Vector2Rotate(player_on_the_ball->velocity, -max_angle));
	    }
	    entities_ball_shoot(game_state->vars.ball_entity_id,
				player_on_the_ball->id,
				shot_direction,
				player_on_the_ball->controls->shot,
				game_state);
	    kicked_ball = true;
	} else if (player_on_the_ball->controls->pass > 0 && steering_magnitude > 0.1f) {
	    // Pass
	    player_on_the_ball->player->last_kick_time = game_state->time;
	    v3 pass_direction;
	    entity_id_t teammate_entity_id =
		find_pass_target(game_state, player_on_the_ball, 30, 300);
	    f32 power = player_on_the_ball->controls->pass;
	    b32 automatic_pass = teammate_entity_id != EID_NOT_VALID &&
				 player_on_the_ball->controls->automatic_pass;
	    if (automatic_pass) {
		struct component_transform *teammate_transform =
		    &entities_entity_get(game_state->entity_repository, teammate_entity_id)
			 ->transform;
		v2 to_teammate =
		    Vector2Subtract(teammate_transform->position, player_on_the_ball->position);
		pass_direction = Vector3Normalize(V2_TO_V3(to_teammate, 0));
		ball->pass_target_entity_id = teammate_entity_id;
		power = Vector2Length(to_teammate) / 300.0f;
	    } else {
		pass_direction =
		    Vector3Normalize(V2_TO_V3(player_on_the_ball->controls->steering, 0));
	    }
	    entities_ball_short_pass(game_state->vars.ball_entity_id,
				     player_on_the_ball->id,
				     pass_direction,
				     power,
				     automatic_pass,
				     game_state);
	    kicked_ball = true;
	} else if (player_on_the_ball->controls->long_pass && steering_magnitude > 0.1f) {
	    // Long pass
	    player_on_the_ball->player->last_kick_time = game_state->time;
	    v2 long_pass_direction = Vector2Normalize(player_on_the_ball->controls->steering);
	    // Limit angle between requested shoot direction and current velocity
	    f32 angle = Vector2Angle(player_on_the_ball->velocity, long_pass_direction);
	    f32 max_angle = 120 * DEG2RAD;
	    if (angle > max_angle) {
		long_pass_direction =
		    Vector2Normalize(Vector2Rotate(player_on_the_ball->velocity, max_angle));
	    } else if (angle < -max_angle) {
		long_pass_direction =
		    Vector2Normalize(Vector2Rotate(player_on_the_ball->velocity, -max_angle));
	    }
	    entities_ball_long_pass(game_state->vars.ball_entity_id,
				    player_on_the_ball->id,
				    long_pass_direction,
				    player_on_the_ball->controls->long_pass,
				    game_state);
	    kicked_ball = true;
	} else if (!kicked_ball && steering_magnitude > 0.01f) {
	    // For ball control we use velocity directly because it's easier and produce
	    // better results
	    if (player_speed < game_state->constants.player_run_max_velocity * 0.7f) {
		player_on_the_ball->player->control_ball = true;
		player_on_the_ball->player->time_spent_in_run_control = 0;
	    }
	    if (player_on_the_ball->player->control_ball) {
		// Precision control
		player_on_the_ball->player->time_spent_in_precision_control += game_state->dt;
		player_on_the_ball->player->time_spent_in_run_control = 0;
		if (player_on_the_ball->player->time_spent_in_precision_control > 0.25f) {
		    player_on_the_ball->player->control_ball = false;
		}
		layers_debug_queue_draw_text(
		    game_state->layer_debug,
		    TextFormat("prec_ctrl: %.2f",
			       player_on_the_ball->player->time_spent_in_precision_control));
		v2 position_to_seek = Vector2Add(
		    player_on_the_ball->position,
		    Vector2Scale(normalized_velocity,
				 player_on_the_ball->collider->colliders[0].props.circle.radius *
				     1));
		// TODO: this should depend on player technical abilities
		if (player_on_the_ball->player->time_spent_in_precision_control < 1) {
		    entities_ball_control_precision(game_state->vars.ball_entity_id,
						    player_on_the_ball->id,
						    player_on_the_ball->velocity,
						    position_to_seek,
						    1,
						    game_state);
		}
	    } else {
		// Run control
		player_on_the_ball->player->time_spent_in_run_control += game_state->dt;
		player_on_the_ball->player->time_spent_in_precision_control = 0;
		if (player_on_the_ball->player->time_spent_in_run_control > 0.25f) {
		    player_on_the_ball->player->control_ball = true;
		}
		v2 control_direction = player_on_the_ball->controls->steering;

		// The control ratio should depend on various variables:
		// - speed of the player
		// - technical abilities of the player
		// - height of the ball
		// - pitch conditions
		//
		// Each one should have a specific weight applied.
		//
		// Range is 0.0-1.0 with 1.0 being the best control quality
		//
		// This will take some time to get right
		//
		// TODO: add what's missing

		// The faster the player is running the less control
		f32 speed_mult = 1 - powf(player_speed, 4) /
					 powf(game_state->constants.player_run_max_velocity, 4);
		// The higher the ball is the less control the player has
		f32 ball_height = ball_transform->virtual_height;
		f32 ball_height_mult =
		    1 - (ball_height - 2.5f) / (game_state->constants.player_control_height -
						2.5f); // Ball center is 2.5f over pitch

		// Let's give weight to each multiplier
		f32 control_ratio = (speed_mult * 1 + ball_height_mult * 1) / 2.0f;

		entities_ball_control_running(game_state->vars.ball_entity_id,
					      player_on_the_ball->id,
					      control_direction,
					      player_on_the_ball->velocity,
					      control_ratio,
					      game_state);
	    }
	}

	// We can reset charge now: we already kicked the ball if needed
	if (kicked_ball && player_on_the_ball->id == game_state->vars.p1.player_entity_id &&
	    game_state->vars.p1.controls.charge.ready) {
	    game_state->vars.p1.controls.charge.ready =
		game_state->vars.p1.controls.charge.charging = false;
	}
    }
}
