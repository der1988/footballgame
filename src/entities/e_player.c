#include "assets.h"
#include "components.h"
#include "entities.h"
#include "game.h"
#include "memory.h"
#include "render.h"
#include "state_machine.h"
#include "types.h"
#include "log.h"
#include "utils.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants.h"
#include "math_ext.h"

implement_state_machine_functions(Player, player);
implement_state_machine_functions(Goalie, goalie);

static void prepare_gfx(struct ComponentPlayer *player, struct game_state *game_state);
static void prepare_goalie_gfx(struct ComponentPlayer *player, struct ComponentGoalie *goalie,
			       struct game_state *game_state);

struct Entity *entities_player_create(PlayerConfig config, struct game_state *game_state)
{
    struct Entity *entity =
	entities_entity_init(game_state->entity_repository, ET_PLAYER, "player_%02d");

    // Initialise transform
    struct ComponentTransform *transform = &entity->transform;
    transform->header.enabled = true;
    transform->position = config.position;
    transform->rotation = 0;
    transform->scale = (v2){.x = 1, .y = 1};

    // Initialise physics
    struct ComponentPhysics *physics = &entity->physics;
    physics->header.enabled = true;
    physics->mass = 75.0f;
    physics->velocity = (Vector2){0};
    physics->accel = (Vector2){0};
    physics->max_velocity = game_state->constants.player_control_max_velocity;

    // Initialise collider
    struct ComponentCollider *collider = &entity->collider;
    collider->header.enabled = true;

    // Initialise sprite
    char *player_frame_name = "player_front_0";
    struct AssetsSpriteFrame *sprite_frame =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, player_frame_name);
    assert(sprite_frame != NULL);
    struct ComponentSprite *sprite = &entity->sprite;
    sprite->header.enabled = true;
    sprite->frame_id = sprite_frame->id;
    sprite->color = WHITE;
    sprite->flipX = false;
    sprite->flipY = false;
    sprite->pivot_override = (v2){.x = 0.5, .y = 0.65};
    sprite->layer = SL_PLAYERS_AND_BALL;
    sprite->draw_shadow = false;
    // sprite->shadow_color = BLACK;
    // sprite->shadow_color.a = 100;
    // sprite->shadow_offset = (v2){.x = 0.5, .y = 0.5};
    // sprite->shadow_scale = (v2){.x = 0.6, .y = 1};

    // Initialise player
    struct ComponentPlayer *player = &entity->player;
    player->header.enabled = true;
    player->side = config.side;
    player->team_formation_position_idx = config.team_formation_position_idx;
    // Sets initial max velocity to half of max run velocity
    player->current_max_velocity = game_state->constants.player_run_max_velocity * 0.5f;
    struct StateMachinePlayer *player_state_machine =
	memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(*player_state_machine));
    player_state_machine->owner_entity = entity->header.id;
    player_state_machine->previous_state = PS_NONE;
    player->state_machine = player_state_machine;

    // Initialise action
    struct ComponentAction *action = &entity->action;
    action->header.enabled = true;

    if (config.is_goalie)
    {
	// Initialise goalie
	struct ComponentGoalie *goalie = &entity->goalie;
	goalie->header.enabled = true;
	struct StateMachineGoalie *state_machine =
	    memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(*state_machine));
	state_machine->owner_entity = entity->header.id;
	state_machine->previous_state = GS_NONE;
	state_machine->current_state = GS_PROTECT_GOAL;
	goalie->state_machine = state_machine;
	player_state_machine->current_state = PS_GOALIE;

	entities_goalie_state_machine_on_enter(entity, game_state);

	prepare_goalie_gfx(player, &entity->goalie, game_state);

	collider->num_colliders = PCT_COUNT;
	collider->colliders = memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL],
						sizeof(struct Collider) * collider->num_colliders);
	collider->colliders[PCT_GOALIE].shape = CS_CIRCLE;
	collider->colliders[PCT_GOALIE].props.circle.offset = (v2){.y = 0};
	collider->colliders[PCT_GOALIE].props.circle.radius = 6;

	collider->colliders[PCT_DIVE].shape = CS_RECTANGLE;
	collider->colliders[PCT_DIVE].props.rectangle.offset = (v2){.x = -10, .y = -5};
	collider->colliders[PCT_DIVE].props.rectangle.size.x = 22;
	collider->colliders[PCT_DIVE].props.rectangle.size.y = 10;
    }
    else
    {
	player_state_machine->current_state = PS_NORMAL;

	prepare_gfx(&entity->player, game_state);

	collider->colliders =
	    memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(struct Collider));
	collider->num_colliders = 1;
    }

    collider->colliders[PCT_PLAYER].shape = CS_CIRCLE;
    collider->colliders[PCT_PLAYER].props.circle.radius =
	game_state->constants.player_control_radius;

    entities_player_state_machine_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "player created");

    return entity;
}

void entities_player_apply_force(entity_id_t player_entity_id, Vector2 force_to_apply,
				 struct game_state *game_state)
{
    struct ComponentPhysics *physics =
	&entities_entity_get(game_state->entity_repository, player_entity_id)->physics;

    physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

    // Calculate velocity
    physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, game_state->dt));
    if (Vector2Length(physics->velocity) > physics->max_velocity)
    {
	physics->velocity =
	    Vector2Scale(Vector2Normalize(physics->velocity), physics->max_velocity);
    }
}

void entities_player_apply_velocity(entity_id_t player_entity_id, struct game_state *game_state)
{
    struct Entity *entity = entities_entity_get(game_state->entity_repository, player_entity_id);
    struct ComponentPhysics *physics = &entity->physics;
    struct ComponentTransform *transform = &entity->transform;
    transform->position =
	Vector2Add(transform->position, Vector2Scale(physics->velocity, game_state->dt));
}

void entities_player_calculate_running_frame(entity_id_t player_entity_id, v2 direction,
					     f32 velocity_threshold, struct game_state *game_state)
{
    struct Entity *entity = entities_entity_get(game_state->entity_repository, player_entity_id);
    struct ComponentPhysics *physics = &entity->physics;
    struct ComponentPlayer *player = &entity->player;
    struct ComponentSprite *sprite = &entity->sprite;

    if (Vector2Length(physics->velocity) > velocity_threshold)
    {
	enum Direction pd = entities_common_calculate_direction(direction);
	struct AssetsAnimation *animation =
	    assets_animation_get_by_id(game_state->animation_cache, player->run_animations[pd]);
	f32 max_player_velocity = game_state->constants.player_run_max_velocity;
	if ((game_state->time - player->last_frame_update_time) >
	    (animation->delay * (2 - Vector2Length(physics->velocity) / max_player_velocity)))
	{
	    player->last_frame_update_time = game_state->time;
	    u16 new_frame_id = (player->last_running_frame_idx + 1) % animation->frames_count;
	    sprite->frame_id = animation->frames[new_frame_id];
	    player->last_running_frame_idx = new_frame_id;
	}
	player->last_direction = pd;
    }
    else
    {
	sprite->frame_id = player->stand_frames[player->last_direction];
    }
}

void entities_player_state_machine_on_enter(struct Entity *entity, struct game_state *game_state)
{
    struct ComponentPlayer *player = &entity->player;
    struct StateMachinePlayer *state_machine = player->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->header.name, "entering state: %s",
		   StateMachinePlayerStateString[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(union StateMachinePlayerStateData));

    switch (state_machine->current_state)
    {
    case PS_NONE:
	break;
    case PS_NORMAL:
    {
    }
    break;
    case PS_GOALIE:
    {
    }
    break;
    case PS_COUNT:
	break;
    }
}

void entities_player_state_machine_on_exit(struct Entity *entity, struct game_state *game_state)
{
    struct StateMachinePlayer *state_machine = entity->player.state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->header.name, "exiting state: %s",
		   StateMachinePlayerStateString[state_machine->previous_state]);

    switch (state_machine->previous_state)
    {
    case PS_NONE:
	break;
    case PS_NORMAL:
    {
	break;
    }
    case PS_GOALIE:
    {
    }
    break;
    case PS_COUNT:
	break;
    }
}

void entities_goalie_state_machine_on_enter(struct Entity *entity, struct game_state *game_state)
{
    struct StateMachineGoalie *state_machine = entity->goalie.state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->header.name, "entering state: %s",
		   StateMachineGoalieStateString[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(StateMachineGoalieStateData));

    switch (state_machine->current_state)
    {
    case GS_NONE:
	break;
    case GS_PROTECT_GOAL:
    {
	entity->physics.max_velocity = game_state->constants.player_run_max_velocity * 1.25f;
	break;
    }
    case GS_DIVE:
    {
	entity->physics.max_velocity =
	    entity->goalie.dive_speed * game_state->constants.goalie_dive_max_velocity;
	state_machine->state_data.dive.animation_start_time = game_state->time;
	break;
    }
    case GS_HOLD_BALL:
    {
	entity->physics.velocity = Vector2Zero();
	entity->physics.accel = Vector2Zero();
	entity->goalie.state_machine->state_data.hold_ball.hold_start_time = game_state->time;
	entity->sprite.frame_id = entity->goalie.save_frames[SS_UP][ST_WITH_BALL];
	break;
    }
    case GS_COUNT:
	break;
    }
}

void entities_goalie_state_machine_on_exit(struct Entity *entity, struct game_state *game_state)
{
    struct StateMachineGoalie *state_machine = entity->goalie.state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES, entity->header.name, "exiting state: %s",
		   StateMachineGoalieStateString[state_machine->previous_state]);

    switch (state_machine->previous_state)
    {
    case GS_NONE:
	break;
    case GS_PROTECT_GOAL:
    {
	break;
    }
    case GS_DIVE:
    {
	entity->physics.max_velocity = game_state->constants.player_run_max_velocity;
	break;
    }
    case GS_HOLD_BALL:
    {
	break;
    }
    case GS_COUNT:
	break;
    }
}

static void prepare_gfx(struct ComponentPlayer *player, struct game_state *game_state)
{
    // Stand frames
    player->stand_frames[D_DOWN] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_0")->id;
    player->stand_frames[D_UP] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_0")->id;
    player->stand_frames[D_LEFT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_left_0")->id;
    player->stand_frames[D_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_right_0")->id;
    player->stand_frames[D_DOWN_LEFT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_left_0")->id;
    player->stand_frames[D_DOWN_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_front_right_0")->id;
    player->stand_frames[D_UP_LEFT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_left_0")->id;
    player->stand_frames[D_UP_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "player_back_right_0")->id;

    // Running animations
    player->run_animations[D_DOWN] =
	entities_common_animation_id_from_name("player", "run_front", game_state);
    player->run_animations[D_UP] =
	entities_common_animation_id_from_name("player", "run_back", game_state);
    player->run_animations[D_LEFT] =
	entities_common_animation_id_from_name("player", "run_left", game_state);
    player->run_animations[D_RIGHT] =
	entities_common_animation_id_from_name("player", "run_right", game_state);
    player->run_animations[D_DOWN_LEFT] =
	entities_common_animation_id_from_name("player", "run_front_left", game_state);
    player->run_animations[D_DOWN_RIGHT] =
	entities_common_animation_id_from_name("player", "run_front_right", game_state);
    player->run_animations[D_UP_LEFT] =
	entities_common_animation_id_from_name("player", "run_back_left", game_state);
    player->run_animations[D_UP_RIGHT] =
	entities_common_animation_id_from_name("player", "run_back_right", game_state);
}

static void prepare_goalie_gfx(struct ComponentPlayer *player, struct ComponentGoalie *goalie,
			       struct game_state *game_state)
{
    // Stand frames
    player->stand_frames[D_DOWN] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_0")->id;
    player->stand_frames[D_UP] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_0")->id;
    player->stand_frames[D_LEFT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_left_0")->id;
    player->stand_frames[D_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_right_0")->id;
    player->stand_frames[D_DOWN_LEFT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_left_0")
	    ->id;
    player->stand_frames[D_DOWN_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_right_0")
	    ->id;
    player->stand_frames[D_UP_LEFT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_left_0")
	    ->id;
    player->stand_frames[D_UP_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_right_0")
	    ->id;

    // Running animations
    player->run_animations[D_DOWN] =
	entities_common_animation_id_from_name("goalkeeper", "run_front", game_state);
    player->run_animations[D_UP] =
	entities_common_animation_id_from_name("goalkeeper", "run_back", game_state);
    player->run_animations[D_LEFT] =
	entities_common_animation_id_from_name("goalkeeper", "run_left", game_state);
    player->run_animations[D_RIGHT] =
	entities_common_animation_id_from_name("goalkeeper", "run_right", game_state);
    player->run_animations[D_DOWN_LEFT] =
	entities_common_animation_id_from_name("goalkeeper", "run_front_left", game_state);
    player->run_animations[D_DOWN_RIGHT] =
	entities_common_animation_id_from_name("goalkeeper", "run_front_right", game_state);
    player->run_animations[D_UP_LEFT] =
	entities_common_animation_id_from_name("goalkeeper", "run_back_left", game_state);
    player->run_animations[D_UP_RIGHT] =
	entities_common_animation_id_from_name("goalkeeper", "run_back_right", game_state);

    // Save frames
    goalie->save_frames[SS_UP][ST_WITHOUT_BALL] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_0")->id;
    goalie->save_frames[SS_UP][ST_WITH_BALL] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_front_ball_0")
	    ->id;
    goalie->save_frames[SS_DOWN][ST_WITHOUT_BALL] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_0")->id;
    goalie->save_frames[SS_DOWN][ST_WITH_BALL] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache, "goalkeeper_back_ball_0")
	    ->id;

    // Dash frames
    goalie->dash_frames[SS_UP][SD_LEFT][ST_WITHOUT_BALL] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
					"goalkeeper_dash_front_left_0")
	    ->id;
    goalie->dash_frames[SS_UP][SD_LEFT][ST_WITH_BALL] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
					"goalkeeper_dash_front_left_1")
	    ->id;
    goalie->dash_frames[SS_UP][SD_RIGHT][ST_WITHOUT_BALL] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
					"goalkeeper_dash_front_right_0")
	    ->id;
    goalie->dash_frames[SS_UP][SD_RIGHT][ST_WITH_BALL] =
	assets_sprite_frame_get_by_name(game_state->sprite_frame_cache,
					"goalkeeper_dash_front_right_1")
	    ->id;

    // Diving animations
    goalie->dive_animations_start[SS_UP][SD_LEFT][SH_HIGH] = entities_common_animation_id_from_name(
	"goalkeeper", "dive_front_high_left_start", game_state);
    goalie->dive_animations_start[SS_UP][SD_RIGHT][SH_HIGH] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_high_right_start",
					       game_state);
    goalie->dive_animations_start[SS_UP][SD_LEFT][SH_LOW] = entities_common_animation_id_from_name(
	"goalkeeper", "dive_front_low_left_start", game_state);
    goalie->dive_animations_start[SS_UP][SD_RIGHT][SH_LOW] = entities_common_animation_id_from_name(
	"goalkeeper", "dive_front_low_right_start", game_state);

    goalie->dive_animations_end[SS_UP][SD_LEFT][SH_HIGH][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_high_left_end",
					       game_state);
    goalie->dive_animations_end[SS_UP][SD_RIGHT][SH_HIGH][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_high_right_end",
					       game_state);
    goalie->dive_animations_end[SS_UP][SD_LEFT][SH_LOW][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_low_left_end", game_state);
    goalie->dive_animations_end[SS_UP][SD_RIGHT][SH_LOW][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_low_right_end",
					       game_state);
    goalie->dive_animations_end[SS_UP][SD_LEFT][SH_HIGH][ST_WITH_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_high_left_ball_end",
					       game_state);
    goalie->dive_animations_end[SS_UP][SD_RIGHT][SH_HIGH][ST_WITH_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_high_right_ball_end",
					       game_state);
    goalie->dive_animations_end[SS_UP][SD_LEFT][SH_LOW][ST_WITH_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_low_left_ball_end",
					       game_state);
    goalie->dive_animations_end[SS_UP][SD_RIGHT][SH_LOW][ST_WITH_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_low_right_ball_end",
					       game_state);
    // goalie->dive_animations[SS_DOWN][SD_LEFT] =
    //     entities_common_animation_id_from_name("goalkeeper", "save_back_left", game_state);
    // goalie->dive_animations[SS_DOWN][SD_RIGHT] =
    //     entities_common_animation_id_from_name("goalkeeper", "save_back_right", game_state);
}

Rectangle entities_player_calculate_dive_collision_rectangle(v2 pos, struct Collider *dive_collider)
{
    return (Rectangle){.x = pos.x + dive_collider->props.rectangle.offset.x,
		       .y = pos.y + dive_collider->props.rectangle.offset.y,
		       .width = dive_collider->props.rectangle.size.x,
		       .height = dive_collider->props.rectangle.size.y};
}

bool entities_player_is_diving_goalie_colliding_with_ball(struct Entity *goalie_entity,
							  v3 ball_position, f32 ball_radius,
							  f32 max_height)
{
    struct Collider *dive_collider = &goalie_entity->collider.colliders[PCT_DIVE];

    b32 is_colliding_with_ball =
	CheckCollisionCircleRec(V3_TO_V2(ball_position), ball_radius,
				entities_player_calculate_dive_collision_rectangle(
				    goalie_entity->transform.position, dive_collider)) &&
	ball_position.y < max_height;

    return is_colliding_with_ball;
}

bool entities_player_is_standing_goalie_colliding_with_ball(struct Entity *goalie_entity,
							    v3 ball_position, f32 ball_radius,
							    f32 max_height)
{
    struct Collider *goalie_collider = &goalie_entity->collider.colliders[PCT_GOALIE];
    b32 is_colliding_with_ball =
	CheckCollisionCircles(
	    V3_TO_V2(ball_position), ball_radius,
	    Vector2Add(goalie_entity->transform.position, goalie_collider->props.circle.offset),
	    goalie_collider->props.circle.radius) &&
	ball_position.y < max_height;

    return is_colliding_with_ball;
}

bool entities_player_should_dive(struct Entity *goalie_entity, struct Entity *ball_entity,
				 enum SaveDirection *dive_direction, enum SaveHeight *dive_height,
				 f32 *dive_speed, f32 *dive_distance, struct game_state *game_state)
{
    bool should_dive = false;

    const int steps_per_lookahead =
	(game_state->vars.slow_motion ? SLOW_MOTION_LOOKAHEAD_MULTIPLIER : 1) *
	game_state->constants.ball_lookahead_dts;
    // We want a lookahead of 150ms for our goalie
    const int lookaheads =
	floor(Clamp((GOALIE_LOOKAHEAD_MS / game_state->constants.fixed_timestep) /
			game_state->constants.ball_lookahead_dts,
		    1, game_state->constants.ball_lookaheads));
    for (u16 i = 0; i < lookaheads; ++i)
    {
	v3 pos_to_check = ball_entity->ball.lookahead_positions[i];
	b32 is_colliding_with_ball = entities_player_is_standing_goalie_colliding_with_ball(
	    goalie_entity, pos_to_check, ball_entity->collider.colliders[0].props.circle.radius,
	    game_state->constants.crossbar_height);
	if (is_colliding_with_ball)
	{
	    // Ball is coming our direction, let's wait
	    break;
	}
	// Check the ball is still savable
	f32 x_diff = pos_to_check.x - goalie_entity->transform.position.x;
	f32 y_diff = pos_to_check.z - goalie_entity->transform.position.y;
	f32 distance_to_cover = fabsf(x_diff);
	if (HAS_TIME_PASSED(game_state->time, ball_entity->ball.last_kick_time,
			    game_state->constants.goalie_dive_reaction_time) &&
	    y_diff > 0 && y_diff < 5 && distance_to_cover < 60 &&
	    pos_to_check.y < game_state->constants.crossbar_height)
	{
	    f32 time_to_unsaveable = (i + 1) * game_state->dt * steps_per_lookahead;
	    f32 max_distance_we_can_cover_in_available_time =
		game_state->constants.goalie_dive_max_velocity * time_to_unsaveable / 1.0f;
	    *dive_direction = x_diff > 0 ? SD_RIGHT : SD_LEFT;
	    *dive_height = pos_to_check.y < GOALIE_DIVE_HEIGHT_THRESHOLD ? SH_LOW : SH_HIGH;
	    *dive_speed =
		Clamp(distance_to_cover / max_distance_we_can_cover_in_available_time, 0, 1);
	    *dive_distance = distance_to_cover;
	    should_dive = true;
	    GLOGF_DEBUG_CTX(LS_GAME, goalie_entity->header.name,
			    "goalie has %.3fs to save a ball that will be %.2fpx away: "
			    "dive speed %.2fpx/s",
			    time_to_unsaveable, distance_to_cover,
			    goalie_entity->goalie.dive_speed *
				game_state->constants.goalie_dive_max_velocity);
	    break;
	}
    }
    return should_dive;
}
