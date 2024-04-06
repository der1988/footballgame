#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "assets.h"
#include "input.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

void systems_player(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentForEach(entity_repository, base_component, idx) {
        Component *base_transform =
            entities_component_get(game_state->entity_repository, CT_TRANSFORM, idx);
        Component *base_physics =
            entities_component_get(game_state->entity_repository, CT_PHYSICS, idx);
        Component *base_sprite =
            entities_component_get(game_state->entity_repository, CT_SPRITE, idx);
        Component *base_human =
            entities_component_get(game_state->entity_repository, CT_HUMAN, idx);
        Component *base_collider =
            entities_component_get(game_state->entity_repository, CT_COLLIDER, idx);
        Component *base_state_machine =
            entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, idx);
        if (entity_repository->entities[idx].alive && base_transform->header.enabled &&
            base_physics->header.enabled && base_human->header.enabled &&
            base_collider->header.enabled && base_state_machine->header.enabled &&
            base_sprite->header.enabled) {

            ComponentPhysics *physics = &base_physics->physics;
            ComponentSprite *sprite = &base_sprite->sprite;
            ComponentHuman *human = &base_human->human;

            StateMachinePlayer *state_machine = base_state_machine->state_machine.ptr;

            Vector2 steering = Vector2ClampValue(input->left_stick, -1, 1);
            Vector2 force_to_apply = Vector2Zero();

            switch (state_machine->current_state) {
            case PS_NONE:
                break;
            case PS_RUN: {
                f32 sprint = (input->right_trigger + 1) / 2.0f;
                physics->max_velocity = game_state->constants.player_control_max_velocity +
                                        (game_state->constants.player_run_max_velocity -
                                         game_state->constants.player_control_max_velocity) *
                                            sprint;

                // Run force
                force_to_apply = Vector2Scale(Vector2Normalize(steering),
                                              game_state->constants.player_movement_force);

                if (Vector2Length(physics->velocity) > 5) {
                    // Drag force
                    Vector2 drag = Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
                                                game_state->constants.player_movement_drag);
                    force_to_apply = Vector2Add(force_to_apply, drag);
                } else {
                    physics->velocity = Vector2Zero();
                }

                entities_player_apply_force(force_to_apply, physics, game_state->dt);
                entities_player_apply_velocity(&base_transform->transform, physics, game_state->dt);

                // Calculate frame to display
                if (Vector2Length(physics->velocity) > 0.01f) {
                    PlayerDirection pd = entities_player_calculate_direction(physics->velocity);
                    AssetsAnimation *animation = assets_animation_get_by_id(
                        game_state->animation_cache, base_human->human.run_animations[pd]);
                    f32 max_player_velocity = game_state->constants.player_run_max_velocity;
                    if ((game_state->time - human->last_frame_update_time) >
                        (animation->delay *
                         (2 - Vector2Length(physics->velocity) / max_player_velocity))) {
                        human->last_frame_update_time = game_state->time;
                        u16 new_frame_id =
                            (human->last_running_frame_idx + 1) % animation->frames_count;
                        sprite->frame_id = animation->frames[new_frame_id];
                        human->last_running_frame_idx = new_frame_id;
                    }
                    human->last_direction = pd;
                } else {
                    sprite->frame_id = human->stand_frames[human->last_direction];
                }

                break;
            }
            case PS_COUNT:
                break;
            }
        }
    }
}
