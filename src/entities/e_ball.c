#include "assets.h"
#include "components.h"
#include "entities.h"
#include "game.h"
#include "memory.h"
#include "sprites.h"
#include "types.h"
#include "utils.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ImplementStateMachineFunctions(Ball, ball);

void ball_prepare_gfx(ComponentBall *ball, GameState *game_state);

struct Entity *entities_ball_create(BallConfig config, GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_BALL, "ball_%02d");

    // Initialise transform
    ComponentTransform *transform = &entity->transform;
    transform->header.enabled = true;
    transform->position = config.position;
    transform->rotation = 0;
    transform->virtual_height = 100;
    transform->scale = (v2){.x = 0.625, .y = 0.625};

    // Initialise 3d physics
    ComponentPhysics3d *physics_3d = &entity->physics_3d;
    physics_3d->header.enabled = true;
    physics_3d->mass = 0.43f;
    physics_3d->velocity = (Vector3){0};
    physics_3d->accel = (Vector3){0};
    physics_3d->max_velocity = game_state->constants.ball_max_velocity;

    // Initialise sprite
    ComponentSprite *sprite = &entity->sprite;
    char *ball_frame_name = "ball_left_0";
    AssetsSpriteFrame *sprite_frame =
        assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, ball_frame_name);
    assert(sprite_frame != NULL);
    sprite->header.enabled = true;
    sprite->frame_id = sprite_frame->id;
    sprite->color = WHITE;
    sprite->flipX = false;
    sprite->flipY = false;
    sprite->pivot_override = (v2){.x = 0.5, .y = 0.5};
    sprite->layer = SL_PLAYERS_AND_BALL;
    sprite->draw_shadow = true;
    sprite->shadow_color = BLACK;
    sprite->shadow_color.a = 100;
    sprite->shadow_offset = (v2){.x = 0.02, .y = -0.01};
    sprite->shadow_scale = (v2){.x = 1, .y = 1};

    // Initialise collider
    ComponentCollider *collider = &entity->collider;
    collider->header.enabled = true;
    collider->colliders =
        memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(Collider));
    collider->num_colliders = 1;
    collider->colliders[0].shape = CS_CIRCLE;
    collider->colliders[0].props_circle.radius = 2.5f;

    // Initialise action
    entity->action.header.enabled = true;

    // Initialise ball
    ComponentBall *ball = &entity->ball;
    ball->header.enabled = true;
    StateMachineBall *state_machine =
        memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(StateMachineBall));
    state_machine->owner_entity = entity->header.id;
    state_machine->previous_state = BS_NONE;
    state_machine->current_state = BS_IN_PLAY;
    ball->state_machine = state_machine;

    ball_prepare_gfx(ball, game_state);

    entities_ball_state_machine_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "ball created");

    return entity;
}

void entities_ball_state_machine_on_enter(struct Entity *entity, GameState *game_state) {
    ComponentBall *ball = &entity->ball;
    StateMachineBall *state_machine = ball->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->header.name, "entering state: %s",
                   StateMachineBallStateString[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(StateMachineBallStateData));

    switch (state_machine->current_state) {
    case BS_NONE:
        break;
    case BS_IN_PLAY: {
        break;
    }
    case BS_OUT_OF_PLAY: {
        break;
    }
    case BS_GK_HANDS: {
        ComponentSprite *sprite = &entity->sprite;
        sprite->header.enabled = false;
    }
    case BS_COUNT:
        break;
    }
}

void entities_ball_state_machine_on_exit(struct Entity *entity, GameState *game_state) {
    ComponentSprite *sprite = &entity->sprite;
    ComponentBall *ball = &entity->ball;
    StateMachineBall *state_machine = ball->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->header.name, "exiting state: %s",
                   StateMachineBallStateString[state_machine->previous_state]);

    switch (state_machine->previous_state) {
    case BS_NONE:
        break;
    case BS_IN_PLAY: {
        break;
    }
    case BS_OUT_OF_PLAY: {
        break;
    }
    case BS_GK_HANDS: {
        sprite->header.enabled = true;
        break;
    }
    case BS_COUNT:
        break;
    }
}

void ball_prepare_gfx(ComponentBall *ball, GameState *game_state) {
    // Roll frames
    ball->roll_animations[D_UP] = entities_common_animation_id_from_name("ball", "up", game_state);
    ball->roll_animations[D_DOWN] =
        entities_common_animation_id_from_name("ball", "down", game_state);
    ball->roll_animations[D_LEFT] =
        entities_common_animation_id_from_name("ball", "left", game_state);
    ball->roll_animations[D_RIGHT] =
        entities_common_animation_id_from_name("ball", "right", game_state);
    ball->roll_animations[D_UP_LEFT] =
        entities_common_animation_id_from_name("ball", "up_left", game_state);
    ball->roll_animations[D_UP_RIGHT] =
        entities_common_animation_id_from_name("ball", "up_right", game_state);
    ball->roll_animations[D_DOWN_LEFT] =
        entities_common_animation_id_from_name("ball", "down_left", game_state);
    ball->roll_animations[D_DOWN_RIGHT] =
        entities_common_animation_id_from_name("ball", "down_right", game_state);
}

void entities_ball_set_position_and_velocity(EntityId ball_entity_id, v3 pos, v3 velocity,
                                             GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentTransform *transform = &entity->transform;
    transform->position = V3_TO_V2(pos);
    transform->virtual_height = pos.y;
    entity->physics_3d.velocity = velocity;
    entity->ball.subject_to_after_touch = false;
}

void entities_ball_control_precision(EntityId ball_entity_id, EntityId player_on_the_ball_entity_id,
                                     v2 player_velocity, v2 position_to_seek, f32 control_quality,
                                     GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentTransform *transform = &entity->transform;
    ComponentPhysics3d *physics_3d = &entity->physics_3d;
    ComponentBall *ball = &entity->ball;

    physics_3d->velocity =
        Vector3Add(V2_TO_V3(player_velocity, 0),
                   Vector3Scale(Vector3Normalize(Vector3Subtract(
                                    V2_TO_V3(position_to_seek, 0),
                                    V2_TO_V3(transform->position, transform->virtual_height))),
                                game_state->constants.ball_control_force * control_quality));
    ball->last_player_on_the_ball_entity_id = player_on_the_ball_entity_id;
}

void entities_ball_control_running(EntityId ball_entity_id, EntityId player_on_the_ball_entity_id,
                                   v2 control_direction, v2 player_velocity, f32 control_quality,
                                   GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentTransform *transform = &entity->transform;
    ComponentPhysics3d *physics_3d = &entity->physics_3d;
    ComponentBall *ball = &entity->ball;

    f32 vertical_force = transform->virtual_height > 10 ? 0 : 0.5f;
    v3 velocity_to_add = Vector3Scale(
        Vector3Add(Vector3Normalize(V2_TO_V3(control_direction, 0)), (v3){.y = vertical_force}),
        game_state->constants.ball_run_force * 0.5f +
            game_state->constants.ball_run_force * 0.5f * (1 - control_quality));
    physics_3d->velocity = Vector3Add(V2_TO_V3(player_velocity, 0), velocity_to_add);
    ball->last_player_on_the_ball_entity_id = player_on_the_ball_entity_id;
}

void entities_ball_shoot(EntityId ball_entity_id, EntityId kicker_entity_id, v2 direction,
                         f32 power, GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentTransform *transform = &entity->transform;
    ComponentPhysics3d *physics_3d = &entity->physics_3d;
    ComponentBall *ball = &entity->ball;
    ComponentCollider *collider = &entity->collider;

    v3 shot_direction =
        Vector3Add(V2_TO_V3(direction, 0),
                   (Vector3){.x = 0, .y = game_state->constants.ball_shot_height * power, .z = 0});
    if (game_state->constants.shoot_aid_no_inertia) {
        // reset velocity to avoid existing inertia affecting the trajectory: not sure this is
        // better though, need to test
        physics_3d->velocity = Vector3Zero();
    }
    // Calculate shot force
    f32 shot_force = game_state->constants.ball_shot_force;
    f32 ball_radius = collider->colliders[0].props_circle.radius;
    // if (!ball->is_on_the_ground) {
    //     if (transform->virtual_height > (ball_radius + 12)) {
    //         // TODO: this needs to handle headers
    //         GLOG_DEBUG_CTX(LS_GAME, entity->header.name, "ball shot, should be an header");
    //     } else if (transform->virtual_height > ball_radius) {
    //         if (physics_3d->velocity.y < 0) {
    //             // Ball is falling
    //             shot_force *= 1.05f;
    //             GLOG_DEBUG_CTX(LS_GAME, entity->header.name, "ball shot during fall trajectory");
    //         } else if (physics_3d->velocity.y > 0) {
    //             // If it's not falling then maybe it's a drop shot?
    //             if (IS_TIME_PASSED_IN_RANGE(game_state->time, ball->last_bounce_time, 0, 0.25f))
    //             {
    //                 // Drop shot
    //                 shot_force *= 1.10;
    //                 GLOG_DEBUG_CTX(LS_GAME, entity->header.name, "ball drop shot");
    //             } else if (IS_TIME_PASSED_IN_RANGE(game_state->time, ball->last_bounce_time,
    //             0.25f,
    //                                                0.5f)) {
    //                 // Nope, this must be the shittiest shot ever
    //                 shot_force *= 0.8f;
    //                 GLOG_DEBUG_CTX(LS_GAME, entity->header.name, "ball drop shot too late");
    //             }
    //         }
    //     }
    // }

    physics_3d->accel =
        Vector3Add(physics_3d->accel, Vector3Scale(shot_direction, (shot_force) / game_state->dt));
    ball->last_kick_time = game_state->time;
    ball->last_kicker_entity_id = kicker_entity_id;
    ball->last_player_on_the_ball_entity_id = kicker_entity_id;
    ball->subject_to_after_touch = true;
}

void entities_ball_short_pass(EntityId ball_entity_id, EntityId kicker_entity_id, v3 direction,
                              f32 power, b32 auto_pass, GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentPhysics3d *physics_3d = &entity->physics_3d;
    ComponentBall *ball = &entity->ball;

    if (auto_pass) {
        // reset velocity to avoid existing inertia affecting the trajectory
        physics_3d->velocity = Vector3Zero();
    }
    physics_3d->accel =
        Vector3Add(physics_3d->accel,
                   Vector3Scale(direction, (game_state->constants.ball_pass_force * 0.5f +
                                            power * 0.5f * game_state->constants.ball_pass_force) /
                                               game_state->dt));
    ball->last_kick_time = game_state->time;
    ball->last_kicker_entity_id = kicker_entity_id;
    ball->last_player_on_the_ball_entity_id = kicker_entity_id;
}

void entities_ball_long_pass(EntityId ball_entity_id, EntityId kicker_entity_id, v2 direction,
                             f32 power, GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentPhysics3d *physics_3d = &entity->physics_3d;
    ComponentBall *ball = &entity->ball;

    v3 long_pass_direction = Vector3Add(
        Vector3Normalize(V2_TO_V3(direction, 0)),
        (Vector3){.x = 0, .y = game_state->constants.ball_long_pass_height * power, .z = 0});
    physics_3d->accel =
        Vector3Add(physics_3d->accel,
                   Vector3Scale(long_pass_direction,
                                (game_state->constants.ball_long_pass_force * 0.75f +
                                 power * 0.25f * game_state->constants.ball_long_pass_force) /
                                    game_state->dt));
    ball->last_kick_time = game_state->time;
    ball->last_kicker_entity_id = kicker_entity_id;
    ball->last_player_on_the_ball_entity_id = kicker_entity_id;
    ball->subject_to_after_touch = true;
}

void entities_ball_held_by_goalie(EntityId ball_entity_id, EntityId goalie_entity_id,
                                  GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentBall *ball = &entity->ball;

    ball->last_player_on_the_ball_entity_id = goalie_entity_id;
    entities_ball_state_machine_change_state_to(ball->state_machine, BS_GK_HANDS, game_state);
}

void entities_ball_saved_by_goalie(EntityId ball_entity_id, EntityId goalie_entity_id,
                                   GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentBall *ball = &entity->ball;

    ball->last_player_on_the_ball_entity_id = goalie_entity_id;
}

void entities_ball_back_in_play(EntityId ball_entity_id, GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentBall *ball = &entity->ball;

    entities_ball_state_machine_change_state_to(ball->state_machine, BS_IN_PLAY, game_state);
}

void entities_ball_in_goal(EntityId ball_entity_id, GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, ball_entity_id);
    ComponentBall *ball = &entity->ball;

    ball->subject_to_after_touch = false;
}
