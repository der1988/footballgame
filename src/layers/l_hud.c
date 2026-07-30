#include "game/game.h"
#include "input/input.h"
#include "log/log.h"
#include "math_ext/math_ext.h"
#include "macro.h"
#include "entities/entities.h"
#include "layers/internal/layers.h"
#include "layers/layers.h"

layer_hud_t *layer_hud_create(struct game_state *game_state)
{
	layer_hud_t *handle =
		memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(layer_hud_t));
	assert(handle);
	handle->initialised = true;

	handle->radar_position = (v2){.x = 10, .y = 10};

	GLOG_INFO(LS_LAYERS, "layer hud initialised");

	return handle;
}

void layer_hud_destroy(layer_hud_t *handle)
{
	assert(handle->initialised);

	handle->initialised = false;

	GLOG_INFO(LS_LAYERS, "layer hud destroyed");
}

static inline v2 to_radar_position(v2 pos, rect_t pitch_rect, v2 radar_pos, f32 ratio)
{
	v2 radar_relative_pos = Vector2Subtract(pos, (v2){.x = pitch_rect.x, .y = pitch_rect.y});
	radar_relative_pos = Vector2Scale(radar_relative_pos, ratio);
	return Vector2Add(radar_relative_pos, radar_pos);
}

void layer_hud_draw(layer_hud_t *handle, struct game_state *game_state)
{
	f32 section_width = game_state->config.screen_width / 4.0f;
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		struct human_player *hp = &game_state->human_players[i];
		if (!hp->enabled)
			continue;

		struct game_controller *gc =
			&game_state->input_manager->game_controllers[hp->game_controller_id];

		f32 power = 0;

		if (gc->charge_lock.state == INPUT_CHARGE_LOCK_CHARGING ||
			gc->charge_lock.state == INPUT_CHARGE_LOCK_CHARGED) {
			fg_time_t current_duration = gc->charge_lock.end_time - gc->charge_lock.start_time;
			switch (gc->charge_lock.control_id) {
			case GAME_CONTROL_SHOOT:
				power = SHOOT_POWER(current_duration);
				break;
			case GAME_CONTROL_AUTOMATIC_PASS:
			case GAME_CONTROL_PASS:
				power = PASS_POWER(current_duration);
				break;
			case GAME_CONTROL_LONG_PASS:
				power = LONG_PASS_POWER(current_duration);
				break;
			default:
				break;
			}
		}

		f32 bar_max_width = section_width - 20;
		rect_t charge_box = {.x = section_width * i + 10,
							 .y = game_state->config.screen_height - 100,
							 .width = bar_max_width * power,
							 .height = 20};
		DrawRectangleRec(charge_box, power > 0.8f ? RED : WHITE);
	}

	// Radar
	if (game_state->constants.show_radar) {
		f32 radar_ratio = 0.3f;
		rect_t radar_rect = (rect_t){.width = game_state->vars.pitch.field.width * radar_ratio,
									 .height = game_state->vars.pitch.field.height * radar_ratio,
									 .x = handle->radar_position.x,
									 .y = handle->radar_position.y};

		DrawRectangleLinesEx(radar_rect, 1.0f, WHITE);

		struct entity *ball_entity = game_state->vars.ball.entity;

		DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
		UNUSED(ball_physics_3d);
		UNUSED(ball);
		UNUSED(ball_collider);

		DrawCircleV(to_radar_position(ball_transform->position,
									  game_state->vars.pitch.field,
									  handle->radar_position,
									  radar_ratio),
					3,
					WHITE);

		FOR_EACH_ENTITY(entity, game_state->entity_repository)
		{
			if (entity->header.type != ET_PLAYER) {
				continue;
			}

			DECLARE_ENTITY_COMPONENT(entity, transform);

			DrawRectangleV(to_radar_position(transform->position,
											 game_state->vars.pitch.field,
											 handle->radar_position,
											 radar_ratio),
						   (v2){.x = 6, .y = 4},
						   entity->player.team_side == TS_HOME ? RED : BLUE);

			if (game_input_human_player_by_player_entity_id(game_state, entity->header.id)) {
				v2 pos = to_radar_position(transform->position,
										   game_state->vars.pitch.field,
										   handle->radar_position,
										   radar_ratio);
				rect_t player_selection_rect =
					(rect_t){.x = pos.x, .y = pos.y, .width = 7, .height = 5};
				DrawRectangleLinesEx(player_selection_rect, 1, WHITE);
			}
		}
	}
}
