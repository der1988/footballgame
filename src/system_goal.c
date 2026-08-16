#include "system.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_goal.h"
#include "events_router.h"
#include "game.h"
#include "log.h"

void system_goal(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	for (usize i = 0; i < PSI_COUNT; ++i) {
		struct entity *goal_entity = game_state->vars.pitch.goal[i].entity;

		DECLARE_ENTITY_COMPONENT(goal_entity, goal);

		if (game_state->vars.match.state == MS_GOAL) {
			entity_ball_settle_in_net(ball_entity, entity_goal_rectangle(goal_entity), i);
			continue;
		} else if (game_state->vars.match.state != MS_PLAY) {
			continue;
		}

		struct goal_collision collision =
			entity_goal_resolve_ball_impact(goal_entity, ball_entity, game_state);
		if (!collision.hit) {
			continue;
		}

		goal->scored = collision.scored;

		if (collision.woodwork != GW_NONE) {
			goal->hit_post_time = game_state->time;
			entity_ball_set_position_and_velocity(
				ball_entity, collision.deflection.position, collision.deflection.velocity);
			GLOGF_INFO(LS_GAME,
					   "hit post at %.2f, %.2f, %.2f",
					   collision.point.x,
					   collision.point.y,
					   collision.point.z);
			if (collision.scored) {
				GLOG_INFO(LS_GAME, "goal off post!");
			}
		}

		if (collision.scored) {
			events_router_publish(game_state->events_router, EVT_GOAL_SCORED, game_state);
			GLOGF_INFO(LS_GAME,
					   "goal at %.2f, %.2f, %.2f",
					   collision.point.x,
					   collision.point.y,
					   collision.point.z);
		}
	}
}
