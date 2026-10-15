#include "entity_referee.h"
#include "entity.h"
#include "game.h"
#include <stdio.h>

struct entity *entity_referee_create(struct game_state *game_state)
{
	static const char *directions[SO_COUNT] = {
		[SO_UP] = "back", [SO_DOWN] = "front", [SO_LEFT] = "left", [SO_RIGHT] = "right",
		[SO_UP_LEFT] = "back_left", [SO_UP_RIGHT] = "back_right",
		[SO_DOWN_LEFT] = "front_left", [SO_DOWN_RIGHT] = "front_right",
	};
	for (usize direction = 0; direction < SO_COUNT; ++direction) {
		char name[SPRITE_FRAME_NAME_MAX_SIZE];
		snprintf(name, sizeof(name), "referee_%s_0", directions[direction]);
		struct asset_sprite_frame *frame =
			asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, name);
		ASSERT(frame);
		game_state->gfx.referee.stand_frames[direction] = frame->id;
		for (usize i = 0; i < 4; ++i) {
			snprintf(name, sizeof(name), "referee_run_%s_%zu", directions[direction], i);
			frame = asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, name);
			ASSERT(frame);
			game_state->gfx.referee.run_frames[direction][i] = frame->id;
		}
	}

	struct entity *entity = entity_init(game_state->entity_repository, ET_REFEREE, "referee_%02d");
	entity->transform.header.enabled = true;
	entity->transform.position = game_state->vars.pitch.midfield_spot;
	entity->transform.position.x -= 80;
	entity->transform.position.y += 65;
	entity->transform.scale = (v2){1, 1};
	entity->physics.header.enabled = true;
	entity->physics.mass = 1;
	entity->physics.max_velocity = 125;
	entity->sprite.header.enabled = true;
	entity->sprite.color = WHITE;
	entity->sprite.pivot_override = (v2){0.5f, 0.65f};
	entity->sprite.layer = SL_PLAYERS_AND_BALL;
	entity->sprite.orientation = SO_UP;
	entity->sprite.frame_id = game_state->gfx.referee.stand_frames[SO_UP];
	return entity;
}
