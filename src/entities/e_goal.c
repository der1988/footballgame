#include "entities.h"
#include "e_goal.h"
#include "e_goal_net.h"
#include "log.h"
#include <assert.h>
#include <raymath.h>
#include "game.h"

struct entity *entity_goal_create(struct goal_config config, struct game_state *game_state)
{
	struct entity *entity = entity_init(game_state->entity_repository, ET_GOAL, "goal_%02d");

	// Initialise transform
	struct component_transform *transform = &entity->transform;
	transform->header.enabled = true;
	transform->rotation = 0;
	transform->virtual_height = 0;
	transform->scale = (v2){.x = 1, .y = 1};
	transform->position = config.center;

	// Create goal net
	struct entity *goal_net_entity = entity_goal_net_create(game_state);
	struct component_transform *goal_net_transform = &goal_net_entity->transform;
	goal_net_transform->parent = transform;
	goal_net_transform->position = Vector2Zero();

	// Initialise sprite
	struct component_sprite *sprite = &entity->sprite;
	sprite->header.enabled = true;
	char *posts_frame_name = "posts_0";
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

	// Initialise collider
	struct component_collider *collider = &entity->collider;
	collider->header.enabled = true;
	collider->colliders =
		memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(struct collider));
	collider->num_colliders = 1;
	collider->colliders[0].shape = CS_RECTANGLE;
	collider->colliders[0].props.rectangle.size =
		(v2){.x = game_state->vars.pitch.goal_width, .y = game_state->vars.pitch.goal_height};
	collider->colliders[0].props.rectangle.offset =
		(v2){.x = 0, .y = -collider->colliders[0].props.rectangle.size.y * 0.5f};

	// Initialise goal
	struct component_goal *goal = &entity->goal;
	goal->header.enabled = true;
	goal->side = config.side;

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "goal created");

	return entity;
}
