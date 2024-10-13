#include "ai.h"
#include "entities.h"
#include "math_ext.h"
#include "utils.h"
#include <raymath.h>

#define STAY_THERE_RADIUS 10.0f

static v2 pitch_pos_rel_to_abs(Rectangle pitch, v2 pos)
{
    pos.x = pitch.x + pitch.width * 0.5f + pitch.width * 0.5f * pos.x;
    pos.y = pitch.y + pitch.height * (1 - pos.y);
    return pos;
}

enum PlayerAIState ai_player_think(struct Entity *entity, v2 *target_position,
				   enum PlayerAIKickDecision *kick_decision,
				   struct GameState *game_state)
{
    enum PlayerAIState ai_state;

    struct Entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    declare_ball_entity_components(ball_entity);
    UNUSED(ball_collider);
    UNUSED(ball_physics_3d);

    // This player is the target of an automatic pass
    // Let's move towards the ball
    if (ball->pass_target_entity_id == entity->header.id)
    {
	v3 ball_future_pos;
	time_t ball_future_time;
	bool found = entities_common_can_reach_future_ball_position(
	    entity, game_state->constants.player_run_max_velocity, ball->lookahead_positions, NULL,
	    &ball_future_pos, &ball_future_time, game_state);
	if (!found)
	    ball_future_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
	*target_position = V3_TO_V2(ball_future_pos);
	ai_state = PAS_SEEK_POSITION;
    }
    else
    {
	// Let's move based on the tactics of the team
	struct TeamFormationPosition *team_formation_position =
	    football_tactics_get_team_formation_position(
		TF_433, entity->player.team_formation_position_idx);
	*target_position = pitch_pos_rel_to_abs(game_state->vars.pitch.field,
						team_formation_position->position[TM_DEFAULT]);
	if (Vector2Distance(*target_position, entity->transform.position) < STAY_THERE_RADIUS)
	    ai_state = PAS_KEEP_POSITION;
	else
	    ai_state = PAS_ARRIVE_TO_POSITION;
    }

    // TODO: implement kick decision logic
    *kick_decision = PAKD_NO_KICK;

    return ai_state;
}
