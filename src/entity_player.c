#include "macro.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "football.h"
#include "input.h"
#include "log.h"
#include "math_ext.h"
#include "game.h"
#include "memory.h"
#include "state_machine.h"
#include "types.h"
#include "sprite.h"
#include "physics.h"
#include "collision.h"
#include "gfx.h"
#include "constants.h"
#include <float.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Sprite palette, indexed by the red channel of the sprite's own pixels.
// Shared by every kit:
#define HAIR_COLOR		  ((color_t){0, 0, 0, 255})
#define SKIN_DARK_COLOR	  ((color_t){199, 137, 137, 255})
#define SKIN_LIGHT_COLOR  ((color_t){243, 170, 170, 255})
#define SHOES_COLOR		  ((color_t){0, 0, 0, 255})
#define SHADOW_COLOR	  ((color_t){7, 45, 45, 100})
#define SOCKS_LIGHT_COLOR ((color_t){255, 255, 255, 255})
#define SHORTS_COLOR	  ((color_t){255, 255, 255, 255})
// Index 10 is reached by referee art alone, which nothing draws yet.
#define CARD_COLOR_UNUSED ((color_t){0, 0, 0, 255})
// Per kit:
#define PLAYER_KIT_COLOR_HOME	RED
#define PLAYER_KIT_COLOR_AWAY	BLUE
#define PLAYER_SOCKS_DARK_COLOR ((color_t){208, 208, 208, 255})
#define GOALIE_KIT_LIGHT_COLOR	((color_t){29, 29, 29, 255})
#define GOALIE_KIT_DARK_COLOR	((color_t){0, 0, 0, 255})
#define GOALIE_SOCKS_DARK_COLOR ((color_t){255, 255, 255, 255})
// Index 11 is present in the keeper's sprites only.
#define GOALIE_GLOVES_COLOR ((color_t){9, 64, 110, 255})

const struct header_phase_window header_phase_windows[] = {
	{16, 22, 19}, {19, 25, 22}, {22, 28, 25}, {25, 31, 28}, {28, 34, 31}};

const char *state_machine_player_state_string[PS_COUNT] = {"none",
														   "normal",
														   "throw_in",
														   "header",
														   "tackle",
														   "sliding_tackle",
														   "fall",
														   "tackled_run",
														   "goalie"};

const char *player_dribble_turn_string[PDT_COUNT] = {"FtoB_right",
													 "FtoB_left",
													 "BtoF_right",
													 "BtoF_left",
													 "RtoL_front",
													 "RtoL_back",
													 "LtoR_front",
													 "LtoR_back"};

const char *player_collider_type_to_string[PCT_COUNT] = {"player",
														 "goalie",
														 "dive",
														 "tackle_sensor",
														 "tackle",
														 "sliding_tackle_1",
														 "sliding_tackle_2",
														 "header",
														 "body"};

// Linearly interpolate a value on the 1-5 characteristic scale
static inline f32 char_lerp(u8 value, f32 at_1, f32 at_5)
{
	f32 t = (value - 1) / 4.0f;
	return at_1 + t * (at_5 - at_1);
}

struct player_constants entity_player_derive_constants(const struct player_characteristics *c,
													   struct game_state *game_state)
{
	const struct game_constants *k = &game_state->constants;
	return (struct player_constants){
		.movement_force = k->player_movement_force * char_lerp(c->strength, 0.8f, 1.2f),
		.drag = k->player_movement_drag * char_lerp(c->strength, 1.2f, 0.8f),
		.run_max_velocity = k->player_run_max_velocity * char_lerp(c->speed, 0.85f, 1.15f),
		// Not characteristic-derived yet: pass the base through. This function is the single
		// place to add per-characteristic scaling for these later.
		.control_max_velocity = k->player_control_max_velocity,
		.control_radius = k->player_control_radius,
		.control_height = k->player_control_height,
		.tackle_friction_force = k->player_tackle_friction_force,
		.sliding_tackle_friction_force = k->player_sliding_tackle_friction_force,
		.accel_ramp_up = char_lerp(c->acceleration, 30.0f, 70.0f),
		.decel_ramp_down = char_lerp(c->acceleration, 60.0f, 140.0f),
		.mass = char_lerp(c->weight, 60.0f, 95.0f),
	};
}

void entity_player_recompute_constants(struct entity *player_entity, struct game_state *game_state)
{
	struct component_player *player = &player_entity->player;
	player->constants = entity_player_derive_constants(&player->characteristics, game_state);
	player_entity->physics.mass = player->constants.mass;
}

void entity_player_on_constants_changed(enum event_type type, void *context)
{
	UNUSED(type);
	struct game_state *game_state = context;
	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (entity->header.alive && entity->header.type == ET_PLAYER) {
			entity_player_recompute_constants(entity, game_state);
		}
	}
}

const char *state_machine_goalie_state_string[GS_COUNT] = {
	"none", "protect_goal", "dive", "hold_ball", "jump", "throw"};

const char *state_machine_player_trigger_string[PT_COUNT] = {"none",
															 "head",
															 "tackle",
															 "slide_tackle",
															 "tackled",
															 "knocked_down",
															 "recover",
															 "step_out",
															 "guard_goal",
															 "pick_up"};

const char *state_machine_goalie_trigger_string[GT_COUNT] = {
	"none", "dive", "jump", "catch", "recover", "release", "throw"};

// A keeper holds PS_GOALIE while he is the goalie and PS_NORMAL while he plays with his feet, so
// those two swap both ways. Everything that can happen to a player happens to him in PS_NORMAL.
static const enum state_machine_player_state player_transitions[PS_COUNT][PT_COUNT] = {
	[PS_NORMAL] = {[PT_HEAD] = PS_HEADER,
				   [PT_TACKLE] = PS_TACKLE,
				   [PT_SLIDE_TACKLE] = PS_SLIDING_TACKLE,
				   [PT_TACKLED] = PS_TACKLED_RUN,
				   [PT_KNOCKED_DOWN] = PS_FALL,
				   [PT_GUARD_GOAL] = PS_GOALIE,
				   [PT_PICK_UP] = PS_GOALIE},
	[PS_GOALIE] = {[PT_STEP_OUT] = PS_NORMAL},
	[PS_HEADER] = {[PT_RECOVER] = PS_NORMAL},
	[PS_TACKLE] = {[PT_RECOVER] = PS_NORMAL},
	[PS_SLIDING_TACKLE] = {[PT_RECOVER] = PS_NORMAL},
	[PS_FALL] = {[PT_RECOVER] = PS_NORMAL},
	[PS_TACKLED_RUN] = {[PT_RECOVER] = PS_NORMAL},
};

static const enum state_machine_goalie_state goalie_transitions[GS_COUNT][GT_COUNT] = {
	[GS_PROTECT_GOAL] = {[GT_DIVE] = GS_DIVE, [GT_JUMP] = GS_JUMP, [GT_CATCH] = GS_HOLD_BALL},
	[GS_DIVE] = {[GT_CATCH] = GS_HOLD_BALL, [GT_RECOVER] = GS_PROTECT_GOAL},
	[GS_JUMP] = {[GT_CATCH] = GS_HOLD_BALL, [GT_RECOVER] = GS_PROTECT_GOAL},
	[GS_HOLD_BALL] = {[GT_RELEASE] = GS_PROTECT_GOAL, [GT_THROW] = GS_THROW},
	[GS_THROW] = {[GT_RELEASE] = GS_PROTECT_GOAL},
};

STATE_MACHINE_FORWARD_DECLARE_FUNCTIONS(player, struct entity);
STATE_MACHINE_FORWARD_DECLARE_FUNCTIONS(goalie, struct entity);

STATE_MACHINE_IMPLEMENT_FUNCTIONS(player, struct entity);
STATE_MACHINE_IMPLEMENT_FUNCTIONS(goalie, struct entity);

// How a player would come to meet the ball: the earliest predicted position he gets to before the
// ball does, or, when the ball outruns him everywhere, his quickest chase of it.
struct player_ball_meeting {
	b32 can_intercept;
	fg_time_t time_to_ball;
	f32 distance_to_ball_path;
};

static void player_custom_begin_draw(struct entity *entity, struct game_state *game_state);
static void player_custom_after_draw(struct entity *entity, struct game_state *game_state);
static b32 is_player_selectable(struct entity *player_entity);
static struct player_ball_meeting player_meeting_with_the_ball(struct entity *player_entity,
															   struct entity *ball_entity,
															   struct game_state *game_state);
static f32 goal_side_discount(v2 player_position,
							  v2 ball_position,
							  enum team_side team_side,
							  struct game_state *game_state);
static b32 check_whether_goalie_should_dive(v3 pos_to_check,
											b32 attacking_south,
											f32 time_to_unsaveable,
											struct entity *goalie_entity,
											struct goalie_dive *dive,
											struct game_state *game_state);

struct entity *entity_player_create(struct player_config config, struct game_state *game_state)
{
	struct entity *entity = entity_init(game_state->entity_repository, ET_PLAYER, "player_%02d");

	// Initialise transform
	struct component_transform *transform = &entity->transform;
	transform->header.enabled = true;
	transform->position = config.position;
	transform->rotation = 0;
	transform->scale = (v2){.x = 1, .y = 1};

	// Initialise physics
	struct component_physics *physics = &entity->physics;
	physics->header.enabled = true;
	physics->velocity = (Vector2){0};
	physics->accel = (Vector2){0};
	// physics->mass and max_velocity are set from the player's resolved constants below, once
	// characteristics are known (see entity_player_recompute_constants).

	// Initialise collider
	struct component_collider *collider = &entity->collider;
	collider->header.enabled = true;

	// Initialise sprite
	enum pitch_side pitch_side = game_state->vars.teams[config.team_side].play_context.pitch_side;

	char *player_frame_name = "player_front_0";
	struct asset_sprite_frame *sprite_frame =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, player_frame_name);
	ASSERT(sprite_frame != NULL);
	struct component_sprite *sprite = &entity->sprite;
	sprite->header.enabled = true;
	sprite->frame_id = sprite_frame->id;
	sprite->color = WHITE;
	sprite->flipX = false;
	sprite->flipY = false;
	sprite->pivot_override = (v2){.x = 0.5, .y = 0.65};
	sprite->layer = SL_PLAYERS_AND_BALL;
	// A player faces the goal he is attacking until he first moves.
	sprite->orientation = pitch_side == PSI_NORTH ? SO_DOWN : SO_UP;
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
	player->characteristics = config.characteristics;
	// Resolve this player's constants from their characteristics (also sets physics->mass)
	entity_player_recompute_constants(entity, game_state);
	physics->max_velocity = player->constants.control_max_velocity;
	// Sets initial max velocity to half of max run velocity
	player->current_max_velocity = player->constants.run_max_velocity * 0.5f;
	enum state_machine_player_state initial_player_state = PS_NORMAL;

	// Initialise action
	struct component_action *action = &entity->action;
	action->header.enabled = true;

	// Initialise player controls
	struct component_player_controls *player_controls = &entity->player_controls;
	player_controls->header.enabled = true;
	player_controls->pass_target_entity_id = EID_NOT_VALID;

	if (config.is_goalie) {
		// Initialise goalie
		struct component_goalie *goalie = &entity->goalie;
		goalie->header.enabled = true;
		initial_player_state = PS_GOALIE;

		state_machine_goalie_init(
			&goalie->state_machine, entity, goalie_transitions, GS_PROTECT_GOAL, game_state);
	}

	sprite->before_draw = &player_custom_begin_draw;
	sprite->after_draw = &player_custom_after_draw;

	collider->num_colliders = PCT_COUNT;
	collider->colliders = memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL],
											sizeof(struct collider) * collider->num_colliders);
	ASSERT(collider->colliders);
	collider->colliders[PCT_GOALIE].shape = CS_CIRCLE;
	collider->colliders[PCT_GOALIE].props.circle.radius = 4;
	collider->colliders[PCT_GOALIE].props.circle.offset =
		(v2){.y = 0 + (collider->colliders[PCT_GOALIE].props.circle.radius * 0.6f *
					   (pitch_side == PSI_NORTH ? -1 : 1))};

	collider->colliders[PCT_DIVE].shape = CS_RECTANGLE;
	collider->colliders[PCT_DIVE].props.rectangle.size.x = 17;
	collider->colliders[PCT_DIVE].props.rectangle.size.y = 5;
	collider->colliders[PCT_DIVE].props.rectangle.offset =
		(v2){.x = 0,
			 .y = 0 + (collider->colliders[PCT_DIVE].props.rectangle.size.y * 1.0f *
					   (pitch_side == PSI_NORTH ? -1 : 1))};

	collider->colliders[PCT_PLAYER].shape = CS_CIRCLE;
	collider->colliders[PCT_PLAYER].props.circle.radius = player->constants.control_radius;

	collider->colliders[PCT_TACKLE].shape = CS_CIRCLE;
	collider->colliders[PCT_TACKLE].props.circle.radius = 6;

	collider->colliders[PCT_SLIDING_TACKLE_1].shape = CS_CIRCLE;
	collider->colliders[PCT_SLIDING_TACKLE_1].props.circle.radius = 4;
	collider->colliders[PCT_SLIDING_TACKLE_2].shape = CS_CIRCLE;
	collider->colliders[PCT_SLIDING_TACKLE_2].props.circle.radius = 5;
	collider->colliders[PCT_SLIDING_TACKLE_2].props.circle.offset = (v2){.y = -3};

	collider->colliders[PCT_TACKLE_SENSOR].shape = CS_CIRCLE;
	collider->colliders[PCT_TACKLE_SENSOR].props.circle.radius = 3;

	collider->colliders[PCT_HEADER].shape = CS_CIRCLE;
	collider->colliders[PCT_HEADER].props.circle.radius = 8;

	collider->colliders[PCT_BODY].shape = CS_CIRCLE;
	collider->colliders[PCT_BODY].props.circle.radius = 5;

	state_machine_player_init(
		&player->state_machine, entity, player_transitions, initial_player_state, game_state);

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "player created");

	return entity;
}

void entity_player_physics_apply_force(struct entity *player_entity,
									   Vector2 force_to_apply,
									   struct game_state *game_state)
{
	physics_apply_force(&player_entity->physics, force_to_apply, game_state->dt);
	physics_clamp_velocity_to_max(&player_entity->physics);
}

void entity_player_physics_apply_velocity(struct entity *player_entity,
										  struct game_state *game_state)
{
	physics_apply_velocity(&player_entity->transform, &player_entity->physics, game_state->dt);
}

v2 entity_player_steer_seek(struct entity *player_entity, v2 target)
{
	return Vector2Normalize(Vector2Subtract(target, player_entity->transform.position));
}

v2 entity_player_steer_arrive(struct entity *player_entity,
							  v2 target,
							  struct game_state *game_state)
{
	return entity_player_steer_arrive_limited(player_entity, target, FLT_MAX, game_state);
}

v2 entity_player_steer_arrive_limited(struct entity *player_entity,
									  v2 target,
									  f32 max_speed,
									  struct game_state *game_state)
{
	const struct player_constants *constants = &player_entity->player.constants;

	v2 to_target = Vector2Subtract(target, player_entity->transform.position);
	f32 distance = Vector2Length(to_target);

	// The fastest the player can run at and still stop on the target, braking at brake_ratio of
	// his full push. Far away this is above his top speed, so the stick stays at full length.
	v2 desired_velocity = Vector2Zero();
	if (distance > 0) {
		f32 brake_accel = constants->movement_force / constants->mass *
						  game_state->constants.player_steering_brake_ratio;
		desired_velocity =
			Vector2Scale(to_target, fminf(sqrtf(2 * brake_accel * distance), max_speed) / distance);
	}

	v2 steering = Vector2Scale(Vector2Subtract(desired_velocity, player_entity->physics.velocity),
							   1.0f / game_state->constants.player_steering_band);
	return Vector2ClampValue(steering, 0, 1);
}

v2 entity_player_steer_pursuit(struct entity *player_entity, v2 target, v2 target_velocity)
{
	v2 to_target = Vector2Subtract(target, player_entity->transform.position);

	// The lookahead time grows with the distance to the target and shrinks with the speed the
	// two close in at
	f32 look_ahead_time =
		Vector2Length(to_target) /
		(player_entity->player.constants.run_max_velocity + Vector2Length(target_velocity));

	return entity_player_steer_seek(
		player_entity, Vector2Add(target, Vector2Scale(target_velocity, look_ahead_time)));
}

void entity_player_anim_calculate_running_frame(struct entity *player_entity,
												asset_sprite_frame_id_t *stand_frames,
												asset_animation_id_t *run_animations,
												v2 direction,
												f32 velocity_threshold,
												struct game_state *game_state)
{
	struct component_physics *physics = &player_entity->physics;
	struct component_player *player = &player_entity->player;
	struct component_sprite *sprite = &player_entity->sprite;

	f32 player_speed = Vector2Length(physics->velocity);
	if (Vector2Length(physics->velocity) > velocity_threshold) {
		enum sprite_orientation sprite_orientation = sprite_calculate_orientation(direction);
		struct asset_animation *animation = asset_animation_get_by_id(
			game_state->assets.animation_cache, run_animations[sprite_orientation]);

		f32 max_player_velocity = player->constants.run_max_velocity;
		f32 player_speed_ratio = player_speed / max_player_velocity;

		if ((game_state->time - player->last_frame_update_time) >
			(animation->delay * (2.5f - player_speed_ratio * 1.2f))) {
			player->last_frame_update_time = game_state->time;
			u16 new_frame_id = (player->last_running_frame_idx + 1) % animation->frames_count;
			sprite->frame_id = animation->frames[new_frame_id];
			player->last_running_frame_idx = new_frame_id;
		}
		sprite->orientation = sprite_orientation;
	} else {
		sprite->frame_id = stand_frames[sprite->orientation];
	}
}

static void state_machine_player_on_enter(struct entity *entity,
										  enum state_machine_player_state from,
										  enum state_machine_player_trigger trigger,
										  enum state_machine_player_state to,
										  struct game_state *game_state)
{
	UNUSED(game_state);
	UNUSED(from);
	UNUSED(trigger);

	struct component_player *player = &entity->player;
	struct component_transform *player_transform = &entity->transform;
	struct component_transform *ball_transform = &game_state->vars.ball.entity->transform;
	struct component_sprite *sprite = &entity->sprite;
	struct state_machine_player *state_machine = &player->state_machine;

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "entering state: %s",
				   state_machine_player_state_string[to]);

	// Reset state data
	memset(&state_machine->state_data, 0, sizeof(union state_machine_player_state_data));

	switch (to) {
	case PS_TACKLE:
		entity->player.state_machine.state_data.tackle.direction =
			Vector2Normalize(entity->player_controls.steering);
		input_reset_player(entity->header.id, game_state);
		entity->sprite.frame_id = game_state->gfx.player.tackle_frames[sprite->orientation];
		entity->player.state_machine.state_data.tackle.start_time = game_state->time;
		break;
	case PS_SLIDING_TACKLE:
		input_reset_player(entity->header.id, game_state);
		entity->sprite.frame_id = game_state->gfx.player.sliding_tackle_frames[sprite->orientation];
		entity->player.state_machine.state_data.sliding_tackle.start_time = game_state->time;
		break;
	case PS_FALL:
		entity->player.state_machine.state_data.fall.animation_start_time = game_state->time;
		input_reset_player(entity->header.id, game_state);
		struct entity *ball_entity = game_state->vars.ball.entity;
		if (ball_entity->ball.last_touch_entity_id == entity->header.id) {
			// Falling player had the ball, so let's reset touch time to make it clear player is no longer in possession
			ball_entity->ball.last_touch_time = 0;
		}
		break;
	case PS_HEADER: {
		entity->player.state_machine.state_data.header.animation_start_time = game_state->time;
		entity->player.state_machine.state_data.header.phase = HP_GROUND;
		input_reset_player(entity->header.id, game_state);

		v2 direction = Vector2Subtract(ball_transform->position, player_transform->position);

		enum sprite_orientation sprite_orientation = sprite_calculate_orientation(direction);
		sprite->orientation = sprite_orientation;

		break;
	}
	case PS_TACKLED_RUN: {
		struct state_machine_player_tackled_run_data *tackled_run_data =
			&state_machine->state_data.tackled_run;
		input_reset_player(entity->header.id, game_state);
		const v2 velocity = entity->physics.velocity;
		tackled_run_data->direction =
			physics_is_at_rest(velocity, PLAYER_AT_REST_SPEED) ?
				Vector2Normalize(ORIENTATION_VECTORS[sprite->orientation]) :
				Vector2Normalize(velocity);
		tackled_run_data->speed =
			fmaxf(Vector2Length(velocity), game_state->constants.player_tackled_run_min_velocity);
		tackled_run_data->start_time = game_state->time;
		break;
	}
	case PS_NONE:
	case PS_NORMAL:
	case PS_GOALIE:
	case PS_THROW_IN:
	case PS_COUNT:
		break;
	}
}

static void state_machine_player_on_exit(struct entity *entity,
										 enum state_machine_player_state from,
										 enum state_machine_player_trigger trigger,
										 enum state_machine_player_state to,
										 struct game_state *game_state)
{
	UNUSED(game_state);
	UNUSED(trigger);
	UNUSED(to);

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "exiting state: %s",
				   state_machine_player_state_string[from]);

	switch (from) {
	case PS_NONE:
	case PS_NORMAL:
	case PS_GOALIE:
	case PS_THROW_IN:
	case PS_HEADER:
	case PS_TACKLE:
	case PS_SLIDING_TACKLE:
	case PS_FALL:
	case PS_TACKLED_RUN:
	case PS_COUNT:
		break;
	}
}

static void state_machine_goalie_on_enter(struct entity *entity,
										  enum state_machine_goalie_state from,
										  enum state_machine_goalie_trigger trigger,
										  enum state_machine_goalie_state to,
										  struct game_state *game_state)
{
	UNUSED(from);
	UNUSED(trigger);

	struct state_machine_goalie *state_machine = &entity->goalie.state_machine;

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "entering state: %s",
				   state_machine_goalie_state_string[to]);

	// Reset state data
	memset(&state_machine->state_data, 0, sizeof(union state_machine_goalie_state_data));

	switch (to) {
	case GS_NONE:
		break;
	case GS_PROTECT_GOAL: {
		entity->physics.max_velocity = entity->player.constants.run_max_velocity;
		break;
	}
	case GS_DIVE: {
		state_machine->state_data.dive.animation.start_time = game_state->time;
		break;
	}
	case GS_HOLD_BALL: {
		entity->physics.velocity = Vector2Zero();
		entity->physics.accel = Vector2Zero();
		entity->goalie.state_machine.state_data.hold_ball.hold_start_time = game_state->time;
		entity->sprite.frame_id =
			game_state->gfx.goalie.save_frames[game_state->vars.teams[entity->player.team_side]
												   .play_context.pitch_side][ST_WITH_BALL];
		break;
	}
	case GS_JUMP: {
		entity->physics.velocity = Vector2Zero();
		entity->physics.accel = Vector2Zero();
		state_machine->state_data.jump.animation.start_time = game_state->time;
		break;
	}
	case GS_THROW: {
		entity->physics.velocity = Vector2Zero();
		entity->physics.accel = Vector2Zero();
		state_machine->state_data.throw_ball.start_time = game_state->time;
		break;
	}
	case GS_COUNT:
		break;
	}
}

static void state_machine_goalie_on_exit(struct entity *entity,
										 enum state_machine_goalie_state from,
										 enum state_machine_goalie_trigger trigger,
										 enum state_machine_goalie_state to,
										 struct game_state *game_state)
{
	UNUSED(trigger);
	UNUSED(to);
	UNUSED(game_state);

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "exiting state: %s",
				   state_machine_goalie_state_string[from]);

	switch (from) {
	case GS_NONE:
		break;
	case GS_PROTECT_GOAL: {
		break;
	}
	case GS_DIVE: {
		entity->physics.max_velocity = entity->player.constants.run_max_velocity;
		break;
	}
	case GS_HOLD_BALL: {
		break;
	}
	case GS_JUMP: {
		break;
	}
	case GS_THROW: {
		break;
	}
	case GS_COUNT:
		break;
	}
}

// If ball velocity is sufficient, collision force depends on how aligned are the velocities of
// ball and player
static f32 ball_collision_force(v2 ball_velocity, v2 player_velocity, f32 slow_ball_force)
{
	if (Vector2LengthSqr(ball_velocity) <= 25) {
		return slow_ball_force;
	}
	return 0.5f * (1 - Vector2ProjectScalar(Vector2Normalize(ball_velocity),
											Vector2Normalize(player_velocity)));
}

void entity_player_update_colliders(struct entity *player_entity)
{
	struct collider *colliders = player_entity->collider.colliders;
	const enum sprite_orientation orientation = player_entity->sprite.orientation;

	colliders[PCT_PLAYER].props.circle.radius = player_entity->player.constants.control_radius;

	const struct tackle_collider_config tackle = ORIENTATION_TO_TACKLE_COLLIDER_CONFIG[orientation];
	colliders[PCT_TACKLE].props.circle.offset = tackle.offset;
	colliders[PCT_TACKLE].props.circle.radius = tackle.radius;

	const struct sliding_tackle_colliders_config sliding_tackle =
		ORIENTATION_TO_SLIDING_TACKLE_COLLIDERS_CONFIG[orientation];
	colliders[PCT_SLIDING_TACKLE_1].props.circle.offset = sliding_tackle.collider_1.offset;
	colliders[PCT_SLIDING_TACKLE_1].props.circle.radius = sliding_tackle.collider_1.radius;
	colliders[PCT_SLIDING_TACKLE_2].props.circle.offset = sliding_tackle.collider_2.offset;
	colliders[PCT_SLIDING_TACKLE_2].props.circle.radius = sliding_tackle.collider_2.radius;
}

struct collision_information
entity_player_is_colliding_with_ball(struct entity *entity,
									 enum player_collider_type collider_type,
									 v3 ball_position,
									 v3 ball_velocity,
									 f32 ball_radius,
									 f32 max_height)
{
	const v2 player_position = entity->transform.position;
	const v2 ball_pos = V3_TO_V2(ball_position);
	const struct collider *collider = &entity->collider.colliders[collider_type];

	b32 check_needed = true;
	b32 plays_ball = false;
	f32 slow_ball_force = 0;

	switch (collider_type) {
	case PCT_TACKLE:
	case PCT_SLIDING_TACKLE_1:
	case PCT_SLIDING_TACKLE_2:
		plays_ball = true;
		slow_ball_force = 0.5f;
		break;
	case PCT_HEADER:
		plays_ball = true;
		break;
	case PCT_PLAYER:
	case PCT_GOALIE:
	case PCT_DIVE:
		break;
	case PCT_BODY:
	case PCT_TACKLE_SENSOR:
	case PCT_COUNT:
		check_needed = false;
		break;
	}

	if (!check_needed || ball_position.y >= max_height) {
		return (struct collision_information){.is_colliding = false};
	}

	const b32 is_colliding =
		collider->shape == CS_RECTANGLE ?
			CheckCollisionCircleRec(
				ball_pos,
				ball_radius,
				collision_calculate_rectangle(player_position,
											  collider->props.rectangle.size,
											  collider->props.rectangle.offset)) :
			CheckCollisionCircles(ball_pos,
								  ball_radius,
								  Vector2Add(player_position, collider->props.circle.offset),
								  collider->props.circle.radius);

	struct collision_information collision = {.is_colliding = is_colliding};
	if (is_colliding && plays_ball) {
		collision.direction = Vector2Normalize(Vector2Subtract(ball_pos, player_position));
		collision.force = ball_collision_force(
			V3_TO_V2(ball_velocity), entity->physics.velocity, slow_ball_force);
	}
	return collision;
}

b32 entity_player_goalie_is_goalkeeping(struct entity *player_entity)
{
	return player_entity->player.state_machine.current_state == PS_GOALIE &&
		   player_entity->goalie.state_machine.current_state != GS_HOLD_BALL;
}

b32 entity_player_goalie_is_colliding_with_ball(struct entity *goalie_entity,
												struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);

	const struct component_goalie *goalie = &goalie_entity->goalie;

	const enum pitch_side pitch_side =
		game_state->vars.teams[goalie_entity->player.team_side].play_context.pitch_side;

	const v3 ball_position = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
	const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;

	switch (goalie->state_machine.current_state) {
	case GS_PROTECT_GOAL:
		return entity_player_is_colliding_with_ball(goalie_entity,
													PCT_GOALIE,
													ball_position,
													ball_physics_3d->velocity,
													ball_radius,
													game_state->constants.crossbar_height)
			.is_colliding;
	case GS_JUMP: {
		const struct state_machine_goalie_jump_data *jump_state_data =
			&goalie->state_machine.state_data.jump;

		return !jump_state_data->ball_saved &&
			   HAS_TIME_PASSED(game_state->time, goalie->last_save_time, 0.1f) &&
			   !jump_state_data->animation.completed &&
			   entity_player_is_colliding_with_ball(goalie_entity,
													PCT_GOALIE,
													ball_position,
													ball_physics_3d->velocity,
													ball_radius,
													game_state->constants.crossbar_height)
				   .is_colliding;
	}
	case GS_DIVE: {
		const struct state_machine_goalie_dive_data *dive_state_data =
			&goalie->state_machine.state_data.dive;

		return !dive_state_data->ball_saved &&
			   HAS_TIME_PASSED(game_state->time, goalie->last_save_time, 0.1f) &&
			   // If the ball hits the post and comes back we don't want to touch the ball
			   ((pitch_side == PSI_NORTH && ball_physics_3d->velocity.z < 0) ||
				(pitch_side == PSI_SOUTH && ball_physics_3d->velocity.z > 0)) &&
			   (!dive_state_data->animation.completed ||
				// If ball height is very low we are going to make the save even when
				// lying on the floor after the dive
				ball_transform->virtual_height < ball_radius) &&
			   entity_player_is_colliding_with_ball(goalie_entity,
													PCT_DIVE,
													ball_position,
													ball_physics_3d->velocity,
													ball_radius,
													game_state->constants.crossbar_height -
														ball_radius * 1.5f)
				   .is_colliding;
	}
	case GS_HOLD_BALL:
		// With the ball in his hands he is in contact with it every tick. Keyed on him holding it
		// and never on who touched it last, so that anything else taking the last touch for a
		// frame cannot strand him holding a ball he can no longer release.
		return ball->state_machine.current_state == BS_GK_HANDS;
	case GS_THROW:
	case GS_NONE:
	case GS_COUNT:
		break;
	}
	return false;
}

void entity_player_goalie_on_the_ball(struct entity *goalie_entity, struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);

	struct component_player *player = &goalie_entity->player;
	struct component_goalie *goalie = &goalie_entity->goalie;
	const entity_id_t goalie_entity_id = goalie_entity->header.id;

	const enum pitch_side pitch_side =
		game_state->vars.teams[player->team_side].play_context.pitch_side;

	const f32 ball_velocity = Vector3Length(ball_physics_3d->velocity);

	// A ball a team mate played back to him has to be played with his feet
	const b32 may_handle_ball = !entity_ball_is_goalie_hands_barred(ball_entity, player->team_side);

	switch (goalie->state_machine.current_state) {
	case GS_PROTECT_GOAL: {
		// Choose whether to bounce or hold the ball
		// Definitely hold if ball velocity is lower than 54km/h
		// Sometimes hold if ball velocity is lower than 108km/h
		// Otherwise bounce it
		const b32 hold = may_handle_ball && GOALIE_HOLD_OR_BOUNCE(ball_velocity);

		if (hold) {
			// Hold ball in hands
			entity_ball_held_by_goalie(ball_entity, goalie_entity_id, game_state);
			state_machine_goalie_fire(&goalie->state_machine, GT_CATCH, game_state);

		} else {
			// Bounce ball
			v3 new_ball_velocity = Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1});

			// Step ball position back one tick to place it outside the collider
			v3 where_to_place_ball =
				Vector3Add(V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
						   Vector3Scale(Vector3Negate(ball_physics_3d->velocity), game_state->dt));

			entity_ball_set_position_and_velocity(
				ball_entity, where_to_place_ball, Vector3Scale(new_ball_velocity, 0.3f));
			entity_ball_saved_by_goalie(ball_entity, goalie_entity_id, game_state);

			// TODO: we should play some animation here and maybe transition to some stunned state
		}
		break;
	}
	case GS_JUMP: {
		struct state_machine_goalie_jump_data *jump_state_data =
			&goalie->state_machine.state_data.jump;

		// Choose whether to bounce or hold the ball
		// Definitely hold if ball velocity is lower than 54km/h
		// Sometimes hold if ball velocity is lower than 108km/h
		// Otherwise bounce it
		jump_state_data->ball_held = may_handle_ball && GOALIE_HOLD_OR_BOUNCE(ball_velocity);

		if (jump_state_data->ball_held) {
			entity_ball_held_by_goalie(ball_entity, goalie_entity_id, game_state);
		} else {
			// Bounce ball
			v3 new_ball_velocity =
				Vector3Normalize(Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1}));
			new_ball_velocity.y = 1;

			// Step ball position back one tick to place it outside the collider
			v3 where_to_place_ball =
				Vector3Add(V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
						   Vector3Scale(Vector3Negate(ball_physics_3d->velocity), game_state->dt));

			entity_ball_set_position_and_velocity(ball_entity,
												  where_to_place_ball,
												  Vector3Scale(new_ball_velocity,
															   10 * PIXELS_PER_METER));
			entity_ball_saved_by_goalie(ball_entity, goalie_entity_id, game_state);
		}

		goalie->last_save_time = game_state->time;
		jump_state_data->ball_saved = true;
		break;
	}
	case GS_DIVE: {
		struct state_machine_goalie_dive_data *dive_state_data =
			&goalie->state_machine.state_data.dive;

		// Choose whether to bounce or hold the ball
		// Definitely hold if ball velocity is lower than 54km/h
		// Sometimes hold if ball velocity is lower than 108km/h
		// Definitely don't hold the ball if ball is very high
		// Otherwise bounce it
		dive_state_data->ball_held = may_handle_ball && GOALIE_HOLD_OR_BOUNCE(ball_velocity) &&
									 ball_transform->virtual_height < 25;

		if (dive_state_data->ball_held) {
			entity_ball_held_by_goalie(ball_entity, goalie_entity_id, game_state);
		} else {
			// Bounce ball
			// Type of deflection depends on where the ball hit the diving goalkeeper

			f32 deflection_x = ball_transform->position.x - goalie_entity->transform.position.x;
			f32 dive_collider_width =
				goalie_entity->collider.colliders[PCT_DIVE].props.rectangle.size.x;
			f32 deflection_factor_x = Clamp1(deflection_x / (dive_collider_width * 0.5f));

			// Deflection velocity is scaled down from the impact velocity.
			// Minimum is 0.3x the original, max 0.5x.
			// If the ball is saved in the middle of the collider it's going to be near to 0.3x
			// If it's saved at the edges it's going to be near to 0.5x
			f32 velocity_scale = 0.5f - (1 - deflection_factor_x * deflection_factor_x) * 0.2f;

			GLOGF_TRACE_CTX(LS_GAME,
							goalie_entity->header.name,
							"dive deflection factor x: %.2f",
							deflection_factor_x);

			v3 deflection = (v3){.x = 0, .y = 0, .z = ball_physics_3d->velocity.z > 0 ? -1 : 1};
			deflection = Vector3RotateByAxisAngle(deflection,
												  (v3){.y = pitch_side == PSI_SOUTH ? 1 : -1},
												  -deflection_factor_x * 90 * DEG2RAD);

			// Deflection over the bar
			if (ball_transform->virtual_height >= 25) {
				deflection = Vector3RotateByAxisAngle(deflection,
													  (v3){.x = pitch_side == PSI_SOUTH ? 1 : -1},
													  GetRandomValue(130, 170) * DEG2RAD);
				// Deflections over the bar keep most of the original velocity
				velocity_scale = 0.9f - (1 - deflection_factor_x * deflection_factor_x) * 0.2f;
			}

			GLOGF_TRACE_CTX(
				LS_GAME, goalie_entity->header.name, "dive velocity scale: %.2f", velocity_scale);

			deflection =
				Vector3Scale(deflection, Vector3Length(ball_physics_3d->velocity) * velocity_scale);

			// Step ball position back one tick to place it outside the collider
			v3 where_to_place_ball =
				Vector3Add(V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
						   Vector3Scale(Vector3Negate(ball_physics_3d->velocity), game_state->dt));

			entity_ball_set_position_and_velocity(ball_entity, where_to_place_ball, deflection);
			entity_ball_saved_by_goalie(ball_entity, goalie_entity_id, game_state);
		}

		goalie->last_save_time = game_state->time;
		dive_state_data->ball_saved = true;
		break;
	}
	case GS_HOLD_BALL: {
		// What he does with the ball in his hands
		if (ball->state_machine.current_state != BS_GK_HANDS) {
			break;
		}

		DECLARE_ENTITY_COMPONENT(goalie_entity, sprite);

		struct component_player_controls *player_controls = &goalie_entity->player_controls;
		const v2 goalie_position = goalie_entity->transform.position;
		const struct collider *goalie_collider = &goalie_entity->collider.colliders[PCT_GOALIE];
		const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;

		const i8 direction = pitch_side == PSI_NORTH ? 1 : -1;
		const v2 new_ball_position =
			Vector2Add(goalie_position,
					   Vector2Scale((v2){.y = direction},
									goalie_collider->props.circle.offset.y +
										goalie_collider->props.circle.radius + ball_radius + 7));

		if (player_controls->put_down) {
			entity_ball_released_by_goalie(ball_entity, goalie_entity_id, game_state);
			entity_ball_set_position_and_velocity(
				ball_entity, V2_TO_V3(new_ball_position, ball_radius), Vector3Zero());
			state_machine_goalie_fire(&goalie->state_machine, GT_RELEASE, game_state);
			state_machine_player_fire(&player->state_machine, PT_STEP_OUT, game_state);
			input_reset_player(goalie_entity_id, game_state);
			break;
		}

		if (player_controls->pass > 0) {
			// Thrown out by hand, led to the team mate the way an automatic pass is
			v2 throw_direction = player_controls->steering;
			if (IS_V2_ZERO(throw_direction)) {
				throw_direction = (v2){.y = direction};
			}
			f32 throw_power = player_controls->pass;

			struct entity *target_entity =
				entity_get(game_state->entity_repository, player_controls->pass_target_entity_id);
			if (target_entity) {
				struct player_pass pass =
					entity_player_aim_pass(goalie_entity, ball_entity, target_entity, game_state);
				throw_direction = V3_TO_V2(pass.direction);
				throw_power = pass.power * pass.power;
				ball->pass_target_entity_id = target_entity->header.id;
			}

			throw_direction = Vector2Normalize(throw_direction);
			sprite->orientation = sprite_calculate_orientation(throw_direction);

			const v2 ball_release_position =
				Vector2Add(goalie_position,
						   Vector2Scale(throw_direction,
										goalie_collider->props.circle.radius + ball_radius + 2));
			entity_ball_released_by_goalie(ball_entity, goalie_entity_id, game_state);
			entity_ball_set_position_and_velocity(
				ball_entity, V2_TO_V3(ball_release_position, GOALIE_THROW_HEIGHT), Vector3Zero());
			// Rolled out along the ground, so no height on the direction
			entity_ball_short_pass(ball_entity,
								   goalie_entity_id,
								   V2_TO_V3(throw_direction, 0),
								   throw_power,
								   player_controls->automatic_pass,
								   game_state);

			state_machine_goalie_fire(&goalie->state_machine, GT_THROW, game_state);
			// Set after the transition because on_enter resets the state data
			goalie->state_machine.state_data.throw_ball.direction = throw_direction;
			goalie->state_machine.state_data.throw_ball.power = throw_power;
			break;
		}

		if (player_controls->long_pass > 0) {
			entity_ball_released_by_goalie(ball_entity, goalie_entity_id, game_state);
			entity_ball_set_position_and_velocity(
				ball_entity, V2_TO_V3(new_ball_position, 10), Vector3Zero());
			entity_ball_long_pass(ball_entity,
								  goalie_entity_id,
								  (v2){.y = direction},
								  player_controls->long_pass,
								  false,
								  game_state);
			state_machine_goalie_fire(&goalie->state_machine, GT_RELEASE, game_state);
			break;
		}

		// Human or AI, he leaves GS_HOLD_BALL only through the controls above
		break;
	}
	case GS_THROW:
	case GS_NONE:
	case GS_COUNT:
		break;
	}
}

void entity_player_goalie_dive_start(struct entity *goalie_entity,
									 struct goalie_dive dive,
									 struct game_state *game_state)
{
	state_machine_goalie_fire(&goalie_entity->goalie.state_machine, GT_DIVE, game_state);
	// Set after the transition because on_enter resets the state data
	goalie_entity->goalie.state_machine.state_data.dive.dive = dive;
	goalie_entity->physics.max_velocity =
		dive.speed * game_state->constants.goalie_dive_max_velocity;
}

b32 entity_player_goalie_should_dive(struct entity *goalie_entity,
									 struct entity *ball_entity,
									 struct goalie_dive *dive,
									 struct game_state *game_state)
{
	b32 should_dive = false;
	const f32 ball_radius = ball_entity->collider.colliders[0].props.circle.radius;

	const u32 steps_per_lookahead = game_state->constants.ball_lookahead_dts;
	// We want a specific lookahead for our goalie
	const int lookaheads = floor(
		Clamp((GOALIE_LOOKAHEAD_MS / game_state->constants.fixed_timestep) / steps_per_lookahead,
			  1,
			  game_state->constants.ball_lookaheads));

	// We need to account for both pitch sides
	const b32 attacking_south =
		game_state->vars.teams[goalie_entity->player.team_side].play_context.pitch_side ==
		PSI_NORTH;

	// Check whether the ball is coming directly at us
	if (collision_is_moving_circle_hitting_still_circle(
			ball_entity->transform.position,
			goalie_entity->transform.position,
			ball_radius,
			goalie_entity->collider.colliders[PCT_GOALIE].props.circle.radius,
			V3_TO_V2(ball_entity->physics3d.velocity))) {
		// Ball is going to end up in our hands, don't dive!
		return false;
	}

	// TODO: reaction time is not fixed for real goalkeepers:
	// sometimes they dive by instinct and some simply have better reflexes.
	// We should implement this here by adding some randomness to the reaction time
	// and by taking goalkeeper quality into consideration.

	// For now reaction time can be 50% better than the default one 2% of the time
	f32 reaction_time =
		game_state->constants.goalie_dive_reaction_time * (RANDOM_01(100) > 0.02f ? 1 : 0.5f);

	if (!HAS_TIME_PASSED(game_state->time, ball_entity->ball.last_loose_time, reaction_time)) {
		return false;
	}

	if ((should_dive = check_whether_goalie_should_dive(
			 V2_TO_V3(ball_entity->transform.position, ball_entity->transform.virtual_height),
			 attacking_south,
			 0.0f,
			 goalie_entity,
			 dive,
			 game_state))) {
		// Usually at this point there's nothing left to do, but let's dive anyway to show off
		return true;
	}

	for (u16 i = 0; i < lookaheads; ++i) {
		const v3 pos_to_check = game_state->vars.ball.lookahead_positions[i];

		f32 time_to_unsaveable = (i + 1) * game_state->dt * steps_per_lookahead;

		if ((should_dive = check_whether_goalie_should_dive(pos_to_check,
															attacking_south,
															time_to_unsaveable,
															goalie_entity,
															dive,
															game_state))) {
			break;
		}
	}
	return should_dive;
}

void entity_player_gained_possession(struct entity *player_entity, struct game_state *game_state)
{
	player_entity->player.last_possession_time = game_state->time;

	entity_ball_set_last_touch(game_state->vars.ball.entity, player_entity->header.id, game_state);
}

enum player_ball_possession entity_player_ball_possession(struct entity *player_entity,
														  struct entity *ball_entity,
														  struct game_state *game_state)
{
	if (player_entity->header.id == ball_entity->ball.last_touch_entity_id) {
		if (ball_entity->ball.last_touch_time == game_state->time) {
			return PBP_ON_THE_BALL;
		} else {
			return PBP_IN_POSSESSION;
		}
	} else {
		return PBP_OFF_THE_BALL;
	}
}

b32 entity_player_is_goalie(struct entity *player_entity)
{
	return player_entity->goalie.header.enabled;
}

b32 entity_player_goalie_try_pick_up_ball(struct entity *player_entity,
										  struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;
	const enum pitch_side pitch_side =
		game_state->vars.teams[player_entity->player.team_side].play_context.pitch_side;

	// Only a keeper playing on his feet takes it in his hands from here: mid-header, mid-tackle or
	// on the floor he finishes what he is doing first, and PT_PICK_UP leads nowhere from there.
	if (!entity_player_is_goalie(player_entity) ||
		player_entity->player.state_machine.current_state != PS_NORMAL ||
		!football_pitch_is_position_inside_penalty_area(
			player_entity->transform.position, pitch_side, game_state) ||
		entity_ball_is_goalie_hands_barred(ball_entity, player_entity->player.team_side)) {
		return false;
	}

	entity_ball_held_by_goalie(ball_entity, player_entity->header.id, game_state);
	state_machine_player_fire(&player_entity->player.state_machine, PT_PICK_UP, game_state);
	state_machine_goalie_fire(&player_entity->goalie.state_machine, GT_CATCH, game_state);
	input_reset_player(player_entity->header.id, game_state);

	return true;
}

void entity_player_goalie_update_role(struct entity *player_entity, struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;
	struct component_player *player = &player_entity->player;
	struct component_goalie *goalie = &player_entity->goalie;

	const enum pitch_side pitch_side =
		game_state->vars.teams[player->team_side].play_context.pitch_side;
	const b32 is_in_own_box = football_pitch_is_position_inside_penalty_area(
		player_entity->transform.position, pitch_side, game_state);
	const b32 is_play_running = game_state->gameplay.state_machine.current_state == FS_PLAY;

	// A ball he may not handle was last played by his own side and no opponent has touched it since,
	// so he has to play it with his feet
	const b32 may_handle_ball = !entity_ball_is_goalie_hands_barred(ball_entity, player->team_side);

	// Both roles read this one answer, so they can never send him back and forth
	const b32 should_guard_goal = is_in_own_box && (!is_play_running || may_handle_ball);

	switch (player->state_machine.current_state) {
	case PS_GOALIE: {
		if (goalie->state_machine.current_state != GS_PROTECT_GOAL) {
			// A dive, a jump and the ball in his hands all play out to their end first
			break;
		}

		if (!should_guard_goal) {
			state_machine_player_fire(&player->state_machine, PT_STEP_OUT, game_state);
		}
		break;
	}
	case PS_NORMAL: {
		if (should_guard_goal) {
			state_machine_player_fire(&player->state_machine, PT_GUARD_GOAL, game_state);
		}
		break;
	}
	default:
		// Mid-header, mid-tackle, on the floor: he finishes what he is doing and comes back to
		// PS_NORMAL, where this runs again.
		break;
	}
}

struct player_ball_reachability_prediction entity_player_can_reach_future_ball_position(
	struct entity *seeker_entity,
	f32 seeker_max_speed,
	v3 *future_ball_positions,
	b32(predicate)(v3 pos_to_check, struct entity *seeker_entity, struct game_state *game_state),
	struct game_state *game_state)
{
	struct player_ball_reachability_prediction prediction = {.can_reach_it = false};

	const int steps_per_lookahead = game_state->constants.ball_lookahead_dts;
	const int lookaheads = game_state->constants.ball_lookaheads;
	for (u16 i = 0; i < lookaheads; ++i) {
		v3 pos_to_check = future_ball_positions[i];
		if (!predicate || predicate(pos_to_check, seeker_entity, game_state)) {
			fg_time_t lookahead_time = (i + 1) * game_state->dt * steps_per_lookahead;
			if (physics_can_reach_position_in_time(seeker_entity->transform.position,
												   V3_TO_V2(pos_to_check),
												   lookahead_time,
												   seeker_max_speed)) {
				prediction.can_reach_it = true;
				prediction.future_pos = pos_to_check;
				prediction.future_time = lookahead_time;
				return prediction;
			}
		}
	}
	return prediction;
}

v2 entity_player_position_to_seek_to_reach_ball(struct entity *player_entity,
												struct entity *ball_entity,
												struct game_state *game_state)
{
	struct player_ball_reachability_prediction prediction =
		entity_player_can_reach_future_ball_position(
			player_entity,
			player_entity->player.constants.run_max_velocity * 0.8f,
			game_state->vars.ball.lookahead_positions,
			NULL,
			game_state);
	v3 ball_future_pos;
	if (prediction.can_reach_it)
		ball_future_pos = prediction.future_pos;
	else
		ball_future_pos =
			V2_TO_V3(ball_entity->transform.position, ball_entity->transform.virtual_height);
	return V3_TO_V2(ball_future_pos);
}

b32 entity_player_find_pass_target(struct entity *player_on_the_ball_entity,
								   v2 scan_direction,
								   f32 detection_angle,
								   f32 target_distance,
								   f32 max_distance,
								   struct entity **out_pass_target,
								   struct game_state *game_state)
{
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *selected_entity = NULL;
	f32 selected_score = 0;
	FOR_EACH_ENTITY(candidate_entity, entity_repository)
	{
		if (!candidate_entity->header.alive || candidate_entity->header.type != ET_PLAYER ||
			candidate_entity->header.id == player_on_the_ball_entity->header.id ||
			candidate_entity->player.team_side != player_on_the_ball_entity->player.team_side) {
			continue;
		}

		v2 to_player = Vector2Subtract(candidate_entity->transform.position,
									   player_on_the_ball_entity->transform.position);
		f32 angle = fabs(Vector2Angle(scan_direction, to_player) * RAD2DEG);
		if (angle > detection_angle)
			continue;

		b32 clear_trajectory = entity_player_is_trajectory_clear(
			player_on_the_ball_entity->transform.position,
			candidate_entity->transform.position,
			OPPONENT_TEAM_SIDE(candidate_entity->player.team_side),
			game_state);

		// The man nearest to target_distance along his own line from the passer ranks highest
		const f32 distance_from_target = fabsf(Vector2Length(to_player) - target_distance);
		f32 distance_factor =
			Clamp01((distance_from_target * distance_from_target) / (max_distance * max_distance));
		f32 score = 0.33f * (1 - distance_factor);
		score += 0.67f * clear_trajectory;

		if (score > selected_score) {
			selected_entity = candidate_entity;
			selected_score = score;
		}
	}
	if (selected_entity) {
		GLOGF_TRACE(LS_GAME,
					"selected pass target %u with score %.2f",
					selected_entity->header.id,
					selected_score);
		*out_pass_target = selected_entity;
	}
	return selected_entity != NULL;
}

entity_id_t entity_player_choose_pass_target(struct entity *passer_entity,
											 v2 steering,
											 v2 centred_stick_direction,
											 f32 target_distance,
											 struct game_state *game_state)
{
	const v2 scan_direction =
		Vector2Length(steering) < 0.1f ? centred_stick_direction : steering;
	struct entity *target_entity;
	if (entity_player_find_pass_target(passer_entity,
									   scan_direction,
									   PASS_SCAN_HALF_ANGLE,
									   target_distance,
									   game_state->constants.ball_pass_auto_max_distance,
									   &target_entity,
									   game_state)) {
		return target_entity->header.id;
	}
	return EID_NOT_VALID;
}

struct player_pass entity_player_aim_pass(struct entity *passer_entity,
										  struct entity *ball_entity,
										  struct entity *target_entity,
										  struct game_state *game_state)
{
	const f32 max_distance = game_state->constants.ball_pass_auto_max_distance;
	const v2 passer_position = passer_entity->transform.position;
	const v2 target_position = target_entity->transform.position;
	const v2 target_velocity = target_entity->physics.velocity;
	const rect_t field = game_state->vars.pitch.field;
	const f32 min_x = field.x + PASS_AIM_FIELD_MARGIN;
	const f32 max_x = field.x + field.width - PASS_AIM_FIELD_MARGIN;
	const f32 min_y = field.y + PASS_AIM_FIELD_MARGIN;
	const f32 max_y = field.y + field.height - PASS_AIM_FIELD_MARGIN;
	const f32 pass_height = game_state->constants.ball_pass_height;
	const f32 lead_amount = game_state->constants.ball_pass_auto_lead_amount;

	// Aim point and ball travel time depend on each other: leading further makes the pass
	// longer, which makes it both faster and slower to arrive. Solved by fixed point iteration.
	v2 aim_point = target_position;
	for (u8 i = 0; i < PASS_AIM_ITERATIONS; ++i) {
		v2 to_aim = Vector2Subtract(aim_point, passer_position);
		f32 distance = Vector2Length(to_aim);
		f32 power = Clamp01(distance / max_distance);
		v3 direction = Vector3Normalize(V2_TO_V3(to_aim, pass_height));
		f32 speed = math_ext_split_lerp(game_state->constants.ball_pass_force, 0.5f, power * power);

		fg_time_t travel_time = entity_ball_time_to_cover_distance(
			ball_entity, Vector3Scale(direction, speed), distance, game_state);

		v2 lead = Vector2Scale(target_velocity, travel_time * lead_amount);
		v2 next_aim_point = Vector2Add(target_position, lead);
		aim_point = i == 0 ? next_aim_point :
							 Vector2Lerp(aim_point, next_aim_point, PASS_AIM_RELAXATION);

		// Beyond the auto pass range the power saturates and the ball falls short, so pull
		// the lead back in rather than aiming somewhere we cannot reach
		v2 to_aim_led = Vector2Subtract(aim_point, passer_position);
		f32 led_distance = Vector2Length(to_aim_led);
		if (led_distance > max_distance) {
			aim_point =
				Vector2Add(passer_position, Vector2Scale(to_aim_led, max_distance / led_distance));
		}

		// Leading a player running at the touchline must not put the ball out: cut the lead short
		// along his run, and if he is already outside the margin, aim inside it from where he is
		const v2 aim_lead = Vector2Subtract(aim_point, target_position);
		f32 lead_fraction = 1.0f;
		if (aim_point.x < min_x && aim_lead.x < 0)
			lead_fraction = MIN(lead_fraction, (min_x - target_position.x) / aim_lead.x);
		if (aim_point.x > max_x && aim_lead.x > 0)
			lead_fraction = MIN(lead_fraction, (max_x - target_position.x) / aim_lead.x);
		if (aim_point.y < min_y && aim_lead.y < 0)
			lead_fraction = MIN(lead_fraction, (min_y - target_position.y) / aim_lead.y);
		if (aim_point.y > max_y && aim_lead.y > 0)
			lead_fraction = MIN(lead_fraction, (max_y - target_position.y) / aim_lead.y);
		aim_point = Vector2Add(target_position, Vector2Scale(aim_lead, Clamp01(lead_fraction)));
		aim_point.x = Clamp(aim_point.x, min_x, max_x);
		aim_point.y = Clamp(aim_point.y, min_y, max_y);
	}

	const v2 to_aim = Vector2Subtract(aim_point, passer_position);
	return (struct player_pass){.direction = Vector3Normalize(V2_TO_V3(to_aim, pass_height)),
								.power = Clamp01(Vector2Length(to_aim) / max_distance)};
}

// Going over 80% of charge it's going to be bad for the shot
// - accuracy will decrease
// - shot power will decrease
// - shot height will rise
struct player_shot entity_player_aim_shot(struct entity *player_entity,
										  struct entity *ball_entity,
										  f32 charge,
										  struct game_state *game_state)
{
	const v2 ball_position = ball_entity->transform.position;
	const v2 player_velocity = player_entity->physics.velocity;
	const v2 steering = player_entity->player_controls.steering;
	const enum pitch_side player_pitch_side =
		game_state->vars.teams[player_entity->player.team_side].play_context.pitch_side;

	f32 accuracy = game_state->vars.pitch.goal_width * 0.5f +
				   (charge > 0.8f ? GetRandomValue(11, 20) : GetRandomValue(0, 10));

	v2 center_of_goal = game_state->vars.pitch.goal[1 - player_pitch_side].center;
	v2 shot_direction_to_center = Vector2Normalize(Vector2Subtract(center_of_goal, ball_position));
	v2 shot_direction = steering;
	if (Vector2ProjectScalar(shot_direction_to_center, shot_direction) < 0) {
		shot_direction.y *= -1;
	}
	if (game_state->constants.shoot_aid_auto_aim) {
		// Calculate shot direction for auto aim
		v2 shot_direction_to_right_post = Vector2Normalize(
			Vector2Subtract(Vector2Add(center_of_goal, (v2){.x = accuracy}), ball_position));
		f32 angle_center_to_post =
			Vector2Angle(shot_direction_to_center, shot_direction_to_right_post);

		shot_direction = Vector2Rotate(shot_direction_to_center, angle_center_to_post * steering.x);
	}

	// Limit angle between requested shoot direction and current velocity
	f32 angle = Vector2Angle(player_velocity, shot_direction);
	f32 max_angle = 100 * DEG2RAD;
	if (angle > max_angle) {
		shot_direction = Vector2Normalize(Vector2Rotate(player_velocity, max_angle));
	} else if (angle < -max_angle) {
		shot_direction = Vector2Normalize(Vector2Rotate(player_velocity, -max_angle));
	}

	// Stick Y controls shot height: toward goal = high, away = low
	f32 attack_sign = shot_direction_to_center.y > 0 ? -1.0f : 1.0f;
	f32 toward_goal = steering.y * attack_sign;
	// 0.4 should allow a shot to hit the goal even when the stick if fully tilted towards goal
	// unless shot power is overcharged
	f32 shot_height = Clamp01((1.0f - toward_goal) * 0.4f);

	// Calculate shot force
	// Max power is achieved at 80%, if you go over you actually lose power
	f32 power = charge;
	if (power > 0.8f) {
		f32 overcharge_pct = (power - 0.8f) / 0.2f;
		// Lose 20% of power immediately and another 12,5% proportionally to overcharge
		power = 0.8f - 0.1f * overcharge_pct;
		// Shot height is increased by 30% immediately and by another 30% proportionally to overcharge
		shot_height *= 1 + 0.3f + 0.3f * overcharge_pct;
	} else {
		power = power / 0.8f;
	}

	return (struct player_shot){
		.direction = shot_direction,
		.height = shot_height,
		.power = power,
	};
}

// Whether jumping is what the player actually meant: the ball has to be coming in above heading
// height on a path he will meet. Below that he wants to volley it, and leaving the ground for a
// header instead is a bad outcome. No upper bound on purpose — a ball high in the sky is always
// worth going up for.
b32 entity_player_should_jump_for_header(struct entity *player_entity,
										 struct game_state *game_state)
{
	for (usize i = 0; i < game_state->constants.ball_lookaheads; ++i) {
		fg_time_t time_elapsed =
			(i + 1) * game_state->constants.ball_lookahead_dts * game_state->dt;

		v3 pos_to_check = game_state->vars.ball.lookahead_positions[i];

		v2 player_future_position =
			Vector2Add(player_entity->transform.position,
					   Vector2Scale(player_entity->physics.velocity, time_elapsed));

		f32 is_colliding = CheckCollisionCircles(
			V3_TO_V2(pos_to_check),
			game_state->vars.ball.entity->collider.colliders[0].props.circle.radius,
			player_future_position,
			15);
		if (is_colliding && pos_to_check.y > HEADER_MIN_HEIGHT) {
			return true;
		}
	}
	return false;
}

// A ball position the keeper should come off his line for: inside his own penalty area, near
// enough to the goal, and low enough to get to. Shaped as a predicate for
// entity_player_can_reach_future_ball_position.
b32 entity_player_goalie_can_claim_ball_at(v3 pos_to_check,
										   struct entity *seeker_entity,
										   struct game_state *game_state)
{
	enum pitch_side side =
		game_state->vars.teams[seeker_entity->player.team_side].play_context.pitch_side;
	f32 ball_distance_from_goal =
		Vector3Distance(pos_to_check, V2_TO_V3(game_state->vars.pitch.goal[side].center, 0));
	return (
		football_pitch_is_position_inside_penalty_area(V3_TO_V2(pos_to_check), side, game_state) &&
		ball_distance_from_goal < 125 &&
		pos_to_check.y < (game_state->constants.crossbar_height - 7));
}

b32 entity_player_find_nearest_facing_opponent(struct entity *player_entity,
											   struct nearest_player *out_nearest_facing_oppo,
											   struct game_state *game_state)
{
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *opponent_entity;
	for (usize opponent_entity_id = 0; opponent_entity_id < entity_repository->max_slots;
		 ++opponent_entity_id) {
		opponent_entity = entity_get(game_state->entity_repository, opponent_entity_id);
		if (opponent_entity->header.alive && opponent_entity->header.type == ET_PLAYER) {
			struct component_transform *transform = &opponent_entity->transform;

			if (player_entity->header.id == opponent_entity_id ||
				opponent_entity->player.team_side == player_entity->player.team_side) {
				// If player is in our team or the player we are considering we skip
				continue;
			}
			v2 to_opponent = Vector2Subtract(opponent_entity->transform.position,
											 player_entity->transform.position);
			f32 angle = fabs(Vector2Angle(player_entity->physics.velocity, to_opponent) * RAD2DEG);

			f32 distance = Vector2Distance(player_entity->transform.position, transform->position);
			if (distance < 100 && angle < 30) {
				out_nearest_facing_oppo->entity = opponent_entity;
				out_nearest_facing_oppo->distance = distance;
				return true;
			}
		}
	}
	return false;
}

struct list_entities entity_player_find_tackled_opponents(struct entity *player_entity,
														  enum player_collider_type tackle_collider,
														  struct game_state *game_state)
{
	const u8 max_tackled_players = 3;
	LIST_CREATE(struct list_entities,
				tackled_players,
				max_tackled_players,
				game_state->memory_by_lifetime[MAL_UPDATE]);
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *opponent_entity;
	for (usize opponent_entity_id = 0; opponent_entity_id < entity_repository->max_slots;
		 ++opponent_entity_id) {
		opponent_entity = entity_get(game_state->entity_repository, opponent_entity_id);
		if (opponent_entity->header.alive && opponent_entity->header.type == ET_PLAYER &&
			opponent_entity->player.state_machine.current_state == PS_NORMAL &&
			opponent_entity->player.team_side != player_entity->player.team_side) {

			const v2 opponent_collider_pos = Vector2Add(
				opponent_entity->transform.position,
				opponent_entity->collider.colliders[PCT_TACKLE_SENSOR].props.circle.offset);
			const struct collider *tackle = &player_entity->collider.colliders[tackle_collider];
			const v2 player_collider_pos =
				Vector2Add(player_entity->transform.position, tackle->props.circle.offset);

			const b32 is_tackled = CheckCollisionCircles(
				player_collider_pos,
				tackle->props.circle.radius,
				opponent_collider_pos,
				opponent_entity->collider.colliders[PCT_TACKLE_SENSOR].props.circle.radius);

			if (is_tackled) {
				LIST_ADD(tackled_players, opponent_entity)
				if (tackled_players.count == max_tackled_players) {
					break;
				}
			}
		}
	}
	return tackled_players;
}

void entity_player_tackled(struct entity *player_entity,
						   struct entity *tackler_entity,
						   f32 knockdown_probability,
						   struct game_state *game_state)
{
	struct state_machine_player *state_machine = &player_entity->player.state_machine;

	if (RANDOM_01(100000) < knockdown_probability) {
		state_machine_player_fire(state_machine, PT_KNOCKED_DOWN, game_state);
		GLOGF_INFO_CTX(LS_GAME,
					   player_entity->header.name,
					   "player knocked down by %s",
					   tackler_entity->header.name);
	} else {
		state_machine_player_fire(state_machine, PT_TACKLED, game_state);
		GLOGF_INFO_CTX(LS_GAME,
					   player_entity->header.name,
					   "player tackled by %s",
					   tackler_entity->header.name);
	}
}

b32 entity_player_find_nearest(v2 position,
							   enum team_side team_side,
							   struct list_entity_ids exclusion_list,
							   struct nearest_player *out_nearest_player,
							   struct game_state *game_state)
{
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *selected_entity = NULL;
	f32 selected_distance = FLT_MAX;
	struct entity *entity;
	for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
		entity = entity_get(game_state->entity_repository, entity_id);
		if (entity->header.alive && entity->header.type == ET_PLAYER) {
			struct component_transform *transform = &entity->transform;

			b32 should_be_excluded = false;
			LIST_ITERATE(excluded_entity_id, exclusion_list)
			{
				if (entity->header.id == excluded_entity_id) {
					should_be_excluded = true;
					break;
				}
			}

			if (entity->player.team_side != team_side || !is_player_selectable(entity) ||
				should_be_excluded) {
				// If player is either:
				// - an opponent
				// - not in a state where he can be handed over
				// - is in exclusion list
				// let's skip it
				continue;
			}

			f32 distance = Vector2Distance(position, transform->position);
			if (distance < selected_distance) {
				selected_entity = entity;
				selected_distance = distance;
			}
		}
	}
	if (!selected_entity) {
		return false;
	}

	out_nearest_player->entity = selected_entity;
	out_nearest_player->distance = selected_distance;
	return true;
}

b32 entity_player_is_trajectory_clear(v2 passer_position,
									  v2 target_position,
									  enum team_side opposing_team,
									  struct game_state *game_state)
{
	v2 pass_start = passer_position;
	v2 pass_end = target_position;
	v2 pass_direction = Vector2Normalize(Vector2Subtract(pass_end, pass_start));
	f32 pass_distance = Vector2Distance(pass_start, pass_end);

	// Check for opponents along the pass trajectory
	for (usize entity_id = 0; entity_id < game_state->entity_repository->max_slots; ++entity_id) {
		struct entity *opponent = entity_get(game_state->entity_repository, entity_id);

		if (!opponent->header.alive || opponent->header.type != ET_PLAYER ||
			opponent->player.team_side != opposing_team) {
			continue;
		}

		// Calculate opponent's distance to pass line
		v2 to_opponent = Vector2Subtract(opponent->transform.position, pass_start);
		f32 projection_length = Vector2DotProduct(to_opponent, pass_direction);

		// Only check opponents between passer and target
		if (projection_length <= 0 || projection_length >= pass_distance) {
			continue;
		}

		// Calculate perpendicular distance from opponent to pass line
		v2 projected_point =
			Vector2Add(pass_start, Vector2Scale(pass_direction, projection_length));
		f32 perp_distance = Vector2Distance(opponent->transform.position, projected_point);

		// Define interception radius (adjust as needed)
		f32 interception_radius = 20.0f;

		if (perp_distance < interception_radius) {
			return false; // Pass trajectory is blocked
		}
	}
	return true; // Pass trajectory is clear
};

b32 entity_player_find_switch_target(struct entity *ball_entity,
									 enum team_side team_side,
									 entity_id_t current_player_entity_id,
									 struct list_entity_ids exclusion_list,
									 struct entity **out_switch_target,
									 struct game_state *game_state)
{
	struct entity_repository *entity_repository = game_state->entity_repository;
	const v2 ball_position = ball_entity->transform.position;
	const b32 is_defending = entity_ball_team_in_possession(ball_entity, game_state) != team_side;

	struct entity *selected_entity = NULL;
	b32 selected_can_intercept = false;
	fg_time_t selected_time_to_ball = 0;
	struct entity *potential_switch_target_entity;
	for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
		potential_switch_target_entity = entity_get(game_state->entity_repository, entity_id);
		if (potential_switch_target_entity->header.alive &&
			potential_switch_target_entity->header.type == ET_PLAYER) {

			if (current_player_entity_id == entity_id ||
				team_side != potential_switch_target_entity->player.team_side ||
				!is_player_selectable(potential_switch_target_entity)) {
				// We don't want to hand over ourselves, an opponent, nor somebody who is busy
				continue;
			}

			struct component_transform *transform = &potential_switch_target_entity->transform;

			b32 should_be_excluded = false;
			LIST_ITERATE(excluded_entity_id, exclusion_list)
			{
				if (entity_id == excluded_entity_id) {
					should_be_excluded = true;
					break;
				}
			}
			if (should_be_excluded) {
				continue;
			}

			// A candidate the ball outruns everywhere ranks behind anyone who can intercept it
			const struct player_ball_meeting meeting = player_meeting_with_the_ball(
				potential_switch_target_entity, ball_entity, game_state);

			fg_time_t time_to_ball = meeting.time_to_ball;
			if (is_defending) {
				time_to_ball *=
					goal_side_discount(transform->position, ball_position, team_side, game_state);
			}

			if (!selected_entity || (meeting.can_intercept && !selected_can_intercept) ||
				(meeting.can_intercept == selected_can_intercept &&
				 time_to_ball < selected_time_to_ball)) {
				selected_entity = potential_switch_target_entity;
				selected_can_intercept = meeting.can_intercept;
				selected_time_to_ball = time_to_ball;
			}
		}
	}
	if (selected_entity) {
		GLOGF_TRACE(LS_GAME,
					"selected switch target %zu, %.2fs from the ball%s",
					selected_entity->header.id,
					selected_time_to_ball,
					selected_can_intercept ? "" : " (cannot intercept)");
		*out_switch_target = selected_entity;
	}
	return selected_entity != NULL;
}

b32 entity_player_should_dribble(struct entity *player_entity,
								 v2 steering,
								 struct game_state *game_state)
{
	struct component_physics *physics = &player_entity->physics;

	// Striking the ball is not a turn. The seek a charge puts on the run leaves it pointing at
	// the ball while the stick aims the kick somewhere else, and the kick frame is the first one
	// the charge no longer covers.
	if (player_entity->player.kicked_ball) {
		return false;
	}

	// A charging player seeks the ball instead of following the stick, so the stick is aiming
	// the kick, not asking for a turn.
	struct human_player *human_player =
		game_input_human_player_by_player_entity_id(game_state, player_entity->header.id);
	if (human_player) {
		const struct game_controller *game_controller =
			&game_state->input_manager->game_controllers[human_player->game_controller_id];
		if (input_charge_lock_is_held(game_controller)) {
			return false;
		}
	}

	// Turning off the ball is just running: the skid is what carrying the ball through the turn
	// costs you. Possession is too loose a test for that, since it outlives the touch by a
	// second - the ball has to be at his feet this frame.
	if (entity_ball_player_on_the_ball(game_state->vars.ball.entity, game_state) !=
		player_entity->header.id) {
		return false;
	}

	if (Vector2Length(physics->velocity) < game_state->constants.player_dribble_min_velocity) {
		return false;
	}

	if (IS_V2_ZERO(steering)) {
		return false;
	}

	f32 alignment =
		Vector2DotProduct(Vector2Normalize(physics->velocity), Vector2Normalize(steering));

	return alignment < game_state->constants.player_dribble_max_alignment;
}

enum player_dribble_turn entity_player_dribble_turn(v2 velocity, v2 steering)
{
	v2 from = Vector2Normalize(velocity);
	v2 to = Vector2Normalize(steering);

	// Which axis the turn reverses, which is not always the one the run is fastest along. -a*b
	// is positive only when a and b point opposite ways, and grows with how much of each heading
	// lies on that axis, so the higher score is the axis being braked harder. A run up and to
	// the left, cut to the right, flips x while y carries straight on: sideways, even though the
	// run is at 45 degrees. A square cut reverses nothing, scores zero on both, and falls back
	// to the axis the run leans on.
	f32 braking_x = -from.x * to.x;
	f32 braking_y = -from.y * to.y;
	b32 sideways = fabsf(braking_x - braking_y) > 0.01f ? braking_x > braking_y :
														  fabsf(from.x) > fabsf(from.y);

	// Four poses per axis: the run being braked can go either way along it, and the turn can
	// drift either way across it. The pose is drawn facing the run, so `from` alone names it.
	// The drifting axis is the one left unreversed, so both headings agree on its sign and their
	// sum reads the drift off.
	if (sideways) {
		b32 front = (from.y + to.y) > 0;
		return from.x > 0 ? (front ? PDT_LEFT_TO_RIGHT_FRONT : PDT_LEFT_TO_RIGHT_BACK) :
							(front ? PDT_RIGHT_TO_LEFT_FRONT : PDT_RIGHT_TO_LEFT_BACK);
	}

	b32 right = (from.x + to.x) > 0;
	return from.y > 0 ? (right ? PDT_BACK_TO_FRONT_RIGHT : PDT_BACK_TO_FRONT_LEFT) :
						(right ? PDT_FRONT_TO_BACK_RIGHT : PDT_FRONT_TO_BACK_LEFT);
}

void entity_player_dribble_start(struct entity *player_entity,
								 v2 steering,
								 struct game_state *game_state)
{
	struct state_machine_player_normal_data *normal_state_data =
		&player_entity->player.state_machine.state_data.normal;

	normal_state_data->dribble.active = true;
	normal_state_data->dribble.start_time = game_state->time;
	normal_state_data->dribble.steering = Vector2Normalize(steering);
	normal_state_data->dribble.turn =
		entity_player_dribble_turn(player_entity->physics.velocity, steering);

	player_entity->sprite.frame_id =
		game_state->gfx.player.dribble_frames[normal_state_data->dribble.turn];
}

void entity_player_header_start(struct entity *player_entity,
								enum header_type type,
								struct game_state *game_state)
{
	state_machine_player_fire(&player_entity->player.state_machine, PT_HEAD, game_state);
	// Set after the transition because on_enter resets the state data
	player_entity->player.state_machine.state_data.header.type = type;
}

void entity_player_header_hit(struct entity *player_entity, struct game_state *game_state)
{
	struct state_machine_player_header_data *header_data =
		&player_entity->player.state_machine.state_data.header;
	player_entity->player.last_header_time = game_state->time;
	// Once we hit the ball we can stop elevation
	header_data->elevation_completed = true;
	header_data->elevation_completion_time = game_state->time;
	header_data->hit_ball = true;
}

static void player_custom_begin_draw(struct entity *entity, struct game_state *game_state)
{
	color_t kit_color;
	color_t kit_dark_color;
	color_t socks_dark_color;
	if (entity_player_is_goalie(entity)) {
		kit_color = GOALIE_KIT_LIGHT_COLOR;
		kit_dark_color = GOALIE_KIT_DARK_COLOR;
		socks_dark_color = GOALIE_SOCKS_DARK_COLOR;
	} else {
		kit_color = entity->player.team_side == TS_HOME ? PLAYER_KIT_COLOR_HOME :
														  PLAYER_KIT_COLOR_AWAY;
		kit_dark_color = ColorBrightness(kit_color, -0.25f);
		socks_dark_color = PLAYER_SOCKS_DARK_COLOR;
	}

	// Indices 10 and 11 are only ever sampled by referee and keeper sprites respectively, so one
	// layout serves every player.
	u32 *palette = (u32[]){// 0 - Hair
						   HAIR_COLOR.r,
						   HAIR_COLOR.g,
						   HAIR_COLOR.b,
						   HAIR_COLOR.a,
						   // 1 - Socks light
						   SOCKS_LIGHT_COLOR.r,
						   SOCKS_LIGHT_COLOR.g,
						   SOCKS_LIGHT_COLOR.b,
						   SOCKS_LIGHT_COLOR.a,
						   // 2 - Skin dark
						   SKIN_DARK_COLOR.r,
						   SKIN_DARK_COLOR.g,
						   SKIN_DARK_COLOR.b,
						   SKIN_DARK_COLOR.a,
						   // 3 - Kit light
						   kit_color.r,
						   kit_color.g,
						   kit_color.b,
						   kit_color.a,
						   // 4 - Socks dark
						   socks_dark_color.r,
						   socks_dark_color.g,
						   socks_dark_color.b,
						   socks_dark_color.a,
						   // 5 - Pantaloncini
						   SHORTS_COLOR.r,
						   SHORTS_COLOR.g,
						   SHORTS_COLOR.b,
						   SHORTS_COLOR.a,
						   // 6 - Shoes
						   SHOES_COLOR.r,
						   SHOES_COLOR.g,
						   SHOES_COLOR.b,
						   SHOES_COLOR.a,
						   // 7 - Shadow
						   SHADOW_COLOR.r,
						   SHADOW_COLOR.g,
						   SHADOW_COLOR.b,
						   SHADOW_COLOR.a,
						   // 8 - Skin light
						   SKIN_LIGHT_COLOR.r,
						   SKIN_LIGHT_COLOR.g,
						   SKIN_LIGHT_COLOR.b,
						   SKIN_LIGHT_COLOR.a,
						   // 9 - Kit dark
						   kit_dark_color.r,
						   kit_dark_color.g,
						   kit_dark_color.b,
						   kit_dark_color.a,
						   // 10 - Card
						   CARD_COLOR_UNUSED.r,
						   CARD_COLOR_UNUSED.g,
						   CARD_COLOR_UNUSED.b,
						   CARD_COLOR_UNUSED.a,
						   // 11 - Gloves
						   GOALIE_GLOVES_COLOR.r,
						   GOALIE_GLOVES_COLOR.g,
						   GOALIE_GLOVES_COLOR.b,
						   GOALIE_GLOVES_COLOR.a};

	struct asset_shader *assets_shader =
		asset_shader_get_by_name(game_state->assets.shader_cache, "shader_palette");
	u32 location_idx = GetShaderLocation(assets_shader->shader, "palette");
	SetShaderValueV(assets_shader->shader, location_idx, palette, SHADER_UNIFORM_IVEC4, 12);

	BeginShaderMode(assets_shader->shader);
}

static void player_custom_after_draw(struct entity *entity, struct game_state *game_state)
{
	UNUSED(entity);
	UNUSED(game_state);

	EndShaderMode();
}

static b32 check_whether_goalie_should_dive(v3 pos_to_check,
											b32 attacking_south,
											f32 time_to_unsaveable,
											struct entity *goalie_entity,
											struct goalie_dive *dive,
											struct game_state *game_state)
{
	b32 should_dive = false;
	f32 x_diff = pos_to_check.x - goalie_entity->transform.position.x;
	f32 y_diff = pos_to_check.z - goalie_entity->transform.position.y;
	f32 distance_to_cover = fabsf(x_diff);
	if ((attacking_south ? (y_diff > 0 && y_diff < 5) : (y_diff > -5 && y_diff < 0)) &&
		// Maximum distance gk can cover with a dive is half the goal width + 10%
		distance_to_cover < (game_state->vars.pitch.goal_width * 0.5f * 1.1f) &&
		pos_to_check.y < game_state->constants.crossbar_height) {
		f32 max_distance_we_can_cover_in_available_time =
			game_state->constants.goalie_dive_max_velocity * time_to_unsaveable / 1.0f;
		dive->direction = x_diff > 0 ? SD_RIGHT : SD_LEFT;
		dive->height = pos_to_check.y < GOALIE_DIVE_HEIGHT_THRESHOLD ? SH_LOW : SH_HIGH;
		dive->speed = Clamp(distance_to_cover / max_distance_we_can_cover_in_available_time, 0, 1);
		dive->distance = distance_to_cover;
		GLOGF_DEBUG_CTX(LS_GAME,
						goalie_entity->header.name,
						"goalie has %.3fs to save a ball that will be %.2fpx away: "
						"dive speed %.2fpx/s",
						time_to_unsaveable,
						distance_to_cover,
						dive->speed * game_state->constants.goalie_dive_max_velocity);
		should_dive = true;
	}
	return should_dive;
}

struct entity *entity_player_select_press_player(struct entity *ball_entity,
												 enum team_side team_side,
												 f32 max_distance,
												 struct game_state *game_state)
{
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *selected_entity = NULL;
	b32 selected_can_intercept = false;
	fg_time_t selected_time_to_ball = 0;
	struct entity *entity;

	const b32 team_has_humans = game_human_players_count_by_team(game_state, team_side) > 0;

	for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
		entity = entity_get(game_state->entity_repository, entity_id);

		// A pure AI team fields a second chaser alongside the one nearest to the ball. A team
		// with humans gets a single chaser, so the presser is free to be the nearest one.
		if (!entity->header.alive || entity->header.type != ET_PLAYER ||
			entity->player.state_machine.current_state != PS_NORMAL ||
			entity_player_is_goalie(entity) || entity->player.team_side != team_side ||
			(!team_has_humans &&
			 entity_id ==
				 game_state->vars.teams[team_side].play_context.player_nearest_to_the_ball)) {
			continue;
		}

		b32 is_player_human_controlled = false;
		for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
			if (game_state->human_players[i].enabled &&
				game_state->human_players[i].controlled_player.entity_id == entity_id) {
				is_player_human_controlled = true;
				break;
			}
		}
		if (is_player_human_controlled) {
			continue;
		}

		const struct player_ball_meeting meeting =
			player_meeting_with_the_ball(entity, ball_entity, game_state);

		// Nobody sets off after a ball that never comes near him
		if (meeting.distance_to_ball_path > max_distance) {
			continue;
		}

		// The pressing team is the one without the ball, so the goal side discount always applies:
		// of two players equally quick to the ball we want the one also covering our goal.
		const fg_time_t time_to_ball =
			meeting.time_to_ball *
			goal_side_discount(
				entity->transform.position, ball_entity->transform.position, team_side, game_state);

		if (!selected_entity || (meeting.can_intercept && !selected_can_intercept) ||
			(meeting.can_intercept == selected_can_intercept &&
			 time_to_ball < selected_time_to_ball)) {
			selected_entity = entity;
			selected_can_intercept = meeting.can_intercept;
			selected_time_to_ball = time_to_ball;
		}
	}
	if (selected_entity != NULL) {
		game_state->vars.teams[team_side].play_context.player_in_charge_of_pressing =
			selected_entity->header.id;
		GLOGF_TRACE(LS_GAME,
					"selected press player %zu, %.2fs from the ball%s",
					selected_entity->header.id,
					selected_time_to_ball,
					selected_can_intercept ? "" : " (cannot intercept)");
	}
	return selected_entity;
}

// Whether a player can be handed over to a human: nobody busy doing something else (tackling,
// heading, falling), and no goalie unless he is holding the ball.
static b32 is_player_selectable(struct entity *player_entity)
{
	const struct component_player *player = &player_entity->player;
	const struct component_goalie *goalie = &player_entity->goalie;

	// A keeper is handed over only with the ball in his hands. Out of goal he is in PS_NORMAL and
	// selectable like anybody else.
	if (player->state_machine.current_state == PS_GOALIE) {
		return goalie->state_machine.current_state == GS_HOLD_BALL;
	}

	return player->state_machine.current_state == PS_NORMAL;
}

// The speed is the conservative one the AI seek uses, so both agree on what is reachable.
static struct player_ball_meeting player_meeting_with_the_ball(struct entity *player_entity,
															   struct entity *ball_entity,
															   struct game_state *game_state)
{
	const v3 *ball_future_positions = game_state->vars.ball.lookahead_positions;
	const u32 lookaheads = game_state->constants.ball_lookaheads;
	const u32 steps_per_lookahead = game_state->constants.ball_lookahead_dts;

	const v2 player_position = player_entity->transform.position;
	const f32 max_speed = player_entity->player.constants.run_max_velocity * 0.8f;

	// Lookahead positions are only predicted while the ball is in play. Anywhere else (throw in,
	// keeper holding it) the ball stands still, so every predicted position is the one it is on
	// now and the whole window comes down to how long he takes to run there.
	if (ball_entity->ball.state_machine.current_state != BS_IN_PLAY) {
		const f32 distance = Vector2Distance(player_position, ball_entity->transform.position);
		const fg_time_t time_to_reach = distance / max_speed;
		const fg_time_t last_arrival_time = lookaheads * game_state->dt * steps_per_lookahead;

		return (struct player_ball_meeting){.can_intercept = time_to_reach <= last_arrival_time,
											.time_to_ball = time_to_reach,
											.distance_to_ball_path = distance};
	}

	struct player_ball_meeting meeting = {
		.can_intercept = false, .time_to_ball = FLT_MAX, .distance_to_ball_path = FLT_MAX};

	for (u32 i = 0; i < lookaheads; ++i) {
		const v2 future_position = V3_TO_V2(ball_future_positions[i]);
		const f32 distance = Vector2Distance(player_position, future_position);
		const fg_time_t time_to_reach = distance / max_speed;
		const fg_time_t ball_arrival_time = (i + 1) * game_state->dt * steps_per_lookahead;

		meeting.distance_to_ball_path = MIN(meeting.distance_to_ball_path, distance);

		if (time_to_reach <= ball_arrival_time) {
			meeting.can_intercept = true;
			meeting.time_to_ball = time_to_reach;
			break;
		}
		meeting.time_to_ball = MIN(meeting.time_to_ball, time_to_reach);
	}
	return meeting;
}

// Defending, the team mates standing between the ball and our goal are the ones worth having, so
// their time to the ball is discounted. A discount rather than a filter: being goal side wins a
// close call, it doesn't rule out whoever is plainly quicker to the ball.
static f32 goal_side_discount(v2 player_position,
							  v2 ball_position,
							  enum team_side team_side,
							  struct game_state *game_state)
{
	const enum pitch_side defended_side = game_state->vars.teams[team_side].play_context.pitch_side;
	const v2 ball_to_own_goal = Vector2Normalize(
		Vector2Subtract(game_state->vars.pitch.goal[defended_side].center, ball_position));
	const f32 goal_side = Vector2DotProduct(
		ball_to_own_goal, Vector2Normalize(Vector2Subtract(player_position, ball_position)));

	return 1 - Clamp01(goal_side) * game_state->constants.player_goal_side_bonus;
}
