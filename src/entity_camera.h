#ifndef ENTITY_CAMERA_H
#define ENTITY_CAMERA_H

struct game_state;
struct entity;

struct entity *entity_camera_create(struct game_state *game_state);
void entity_camera_follow_target(struct entity *camera_entity, struct game_state *game_state);

#endif // !ENTITY_CAMERA_H
