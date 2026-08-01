#include "system.h"
#include "game.h"
#include "input.h"
#include "time_machine.h"

void system_time_machine(struct game_state *game_state)
{
	time_machine_save_capsule(game_state->time_machine, game_state);
}
