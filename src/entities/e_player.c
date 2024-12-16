#include "components.h"
#include "entities.h"
#include "football.h"
#include "log.h"
#include "math_ext.h"
#include <assert.h>
#include <raylib.h>
#include <string.h>
#include "game.h"
#include "types.h"

implement_state_machine_functions(player);
implement_state_machine_functions(goalie);

static void prepare_gfx(struct component_player *player, struct game_state *game_state);
static void prepare_goalie_gfx(struct component_player *player,
			       struct component_goalie *goalie,
			       struct game_state *game_state);
static inline rect_t calculate_dive_collision_rectangle(v2 pos, struct collider *dive_collider);
color_t *player_custom_palette(entity_id_t entity_id, struct game_state *game_state);
void player_custom_begin_draw(entity_id_t entity_id, struct game_state *game_state);
void player_custom_after_draw(entity_id_t entity_id, struct game_state *game_state);

struct entity *entities_player_create(struct player_config config, struct game_state *game_state)
{
    struct entity *entity =
	entities_entity_init(game_state->entity_repository, ET_PLAYER, "player_%02d");

    // Initialise transform
    struct component_transform *transform = &entity->transform;
    transform->header.enabled = true;
    transform->position = config.position;
    transform->rotation = 0;
    transform->scale = (v2){.x = 1, .y = 1};

    // Initialise physics
    struct component_physics *physics = &entity->physics;
    physics->header.enabled = true;
    physics->mass = 75.0f;
    physics->velocity = (Vector2){0};
    physics->accel = (Vector2){0};
    physics->max_velocity = game_state->constants.player_control_max_velocity;

    // Initialise collider
    struct component_collider *collider = &entity->collider;
    collider->header.enabled = true;

    // Initialise sprite
    char *player_frame_name = "player_front_0";
    struct assets_sprite_frame *sprite_frame =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, player_frame_name);
    assert(sprite_frame != NULL);
    struct component_sprite *sprite = &entity->sprite;
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
    struct component_player *player = &entity->player;
    player->header.enabled = true;
    player->team_side = config.team_side;
    player->team_formation_position_idx = config.team_formation_position_idx;
    // Sets initial max velocity to half of max run velocity
    player->current_max_velocity = game_state->constants.player_run_max_velocity * 0.5f;
    struct state_machine_player *player_state_machine =
	memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(*player_state_machine));
    player_state_machine->owner_entity = entity->header.id;
    player_state_machine->previous_state = PS_NONE;
    player->state_machine = player_state_machine;

    // Initialise action
    struct component_action *action = &entity->action;
    action->header.enabled = true;

    // Initialise player controls
    struct component_player_controls *player_controls = &entity->player_controls;
    player_controls->header.enabled = true;

    if (config.is_goalie) {
	// Initialise goalie
	struct component_goalie *goalie = &entity->goalie;
	goalie->header.enabled = true;
	struct state_machine_goalie *state_machine =
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
						sizeof(struct collider) * collider->num_colliders);
	collider->colliders[PCT_GOALIE].shape = CS_CIRCLE;
	collider->colliders[PCT_GOALIE].props.circle.offset = (v2){.y = 0};
	collider->colliders[PCT_GOALIE].props.circle.radius = 6;

	collider->colliders[PCT_DIVE].shape = CS_RECTANGLE;
	collider->colliders[PCT_DIVE].props.rectangle.offset = (v2){.x = -10, .y = -5};
	collider->colliders[PCT_DIVE].props.rectangle.size.x = 22;
	collider->colliders[PCT_DIVE].props.rectangle.size.y = 10;
    } else {
	player_state_machine->current_state = PS_NORMAL;

	prepare_gfx(&entity->player, game_state);

	sprite->before_draw = &player_custom_begin_draw;
	sprite->after_draw = &player_custom_after_draw;

	collider->colliders =
	    memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(struct collider));
	collider->num_colliders = 1;
    }

    collider->colliders[PCT_PLAYER].shape = CS_CIRCLE;
    collider->colliders[PCT_PLAYER].props.circle.radius =
	game_state->constants.player_control_radius;

    entities_player_state_machine_on_enter(entity, game_state);

    GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "player created");

    return entity;
}

void entities_player_apply_force(entity_id_t player_entity_id,
				 Vector2 force_to_apply,
				 struct game_state *game_state)
{
    struct component_physics *physics =
	&entities_entity_get(game_state->entity_repository, player_entity_id)->physics;

    physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

    // Calculate velocity
    physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, game_state->dt));
    if (Vector2Length(physics->velocity) > physics->max_velocity) {
	physics->velocity =
	    Vector2Scale(Vector2Normalize(physics->velocity), physics->max_velocity);
    }
}

void entities_player_apply_velocity(entity_id_t player_entity_id, struct game_state *game_state)
{
    struct entity *entity = entities_entity_get(game_state->entity_repository, player_entity_id);
    struct component_physics *physics = &entity->physics;
    struct component_transform *transform = &entity->transform;
    transform->position =
	Vector2Add(transform->position, Vector2Scale(physics->velocity, game_state->dt));
}

void entities_player_calculate_running_frame(entity_id_t player_entity_id,
					     v2 direction,
					     f32 velocity_threshold,
					     struct game_state *game_state)
{
    struct entity *entity = entities_entity_get(game_state->entity_repository, player_entity_id);
    struct component_physics *physics = &entity->physics;
    struct component_player *player = &entity->player;
    struct component_sprite *sprite = &entity->sprite;

    f32 player_speed = Vector2Length(physics->velocity);
    if (Vector2Length(physics->velocity) > velocity_threshold) {
	enum direction pd = entities_common_calculate_direction(direction);
	struct assets_animation *animation = assets_animation_get_by_id(
	    game_state->assets.animation_cache, player->run_animations[pd]);

	f32 max_player_velocity = game_state->constants.player_run_max_velocity;
	f32 player_speed_ratio = player_speed / max_player_velocity;

	if ((game_state->time - player->last_frame_update_time) >
	    (animation->delay * (2.5f - player_speed_ratio))) {
	    player->last_frame_update_time = game_state->time;
	    u16 new_frame_id = (player->last_running_frame_idx + 1) % animation->frames_count;
	    sprite->frame_id = animation->frames[new_frame_id];
	    player->last_running_frame_idx = new_frame_id;
	}
	player->last_direction = pd;
    } else {
	sprite->frame_id = player->stand_frames[player->last_direction];
    }
}

void entities_player_state_machine_on_enter(struct entity *entity, struct game_state *game_state)
{
    UNUSED(game_state);

    struct component_player *player = &entity->player;
    struct state_machine_player *state_machine = player->state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES,
		   entity->header.name,
		   "entering state: %s",
		   state_machine_player_state_string[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(union state_machine_player_state_data));

    switch (state_machine->current_state) {
    case PS_NONE:
    case PS_NORMAL:
    case PS_GOALIE:
    case PS_THROW_IN:
    case PS_HEADER:
    case PS_SLIDING_TACKLE:
	entity->sprite.frame_id = player->tackle_frames[player->last_direction];
	entity->player.state_machine->state_data.tackle.start_time = game_state->time;
    case PS_FALL:
    case PS_COUNT:
	break;
    }
}

void entities_player_state_machine_on_exit(struct entity *entity, struct game_state *game_state)
{
    UNUSED(game_state);

    struct state_machine_player *state_machine = entity->player.state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES,
		   entity->header.name,
		   "exiting state: %s",
		   state_machine_player_state_string[state_machine->previous_state]);

    switch (state_machine->previous_state) {
    case PS_NONE:
    case PS_NORMAL:
    case PS_GOALIE:
    case PS_THROW_IN:
    case PS_HEADER:
    case PS_SLIDING_TACKLE:
    case PS_FALL:
    case PS_COUNT:
	break;
    }
}

void entities_goalie_state_machine_on_enter(struct entity *entity, struct game_state *game_state)
{
    struct state_machine_goalie *state_machine = entity->goalie.state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES,
		   entity->header.name,
		   "entering state: %s",
		   state_machine_goalie_state_string[state_machine->current_state]);

    // Reset state data
    memset(&state_machine->state_data, 0, sizeof(union state_machine_goalie_state_data));

    switch (state_machine->current_state) {
    case GS_NONE:
	break;
    case GS_PROTECT_GOAL: {
	entity->physics.max_velocity = game_state->constants.player_run_max_velocity * 1.25f;
	break;
    }
    case GS_DIVE: {
	entity->physics.max_velocity =
	    entity->goalie.dive_speed * game_state->constants.goalie_dive_max_velocity;
	state_machine->state_data.dive.animation_start_time = game_state->time;
	break;
    }
    case GS_HOLD_BALL: {
	entity->physics.velocity = Vector2Zero();
	entity->physics.accel = Vector2Zero();
	entity->goalie.state_machine->state_data.hold_ball.hold_start_time = game_state->time;
	entity->sprite.frame_id =
	    entity->goalie.save_frames[game_state->vars.teams[entity->player.team_side].pitch_side]
				      [ST_WITH_BALL];
	break;
    }
    case GS_JUMP: {
	entity->physics.velocity = Vector2Zero();
	entity->physics.accel = Vector2Zero();
	state_machine->state_data.jump.animation_start_time = game_state->time;
	break;
    }
    case GS_COUNT:
	break;
    }
}

void entities_goalie_state_machine_on_exit(struct entity *entity, struct game_state *game_state)
{
    struct state_machine_goalie *state_machine = entity->goalie.state_machine;

    GLOGF_INFO_CTX(LS_ENTITIES,
		   entity->header.name,
		   "exiting state: %s",
		   state_machine_goalie_state_string[state_machine->previous_state]);

    switch (state_machine->previous_state) {
    case GS_NONE:
	break;
    case GS_PROTECT_GOAL: {
	break;
    }
    case GS_DIVE: {
	entity->physics.max_velocity = game_state->constants.player_run_max_velocity;
	break;
    }
    case GS_HOLD_BALL: {
	break;
    }
    case GS_JUMP: {
	break;
    }
    case GS_COUNT:
	break;
    }
}

b32 entities_player_is_colliding_with_ball(struct entity *entity,
					   enum player_collider_type collider_type,
					   v3 ball_position,
					   f32 ball_radius,
					   f32 max_height)
{
    const v2 player_position = entity->transform.position;
    const v2 ball_pos = V3_TO_V2(ball_position);
    b32 check_collision = false;
    struct collider *collider = &entity->collider.colliders[collider_type];

    switch (collider_type) {
    case PCT_DIVE:
	check_collision = CheckCollisionCircleRec(
	    ball_pos, ball_radius, calculate_dive_collision_rectangle(player_position, collider));
	break;
    case PCT_PLAYER:
    case PCT_GOALIE:
	check_collision =
	    CheckCollisionCircles(ball_pos,
				  ball_radius,
				  Vector2Add(player_position, collider->props.circle.offset),
				  collider->props.circle.radius);
	break;
    case PCT_COUNT:
	break;
    }

    return check_collision && ball_position.y < max_height;
}

b32 entities_player_should_dive(struct entity *goalie_entity,
				struct entity *ball_entity,
				enum save_direction *dive_direction,
				enum save_height *dive_height,
				f32 *dive_speed,
				f32 *dive_distance,
				struct game_state *game_state)
{
    b32 should_dive = false;
    const f32 ball_radius = ball_entity->collider.colliders[0].props.circle.radius;

    const u32 steps_per_lookahead =
	(game_state->vars.slow_motion ? SLOW_MOTION_LOOKAHEAD_MULTIPLIER : 1) *
	game_state->constants.ball_lookahead_dts;
    // We want a lookahead of 150ms for our goalie
    const int lookaheads =
	floor(Clamp((GOALIE_LOOKAHEAD_MS / game_state->constants.fixed_timestep) /
			game_state->constants.ball_lookahead_dts,
		    1,
		    game_state->constants.ball_lookaheads));
    for (u16 i = 0; i < lookaheads; ++i) {
	v3 pos_to_check = ball_entity->ball.lookahead_positions[i];

	const b32 is_colliding_with_ball =
	    entities_player_is_colliding_with_ball(goalie_entity,
						   PCT_GOALIE,
						   pos_to_check,
						   ball_radius,
						   game_state->constants.crossbar_height);

	if (is_colliding_with_ball) {
	    // Ball is coming our direction, let's wait
	    break;
	}
	// Check the ball is still savable
	f32 x_diff = pos_to_check.x - goalie_entity->transform.position.x;
	f32 y_diff = pos_to_check.z - goalie_entity->transform.position.y;
	f32 distance_to_cover = fabsf(x_diff);
	if (HAS_TIME_PASSED(game_state->time,
			    ball_entity->ball.last_kick_time,
			    game_state->constants.goalie_dive_reaction_time) &&
	    y_diff > 0 && y_diff < 5 && distance_to_cover < 60 &&
	    pos_to_check.y < game_state->constants.crossbar_height) {
	    f32 time_to_unsaveable = (i + 1) * game_state->dt * steps_per_lookahead;
	    f32 max_distance_we_can_cover_in_available_time =
		game_state->constants.goalie_dive_max_velocity * time_to_unsaveable / 1.0f;
	    *dive_direction = x_diff > 0 ? SD_RIGHT : SD_LEFT;
	    *dive_height = pos_to_check.y < GOALIE_DIVE_HEIGHT_THRESHOLD ? SH_LOW : SH_HIGH;
	    *dive_speed =
		Clamp(distance_to_cover / max_distance_we_can_cover_in_available_time, 0, 1);
	    *dive_distance = distance_to_cover;
	    should_dive = true;
	    GLOGF_DEBUG_CTX(LS_GAME,
			    goalie_entity->header.name,
			    "goalie has %.3fs to save a ball that will be %.2fpx away: "
			    "dive speed %.2fpx/s",
			    time_to_unsaveable,
			    distance_to_cover,
			    goalie_entity->goalie.dive_speed *
				game_state->constants.goalie_dive_max_velocity);
	    break;
	}
    }
    return should_dive;
}

static void prepare_gfx(struct component_player *player, struct game_state *game_state)
{
    // Stand frames
    player->stand_frames[D_DOWN] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_front_0")->id;
    player->stand_frames[D_UP] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_back_0")->id;
    player->stand_frames[D_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_left_0")->id;
    player->stand_frames[D_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_right_0")->id;
    player->stand_frames[D_DOWN_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_front_left_0")
	    ->id;
    player->stand_frames[D_DOWN_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_front_right_0")
	    ->id;
    player->stand_frames[D_UP_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_back_left_0")
	    ->id;
    player->stand_frames[D_UP_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_back_right_0")
	    ->id;

    // Tackle frames
    player->tackle_frames[D_DOWN] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_tackle_front_0")
	    ->id;
    player->tackle_frames[D_UP] = assets_sprite_frame_get_by_name(
				      game_state->assets.sprite_frame_cache, "player_tackle_back_0")
				      ->id;
    player->tackle_frames[D_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_tackle_left_0")
	    ->id;
    player->tackle_frames[D_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_tackle_right_0")
	    ->id;
    player->tackle_frames[D_DOWN_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_tackle_front_left_0")
	    ->id;
    player->tackle_frames[D_DOWN_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_tackle_front_right_0")
	    ->id;
    player->tackle_frames[D_UP_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_tackle_back_left_0")
	    ->id;
    player->tackle_frames[D_UP_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"player_tackle_back_right_0")
	    ->id;

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

    // Fall animations
    player->fall_animations[D_DOWN] =
	entities_common_animation_id_from_name("player", "fall_front", game_state);
    player->fall_animations[D_UP] =
	entities_common_animation_id_from_name("player", "fall_back", game_state);
    player->fall_animations[D_LEFT] =
	entities_common_animation_id_from_name("player", "fall_left", game_state);
    player->fall_animations[D_RIGHT] =
	entities_common_animation_id_from_name("player", "fall_right", game_state);
    player->fall_animations[D_DOWN_LEFT] =
	entities_common_animation_id_from_name("player", "fall_front_left", game_state);
    player->fall_animations[D_DOWN_RIGHT] =
	entities_common_animation_id_from_name("player", "fall_front_right", game_state);
    player->fall_animations[D_UP_LEFT] =
	entities_common_animation_id_from_name("player", "fall_back_left", game_state);
    player->fall_animations[D_UP_RIGHT] =
	entities_common_animation_id_from_name("player", "fall_back_right", game_state);
}

static void prepare_goalie_gfx(struct component_player *player,
			       struct component_goalie *goalie,
			       struct game_state *game_state)
{
    // Stand frames
    player->stand_frames[D_DOWN] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_front_0")
	    ->id;
    player->stand_frames[D_UP] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_back_0")
	    ->id;
    player->stand_frames[D_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_left_0")
	    ->id;
    player->stand_frames[D_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_right_0")
	    ->id;
    player->stand_frames[D_DOWN_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_front_left_0")
	    ->id;
    player->stand_frames[D_DOWN_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_front_right_0")
	    ->id;
    player->stand_frames[D_UP_LEFT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_back_left_0")
	    ->id;
    player->stand_frames[D_UP_RIGHT] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_back_right_0")
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
    goalie->save_frames[PSI_NORTH][ST_WITHOUT_BALL] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_front_0")
	    ->id;
    goalie->save_frames[PSI_NORTH][ST_WITH_BALL] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_front_ball_0")
	    ->id;
    goalie->save_frames[PSI_SOUTH][ST_WITHOUT_BALL] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_back_0")
	    ->id;
    goalie->save_frames[PSI_SOUTH][ST_WITH_BALL] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_back_ball_0")
	    ->id;

    // Dash frames
    goalie->dash_frames[PSI_NORTH][SD_LEFT][ST_WITHOUT_BALL] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_dash_front_left_0")
	    ->id;
    goalie->dash_frames[PSI_NORTH][SD_LEFT][ST_WITH_BALL] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_dash_front_left_1")
	    ->id;
    goalie->dash_frames[PSI_NORTH][SD_RIGHT][ST_WITHOUT_BALL] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_dash_front_right_0")
	    ->id;
    goalie->dash_frames[PSI_NORTH][SD_RIGHT][ST_WITH_BALL] =
	assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
					"goalkeeper_dash_front_right_1")
	    ->id;

    // Diving animations
    goalie->dive_animations_start[PSI_NORTH][SD_LEFT][SH_HIGH] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_high_left_start", game_state);
    goalie->dive_animations_start[PSI_NORTH][SD_RIGHT][SH_HIGH] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_high_right_start", game_state);
    goalie->dive_animations_start[PSI_NORTH][SD_LEFT][SH_LOW] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_low_left_start", game_state);
    goalie->dive_animations_start[PSI_NORTH][SD_RIGHT][SH_LOW] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_low_right_start", game_state);

    goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_HIGH][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_high_left_end", game_state);
    goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_HIGH][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_high_right_end", game_state);
    goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_LOW][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_front_low_left_end", game_state);
    goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_LOW][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_low_right_end", game_state);
    goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_HIGH][ST_WITH_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_high_left_ball_end", game_state);
    goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_HIGH][ST_WITH_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_high_right_ball_end", game_state);
    goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_LOW][ST_WITH_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_low_left_ball_end", game_state);
    goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_LOW][ST_WITH_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_front_low_right_ball_end", game_state);

    goalie->dive_animations_start[PSI_SOUTH][SD_LEFT][SH_HIGH] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_high_left_start", game_state);
    goalie->dive_animations_start[PSI_SOUTH][SD_RIGHT][SH_HIGH] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_high_right_start", game_state);
    goalie->dive_animations_start[PSI_SOUTH][SD_LEFT][SH_LOW] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_low_left_start", game_state);
    goalie->dive_animations_start[PSI_SOUTH][SD_RIGHT][SH_LOW] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_low_right_start", game_state);

    goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_HIGH][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_back_high_left_end", game_state);
    goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_HIGH][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_high_right_end", game_state);
    goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_LOW][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_back_low_left_end", game_state);
    goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_LOW][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "dive_back_low_right_end", game_state);
    goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_HIGH][ST_WITH_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_high_left_ball_end", game_state);
    goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_HIGH][ST_WITH_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_high_right_ball_end", game_state);
    goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_LOW][ST_WITH_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_low_left_ball_end", game_state);
    goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_LOW][ST_WITH_BALL] =
	entities_common_animation_id_from_name(
	    "goalkeeper", "dive_back_low_right_ball_end", game_state);

    // Jump animations
    goalie->jump_animations_start[PSI_NORTH] =
	entities_common_animation_id_from_name("goalkeeper", "front_jump_parry_start", game_state);
    goalie->jump_animations_start[PSI_SOUTH] =
	entities_common_animation_id_from_name("goalkeeper", "back_jump_parry_start", game_state);
    goalie->jump_animations_end[PSI_NORTH][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "front_jump_parry_end", game_state);
    goalie->jump_animations_end[PSI_NORTH][ST_WITH_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "front_jump_catch_end", game_state);
    goalie->jump_animations_end[PSI_SOUTH][ST_WITHOUT_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "back_jump_parry_end", game_state);
    goalie->jump_animations_end[PSI_SOUTH][ST_WITH_BALL] =
	entities_common_animation_id_from_name("goalkeeper", "back_jump_catch_end", game_state);
}

static inline rect_t calculate_dive_collision_rectangle(v2 pos, struct collider *dive_collider)
{
    return (rect_t){.x = pos.x + dive_collider->props.rectangle.offset.x,
		    .y = pos.y + dive_collider->props.rectangle.offset.y,
		    .width = dive_collider->props.rectangle.size.x,
		    .height = dive_collider->props.rectangle.size.y};
}

void player_custom_begin_draw(entity_id_t entity_id, struct game_state *game_state)
{
    struct entity *entity = entities_entity_get(game_state->entity_repository, entity_id);

    u32 *palette =
	memory_arena_push(game_state->memory_by_lifetime[MAL_UPDATE], sizeof(u32) * 3 * 10);

    color_t kit_color = entity->player.team_side == TS_HOME ? RED : BLUE;

    palette = (u32[]){
	// Hair
	0,
	0,
	0,
	// Skin
	201,
	147,
	120,
	// Skin shadow
	166,
	111,
	90,
	// Kit
	kit_color.r,
	kit_color.g,
	kit_color.b,
	// Kit shadow
	201,
	147,
	120,
	// Pantaloncini
	255,
	255,
	255,
	// Shoes
	0,
	0,
	0,
    };

    struct assets_shader *assets_shader =
	assets_shader_get_by_name(game_state->assets.shader_cache, "shader_palette");
    u32 location_idx = GetShaderLocation(assets_shader->shader, "palette");
    SetShaderValueV(assets_shader->shader, location_idx, palette, SHADER_UNIFORM_IVEC3, 7);

    BeginShaderMode(assets_shader->shader);
}

void player_custom_after_draw(entity_id_t entity_id, struct game_state *game_state)
{
    UNUSED(entity_id);
    UNUSED(game_state);

    EndShaderMode();
}
