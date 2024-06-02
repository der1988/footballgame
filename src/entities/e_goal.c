#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../utils/utils.h"
#include "config.h"
#include "sprites/sprites.h"
#include "types.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Entity *entities_goal_create(GoalConfig config, GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_GOAL, "goal_%02d");

    // Initialise transform
    ComponentTransform *transform =
        entities_component_create(game_state->entity_repository, CT_TRANSFORM, entity->id);
    transform->rotation = 0;
    transform->virtual_height = 0;
    transform->scale = (v2){.x = 1, .y = 1};
    if (config.position == GP_NORTH) {
        transform->position = (Vector2){game_state->vars.pitch.rectangle.x +
                                            game_state->vars.pitch.rectangle.width / 2.0f,
                                        game_state->vars.pitch.rectangle.y};
    } else {
        // Not implemented
        assert(false);
    }

    // Create goal net
    Entity *goal_net_entity = entities_goal_net_create(game_state);
    ComponentTransform *goal_net_transform = entities_component_get_typed(
        game_state->entity_repository, CT_TRANSFORM, goal_net_entity->id);
    goal_net_transform->parent = transform;
    goal_net_transform->position = Vector2Zero();

    // Initialise sprite
    ComponentSprite *sprite =
        entities_component_create(game_state->entity_repository, CT_SPRITE, entity->id);
    char *posts_frame_name = "posts_0";
    AssetsSpriteFrame *sprite_frame =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, posts_frame_name);
    assert(sprite_frame != NULL);
    sprite->frame_id = sprite_frame->id;
    sprite->color = WHITE;
    sprite->flipX = false;
    sprite->flipY = false;
    sprite->pivot_override = (v2){.x = 0.5, .y = 1};
    sprite->layer = SL_NORTH_GOAL;
    sprite->draw_shadow = false;
    sprite->shadow_color = BLACK;
    sprite->shadow_color.a = 100;
    sprite->shadow_offset = (v2){.x = 0.05, .y = 0.1};
    sprite->shadow_scale = (v2){.x = 1, .y = 1};

    // Initialise collider
    ComponentCollider *collider =
        entities_component_create(game_state->entity_repository, CT_COLLIDER, entity->id);
    collider->colliders =
        memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(Collider));
    collider->num_colliders = 1;
    collider->colliders[0].shape = CS_RECTANGLE;
    collider->colliders[0].props_rectangle.size = (v2){.x = 79, .y = 27};
    collider->colliders[0].props_rectangle.offset =
        (v2){.x = -collider->colliders[0].props_rectangle.size.x / 2.0f,
             .y = -collider->colliders[0].props_rectangle.size.y};

    // Initialise goal
    entities_component_create(game_state->entity_repository, CT_GOAL, entity->id);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "goal created");

    return entity;
}
