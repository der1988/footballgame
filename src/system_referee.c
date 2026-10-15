#include "system.h"
#include "entity.h"
#include "game.h"
#include "physics.h"
#include "sprite.h"
#include <raymath.h>

void system_referee(struct game_state *game_state)
{
	struct entity *ball = game_state->vars.ball.entity;
	if (!ball || !ball->header.alive) return;
	const rect_t field = game_state->vars.pitch.field;
	FOR_EACH_ENTITY(referee, game_state->entity_repository)
	{
		if (referee->header.type != ET_REFEREE ||
			!entity_is_alive_and_has_components(referee, 3, C_TRANSFORM, C_PHYSICS, C_SPRITE))
			continue;

		// Trail the play diagonally, with room to watch instead of chasing the ball itself.
		v2 target = Vector2Add(ball->transform.position,
			(v2){-80, ball->transform.position.y < field.y + field.height * 0.5f ? 65 : -65});
		target.x = Clamp(target.x, field.x + 24, field.x + field.width - 24);
		target.y = Clamp(target.y, field.y + 24, field.y + field.height - 24);
		v2 desired = Vector2Scale(Vector2Subtract(target, referee->transform.position), 2);
		// Steer around nearby players and keep out of the immediate ball contest.
		FOR_EACH_ENTITY(other, game_state->entity_repository)
		{
			if (!other->header.alive || (other->header.type != ET_PLAYER && other != ball))
				continue;
			v2 away = Vector2Subtract(referee->transform.position, other->transform.position);
			f32 distance = Vector2Length(away);
			const f32 clearance = other == ball ? 45 : 30;
			if (distance < clearance) {
				v2 direction = distance > 0.001f ? Vector2Scale(away, 1 / distance) : (v2){-1, 0};
				desired = Vector2Add(desired, Vector2Scale(direction, 150 * (1 - distance / clearance)));
			}
		}
		const f32 speed = Vector2Length(desired);
		if (speed > referee->physics.max_velocity)
			desired = Vector2Scale(desired, referee->physics.max_velocity / speed);
		else if (speed < 2)
			desired = Vector2Zero();
		referee->physics.velocity = Vector2MoveTowards(referee->physics.velocity, desired,
			360 * game_state->dt);
		physics_apply_velocity(&referee->transform, &referee->physics, game_state->dt);
		referee->transform.position.x = Clamp(referee->transform.position.x,
			field.x + 16, field.x + field.width - 16);
		referee->transform.position.y = Clamp(referee->transform.position.y,
			field.y + 16, field.y + field.height - 16);

		if (Vector2LengthSqr(referee->physics.velocity) > 4) {
			referee->sprite.orientation = sprite_calculate_orientation(referee->physics.velocity);
			referee->sprite.frame_id = game_state->gfx.referee.run_frames
				[referee->sprite.orientation][(u32)(game_state->time / 0.1) % 4];
		} else {
			referee->sprite.orientation = sprite_calculate_orientation(
				Vector2Subtract(ball->transform.position, referee->transform.position));
			referee->sprite.frame_id =
				game_state->gfx.referee.stand_frames[referee->sprite.orientation];
		}
	}
}
