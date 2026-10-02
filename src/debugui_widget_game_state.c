#include "macro.h"
#include "debugui.h"
#include "debugui_internal.h"
#include "entity.h"
#include "entity_ball.h"
#include "football.h"
#include "game.h"
#include "gameplay.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

static const char *entity_label(struct game_state *game_state, entity_id_t id)
{
	if (id == EID_NOT_VALID) {
		return "(invalid)";
	}
	struct entity *entity = entity_get(game_state->entity_repository, id);
	return TextFormat("%u %s", id, entity ? entity->header.name : "?");
}

void debugui_game_state_draw(debugui_t *handle, struct game_state *game_state)
{
	ASSERT(handle->initialised);

	igSetNextWindowSize((ImVec2){handle->window_size.x, handle->window_size.y},
						ImGuiCond_FirstUseEver);

	igBegin("Game State", NULL, 0);

	if (igBeginTabBar("TabBar", 0)) {
		if (igBeginTabItem("Clock", NULL, 0)) {
			igText(TextFormat("time:            %.3f", game_state->time));
			igText(TextFormat("current_time:    %.3f", game_state->current_time));
			igText(TextFormat("dt:              %.4f", game_state->dt));
			igText("accumulator:     %.4f  (%.0f%% of fixed_timestep %.4f)",
				   game_state->accumulator,
				   game_state->accumulator / game_state->constants.fixed_timestep * 100.0,
				   game_state->constants.fixed_timestep);
			igText(TextFormat("physics_updates: %llu",
							  (unsigned long long)game_state->physics_updates));
			igText(
				TextFormat("frame_updates:   %llu", (unsigned long long)game_state->frame_updates));
			igEndTabItem();
		}
		if (igBeginTabItem("Gameplay", NULL, 0)) {
			struct gameplay *gameplay = &game_state->gameplay;
			struct state_machine_football *state_machine = &gameplay->state_machine;

			igText(TextFormat("mode:                  %s", gameplay_mode_string[gameplay->mode]));
			igText(TextFormat("state:                 %s",
							  state_machine_football_state_string[state_machine->current_state]));
			igText(TextFormat("prev_state:            %s",
							  state_machine_football_state_string[state_machine->previous_state]));
			igText(
				TextFormat("goal_celebration_time: %.2f", gameplay->config.goal_celebration_time));
			if (state_machine->current_state == FS_GOAL) {
				fg_time_t start_time = state_machine->state_data.goal.start_time;
				igText(TextFormat("goal_start_time:       %s",
								  debugui_time_string(start_time, game_state)));
				igText(TextFormat("celebration:           %.2f / %.2f",
								  game_state->time - start_time,
								  gameplay->config.goal_celebration_time));
			}

			igSeparator();

			switch (gameplay->mode) {
			case GPM_MATCH:
				igText(TextFormat("score:                 %u - %u",
								  gameplay->data.match.score.home,
								  gameplay->data.match.score.away));
				igText(TextFormat("celebration_time:      %.2f",
								  gameplay->data.match.celebration_time));
				break;
			case GPM_SANDBOX:
				igText(TextFormat("restart_delay:         %.2f",
								  gameplay->data.sandbox.restart_delay));
				break;
			case GPM_COUNT:
				break;
			}
			igEndTabItem();
		}
		if (igBeginTabItem("Teams", NULL, 0)) {
			for (i32 i = 0; i < TS_COUNT; i++) {
				struct team_play_context *play_context = &game_state->vars.teams[i].play_context;
				igText(TextFormat("%s", team_side_to_string[i]));
				igText(TextFormat("  pitch_side:         %s",
								  pitch_side_to_string[play_context->pitch_side]));
				igText(TextFormat("  formation:          %s",
								  team_formation_to_string[play_context->formation]));
				igText(
					TextFormat("  nearest_to_ball:    %s",
							   entity_label(game_state, play_context->player_nearest_to_the_ball)));
				igText(TextFormat("  in_charge_of_press: %s",
								  entity_label(game_state,
											   play_context->player_in_charge_of_pressing)));
				igSeparator();
			}

			struct ball_possession *possession = &game_state->vars.ball.possession_at_tick_start;
			igText("possession_at_tick_start");
			igText(TextFormat("  type:               %s",
							  ball_possession_type_string[possession->type]));
			igText(TextFormat("  entity:             %s",
							  entity_label(game_state, possession->entity_id)));
			igEndTabItem();
		}
		if (igBeginTabItem("Humans", NULL, 0)) {
			b32 any = false;

			for (i32 i = 0; i < MAX_HUMAN_PLAYERS; i++) {
				struct human_player *human_player = &game_state->human_players[i];
				if (!human_player->enabled) {
					continue;
				}
				any = true;
				fg_time_t last_switch_time = human_player->controlled_player.last_switch_time;
				igText(TextFormat("human %d", i));
				igText(TextFormat("  team_side:          %s",
								  team_side_to_string[human_player->team_side]));
				igText(TextFormat("  game_controller_id: %u", human_player->game_controller_id));
				igText(TextFormat("  controlled_player:  %s",
								  entity_label(game_state,
											   human_player->controlled_player.entity_id)));
				igText(TextFormat("  manually_selected:  %s",
								  human_player->controlled_player.is_manually_selected ? "yes" :
																						 "no"));
				igText(TextFormat("  last_switch_time:   %s",
								  debugui_time_string(last_switch_time, game_state)));
				if (last_switch_time != 0) {
					igText(TextFormat("  switch_grace:       %.2f / %.2f",
									  game_state->time - last_switch_time,
									  (double)game_state->constants.player_switch_grace_time));
				}
				igSeparator();
			}

			if (!any) {
				igText("No human players.");
			}
			igEndTabItem();
		}
		igEndTabBar();
	}

	igEnd();
}
