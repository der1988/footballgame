#ifndef DEBUGUI_INTERNAL_H
#define DEBUGUI_INTERNAL_H

#include "types.h"

#define ENTITY_INSPECTOR_FILTER_BUFFER_SIZE 64

// Forward declaration
struct game_state;

struct debugui {
	b32 initialised;
	v2 window_size;
	entity_id_t entity_inspector_selected_entity_id;
	char entity_inspector_filter_buffer[ENTITY_INSPECTOR_FILTER_BUFFER_SIZE];
};

#endif
