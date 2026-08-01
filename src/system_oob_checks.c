#include "system.h"
#include "entity.h"
#include "entity_ball.h"
#include "events_router.h"
#include "football.h"
#include "game.h"
#include "macro.h"
#include <raylib.h>
#include <raymath.h>

void system_oob_checks(struct game_state *game_state)
{
	if (game_state->vars.match.state == MS_PLAY) {
		struct entity *ball_entity = game_state->vars.ball.entity;

		DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);

		UNUSED(ball);
		UNUSED(ball_physics_3d);

		const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;

		if (!CheckCollisionCircleRec(
				ball_transform->position, ball_radius, game_state->vars.pitch.field)) {
			// Ball is out of bounds
			rect_t *pitch_rectangle = &game_state->vars.pitch.field;

			enum event_type event_to_publish;

			if (ball_transform->position.x < pitch_rectangle->x) {
				event_to_publish = EVT_BALL_OOB_WEST;
			} else if (ball_transform->position.x > (pitch_rectangle->x + pitch_rectangle->width)) {
				event_to_publish = EVT_BALL_OOB_EAST;
			} else if (ball_transform->position.y < pitch_rectangle->y) {
				event_to_publish = EVT_BALL_OOB_NORTH;
			} else if (ball_transform->position.y >
					   (pitch_rectangle->y + pitch_rectangle->height)) {
				event_to_publish = EVT_BALL_OOB_SOUTH;
			} else {
				return;
			}

			// We don't take decisions here about the consequence of the ball going out of bounds,
			// we leave it to the current manager supervising the gameplay
			events_router_publish(game_state->events_router, event_to_publish, game_state);
		}
	}
}
