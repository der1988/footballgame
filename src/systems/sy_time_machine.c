#include "game.h"
#include "input.h"
#include "time_machine/time_machine.h"

void systems_time_machine(struct game_state *game_state, struct input *input)
{
	static u64 last_frame = 0;
	if (game_state->frame_updates != last_frame) {
		if (input->devices[0].left_trigger > 0.1f) {
			time_machine_rewind(game_state->time_machine, 1, game_state);
		} else {
			if (game_state->frame_updates % 2 == 0) {
				time_machine_save_capsule(game_state->time_machine, game_state);
			}
		}
		last_frame = game_state->frame_updates;
	}
}
