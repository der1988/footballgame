#include "macro.h"
#include "ai.h"
#include "constants.h"
#include "entity_components.h"
#include "entity_player.h"
#include "entity.h"
#include "football.h"
#include "game.h"
#include "types.h"
#include <raymath.h>

struct goalie_ai_decision ai_goalie_think(struct entity *goalie_entity,
										  struct game_state *game_state)
{
	const struct component_player *player = &goalie_entity->player;
	const enum team_side opposing_team = OPPONENT_TEAM_SIDE(player->team_side);
	const enum pitch_side pitch_side =
		game_state->vars.teams[player->team_side].play_context.pitch_side;

	// He looks up before doing anything, rather than acting on the tick he gathers it
	if (!HAS_TIME_PASSED(game_state->time,
						 goalie_entity->goalie.state_machine.state_data.hold_ball.hold_start_time,
						 GOALIE_AI_THINK_TIME)) {
		return (struct goalie_ai_decision){.action = GAA_HOLD, .target_entity_id = EID_NOT_VALID};
	}

	struct nearest_player nearest_opponent = {0};
	const b32 is_opponent_found = entity_player_find_nearest(goalie_entity->transform.position,
															 opposing_team,
															 LIST_ENTITY_IDS_EMPTY,
															 &nearest_opponent,
															 game_state);

	// Closed down, so he gets rid of it
	if (is_opponent_found && nearest_opponent.distance < GOALIE_AI_PRESSURE_RADIUS) {
		return (struct goalie_ai_decision){
			.action = GAA_KICK, .power = 1.0f, .target_entity_id = EID_NOT_VALID};
	}

	// A team mate he can reach by hand, with nobody in the way of it
	const v2 up_the_pitch = {.y = pitch_side == PSI_NORTH ? 1.0f : -1.0f};
	struct entity *throw_target;
	if (ai_pass_find_target(goalie_entity,
							up_the_pitch,
							GOALIE_AI_THROW_HALF_ANGLE,
							GOALIE_AI_THROW_RANGE * 0.6f,
							GOALIE_AI_THROW_RANGE,
							true,
							&throw_target,
							game_state)) {
		const f32 distance_to_target =
			Vector2Distance(goalie_entity->transform.position, throw_target->transform.position);
		return (struct goalie_ai_decision){
			.action = GAA_THROW,
			.power = Clamp(distance_to_target / GOALIE_AI_THROW_RANGE, 0.2f, 1.0f),
			.target_entity_id = throw_target->header.id};
	}

	// Nobody near him at all, so he sets it down and plays out
	if (!is_opponent_found || nearest_opponent.distance > GOALIE_AI_PLAY_OUT_RADIUS) {
		return (struct goalie_ai_decision){.action = GAA_PUT_DOWN,
										   .target_entity_id = EID_NOT_VALID};
	}

	// Otherwise he walks out towards the edge of his area, where all of this is asked again
	const struct goal *goal = &game_state->vars.pitch.goal[pitch_side];
	const v2 area_edge = {.x = goal->penalty_box.x + goal->penalty_box.width * 0.5f,
						  .y = pitch_side == PSI_NORTH ?
								   goal->penalty_box.y + goal->penalty_box.height :
								   goal->penalty_box.y};

	return (struct goalie_ai_decision){
		.action = GAA_CARRY,
		.steering = entity_player_steer_arrive(goalie_entity, area_edge, game_state),
		.target_entity_id = EID_NOT_VALID};
}
