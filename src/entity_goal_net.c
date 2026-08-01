#include "entity_goal_net.h"
#include "entity.h"
#include "football.h"
#include "log.h"
#include "entity_goal.h"
#include <assert.h>
#include "game.h"

struct entity *entity_goal_net_create(struct goal_config config, struct game_state *game_state)
{
	struct entity *entity =
		entity_init(game_state->entity_repository, ET_GOAL_NET, "goal_net_%02d");

	// Initialise transform
	struct component_transform *transform = &entity->transform;
	transform->header.enabled = true;
	transform->rotation = 0;
	transform->virtual_height = 0;
	transform->scale = (v2){.x = 1, .y = 1};

	// Initialise sprite
	struct component_sprite *sprite = &entity->sprite;
	sprite->header.enabled = true;
	char *posts_frame_name = config.side == PSI_NORTH ? "net_up_0" : "net_down_0";
	struct asset_sprite_frame *sprite_frame =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, posts_frame_name);
	assert(sprite_frame != NULL);
	sprite->frame_id = sprite_frame->id;
	sprite->color = WHITE;
	sprite->flipX = false;
	sprite->flipY = false;
	sprite->pivot_override = (v2){.x = 0.5, .y = 1};
	sprite->layer = config.side == PSI_NORTH ? SL_NORTH_GOAL : SL_SOUTH_GOAL;
	sprite->draw_shadow = false;
	sprite->shadow_color = BLACK;
	sprite->shadow_color.a = 100;
	sprite->shadow_offset = (v2){.x = 0.05, .y = 0.1};
	sprite->shadow_scale = (v2){.x = 1, .y = 1};

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "goal net created");

	return entity;
}
