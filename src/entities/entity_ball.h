#ifndef ENTITIES_BALL_H
#define ENTITIES_BALL_H

#include "components.h"
#include "entities/state_machine.h"
#include "types.h"

struct entity;

struct ball_config {
	v2 position;
};

enum state_machine_ball_state {
	BS_NONE,
	BS_IN_PLAY,
	BS_OUT_OF_PLAY,
	BS_GK_HANDS,
	BS_COUNT
};

static const char *state_machine_ball_state_string[] = {
	"none", "in_play", "out_of_play", "gk_hands", "count"};

declare_state_machine_enter_and_exit(ball);

union state_machine_ball_state_data {
	struct state_machine_default_empty_data in_play;
	struct state_machine_default_empty_data out_of_play;
	struct state_machine_default_empty_data gk_hands;
};

define_state_machine(ball);
define_state_machine_functions(ball);

struct entity *ball_create(struct ball_config config, struct game_state *game_state);
void ball_set_position_and_velocity(struct entity *ball_entity, v3 pos, v3 velocity);
void ball_control_precision(struct entity *ball_entity,
							v2 player_velocity,
							v2 position_to_seek,
							f32 difficulty,
							struct game_state *game_state);
void ball_control_running(struct entity *ball_entity,
						  v2 control_direction,
						  v2 player_velocity,
						  f32 difficulty,
						  struct game_state *game_state);
void ball_shoot(struct entity *ball_entity,
				entity_id_t kicker_entity_id,
				v2 direction,
				f32 height,
				f32 power,
				struct game_state *game_state);
void ball_short_pass(struct entity *ball_entity,
					 entity_id_t kicker_entity_id,
					 v3 direction,
					 f32 power,
					 b32 auto_pass,
					 struct game_state *game_state);
void ball_long_pass(struct entity *ball_entity,
					entity_id_t kicker_entity_id,
					v2 direction,
					f32 power,
					struct game_state *game_state);
void ball_tackle(struct entity *ball_entity,
				 entity_id_t tackling_entity_id,
				 v3 direction,
				 f32 power,
				 struct game_state *game_state);
void ball_deviate_trajectory(struct entity *ball_entity,
							 entity_id_t player_entity_id,
							 struct game_state *game_state);
void ball_held_by_goalie(struct entity *ball_entity,
						 entity_id_t goalie_entity_id,
						 struct game_state *game_state);
void ball_saved_by_goalie(struct entity *ball_entity,
						  entity_id_t goalie_entity_id,
						  struct game_state *game_state);
void ball_back_in_play(struct entity *ball_entity, struct game_state *game_state);
void ball_in_goal(struct entity *ball_entity);
void ball_set_last_touch(struct component_ball *ball,
						 entity_id_t player_on_the_ball_entity_id,
						 struct game_state *game_state);
entity_id_t ball_player_in_possession(struct entity *ball_entity, struct game_state *game_state);
entity_id_t ball_player_on_the_ball(struct entity *ball_entity, struct game_state *game_state);

#endif // !ENTITIES_BALL_H
