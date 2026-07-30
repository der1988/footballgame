#include "game/game.h"
#include "input/input.h"
#include "time_machine/time_machine.h"

void systems_time_machine(struct game_state *game_state)
{
	time_machine_save_capsule(game_state->time_machine, game_state);
}
