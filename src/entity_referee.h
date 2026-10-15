#ifndef ENTITY_REFEREE_H
#define ENTITY_REFEREE_H

struct entity;
struct game_state;

struct entity *entity_referee_create(struct game_state *game_state);

#endif
