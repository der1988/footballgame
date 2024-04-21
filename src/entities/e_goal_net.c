#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../utils/utils.h"
#include "sprites/sprites.h"
#include "types.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Entity *entities_goal_net_create(GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_GOAL, "goal_net_%02d");

    // Initialise transform
    Component *component =
        entities_component_create(game_state->entity_repository, CT_TRANSFORM, entity->id);
    component->transform.position = (Vector2){(f32)GetScreenWidth() / 2, -10};
    component->transform.rotation = 0;
    component->transform.virtual_height = 0;
    component->transform.scale = (v2){.x = 1, .y = 1};

    // Initialise sprite
    component = entities_component_create(game_state->entity_repository, CT_SPRITE, entity->id);
    char *posts = "net_0";
    AssetsSpriteFrame *sprite_frame =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, posts);
    assert(sprite_frame != NULL);
    component->sprite.frame_id = sprite_frame->id;
    component->sprite.color = WHITE;
    component->sprite.flipX = false;
    component->sprite.flipY = false;
    component->sprite.pivot_override = (v2){.x = 0.5, .y = 1};
    component->sprite.layer = SL_BACKGROUND;
    component->sprite.draw_shadow = false;
    component->sprite.shadow_color = BLACK;
    component->sprite.shadow_color.a = 100;
    component->sprite.shadow_offset = (v2){.x = 0.05, .y = 0.1};
    component->sprite.shadow_scale = (v2){.x = 1, .y = 1};

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "goal net created");

    return entity;
}
