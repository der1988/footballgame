#include "assets.h"
#include "components.h"
#include "entities.h"
#include "game.h"
#include "render.h"
#include "types.h"
#include "log.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct entity *entities_goal_net_create(struct game_state *game_state)
{
    struct entity *entity =
	entities_entity_init(game_state->entity_repository, ET_GOAL, "goal_net_%02d");

    // Initialise transform
    struct component_transform *transform = &entity->transform;
    transform->header.enabled = true;
    transform->position = (Vector2){(f32)GetScreenWidth() / 2, -10};
    transform->rotation = 0;
    transform->virtual_height = 0;
    transform->scale = (v2){.x = 1, .y = 1};

    // Initialise sprite
    struct component_sprite *sprite = &entity->sprite;
    sprite->header.enabled = true;
    char *posts_frame_name = "net_0";
    struct AssetsSpriteFrame *sprite_frame =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, posts_frame_name);
    assert(sprite_frame != NULL);
    sprite->frame_id = sprite_frame->id;
    sprite->color = WHITE;
    sprite->flipX = false;
    sprite->flipY = false;
    sprite->pivot_override = (v2){.x = 0.5, .y = 1};
    sprite->layer = SL_BACKGROUND;
    sprite->draw_shadow = false;
    sprite->shadow_color = BLACK;
    sprite->shadow_color.a = 100;
    sprite->shadow_offset = (v2){.x = 0.05, .y = 0.1};
    sprite->shadow_scale = (v2){.x = 1, .y = 1};

    GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "goal net created");

    return entity;
}
