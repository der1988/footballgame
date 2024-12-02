#include "ai.h"
#include "entities.h"
#include "football.h"
#include "math_ext.h"
#include "types.h"
#include "game.h"

#define STAY_THERE_RADIUS 30.0f

enum player_ai_state ai_player_think(struct entity *entity,
				     v2 *target_position,
				     enum player_ai_kick_decision *kick_decision,
				     struct game_state *game_state)
{
    enum player_ai_state ai_state;

    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    declare_ball_entity_components(ball_entity);
    UNUSED(ball_collider);
    UNUSED(ball_physics_3d);

    declare_entity_component(entity, transform);

    f32 distance_from_ball = Vector2Distance(ball_transform->position, transform->position);

    b32 nobody_on_the_ball = ball->last_touch_entity_id == EID_NOT_VALID ||
			     HAS_TIME_PASSED(game_state->time, ball->last_touch_time, 0.2f);

    // This player is the target of an automatic pass
    // Let's move towards the ball
    if (ball->pass_target_entity_id == entity->header.id) {
	v3 ball_future_pos;
	time_t ball_future_time;
	b32 found = entities_common_can_reach_future_ball_position(
	    entity,
	    game_state->constants.player_run_max_velocity,
	    ball->lookahead_positions,
	    NULL,
	    &ball_future_pos,
	    &ball_future_time,
	    game_state);
	if (!found)
	    ball_future_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
	*target_position = V3_TO_V2(ball_future_pos);
	ai_state = PAS_SEEK_POSITION;
    } else if (ball->future_bounce.available &&
	       Vector2Distance(transform->position, ball->future_bounce.position) < 50 &&
	       nobody_on_the_ball) {
	*target_position = ball->future_bounce.position;
	ai_state = PAS_ARRIVE_TO_POSITION;
    } else if (distance_from_ball < 50 && nobody_on_the_ball) {
	v3 ball_future_pos;
	time_t ball_future_time;
	b32 found = entities_common_can_reach_future_ball_position(
	    entity,
	    game_state->constants.player_run_max_velocity,
	    ball->lookahead_positions,
	    NULL,
	    &ball_future_pos,
	    &ball_future_time,
	    game_state);
	if (!found)
	    ball_future_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
	*target_position = V3_TO_V2(ball_future_pos);
	ai_state = PAS_SEEK_POSITION;
    } else {
	// Let's move based on the tactics of the team
	struct team_formation_position *team_formation_position =
	    football_tactics_get_team_formation_position(
		TF_433, entity->player.team_formation_position_idx);

	v2 ball_normalized_pos = football_pitch_position_abs_to_normalized(
	    game_state->vars.pitch.field, ball_transform->position);

	v2 default_pos = team_formation_position->position[TM_DEFAULT];
	v2 target_pos;

	enum pitch_side player_pitch_side =
	    game_state->vars.teams[entity->player.team_side].pitch_side;

	if (player_pitch_side == PSI_SOUTH) {
	    if (ball_normalized_pos.y < 0.5f) {
		v2 attacking_pos = team_formation_position->position[TM_ATTACKING];
		target_pos = Vector2Lerp(attacking_pos, default_pos, ball_normalized_pos.y / 0.5f);
	    } else {
		v2 defending_pos = team_formation_position->position[TM_DEFENDING];
		target_pos =
		    Vector2Lerp(default_pos, defending_pos, (ball_normalized_pos.y - 0.5f) / 0.5f);
	    }
	} else {
	    if (ball_normalized_pos.y > 0.5f) {
		v2 attacking_pos = team_formation_position->position[TM_ATTACKING];
		target_pos =
		    Vector2Lerp(default_pos, attacking_pos, (ball_normalized_pos.y - 0.5f) / 0.5f);
	    } else {
		v2 defending_pos = team_formation_position->position[TM_DEFENDING];
		target_pos = Vector2Lerp(defending_pos, default_pos, ball_normalized_pos.y / 0.5f);
	    }
	}

	// If ball is in the side lanes and player is on the other side of the pitch we need to
	// close the gap.
	if (ball_normalized_pos.x < 0.33f && target_pos.x > ball_normalized_pos.x) {
	    f32 delta = target_pos.x - ball_normalized_pos.x;
	    target_pos = Vector2Lerp(
		Vector2Add(target_pos, (v2){.x = -delta * (0.33f - ball_normalized_pos.x)}),
		target_pos,
		ball_normalized_pos.x / 0.33f);
	} else if (ball_normalized_pos.x > 0.66f && target_pos.x < ball_normalized_pos.x) {
	    f32 delta = ball_normalized_pos.x - target_pos.x;
	    target_pos = Vector2Lerp(target_pos,
				     Vector2Add(target_pos,
						(v2){.x = delta * (ball_normalized_pos.x - 0.66f)}),
				     (ball_normalized_pos.x - 0.66f) / 0.33f);
	}

	if (player_pitch_side == PSI_NORTH) {
	    target_pos.y = 1 - target_pos.y;
	}

	*target_position =
	    football_pitch_position_normalized_to_abs(game_state->vars.pitch.field, target_pos);
	if (Vector2Distance(*target_position, entity->transform.position) < STAY_THERE_RADIUS)
	    ai_state = PAS_KEEP_POSITION;
	else
	    ai_state = PAS_ARRIVE_TO_POSITION;
    }

    // TODO: implement kick decision logic
    *kick_decision = PAKD_NO_KICK;

    return ai_state;
}
