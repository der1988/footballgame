#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>

void systems_player(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentForEach(entity_repository, base_component, idx) {
        Component *base_transform =
            entities_component_get(game_state->entity_repository, CT_TRANSFORM, idx);
        Component *base_physics =
            entities_component_get(game_state->entity_repository, CT_PHYSICS, idx);
        Component *base_human =
            entities_component_get(game_state->entity_repository, CT_HUMAN, idx);
        Component *base_collider =
            entities_component_get(game_state->entity_repository, CT_HUMAN, idx);
        Component *base_state_machine =
            entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, idx);
        if (entity_repository->entities[idx].alive && base_transform->header.enabled &&
            base_physics->header.enabled && base_human->header.enabled &&
            base_collider->header.enabled && base_state_machine->header.enabled) {

            ComponentPhysics *physics = &base_physics->physics;

            StateMachinePlayer *state_machine = base_state_machine->state_machine.ptr;

            Vector2 steering = Vector2Normalize(input->left_stick);
            Vector2 force_to_apply = Vector2Zero();

            switch (state_machine->current_state) {
            case PS_NONE:
                break;
            case PS_STAND:
                if (Vector2Length(steering) > 0.05f) {
                    entities_player_state_machine_change_state_to(state_machine, PS_RUN,
                                                                  game_state);
                }
                break;
            case PS_RUN: {
                if (Vector2Length(steering) < 0.05f) {
                    steering = Vector2Zero();
                    entities_player_state_machine_change_state_to(state_machine, PS_STAND,
                                                                  game_state);
                } else {
                    // Calculate animation to play
                    PlayerDirection pd = entities_player_calculate_direction(steering);
                    if (state_machine->state_data.run.running_action_direction != pd) {
                        actions_action_stop(state_machine->state_data.run.running_action_idx, idx,
                                            game_state);
                        state_machine->state_data.run.running_action_idx = components_action_run(
                            idx, base_human->human.run_actions[pd], game_state);
                        assert(state_machine->state_data.run.running_action_idx >= 0);
                        state_machine->state_data.run.running_action_direction = pd;
                    }
                }

                // Run force
                force_to_apply = Vector2Scale(steering, 1500);
            } break;
            case PS_COUNT:
                break;
            }

            if (Vector2Length(physics->velocity) > 5) {
                // Drag force
                Vector2 drag =
                    Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)), 1000);
                force_to_apply = Vector2Add(force_to_apply, drag);
            } else {
                physics->velocity = Vector2Zero();
            }

            entities_player_apply_force(force_to_apply, physics, &base_human->human,
                                        game_state->dt);
            entities_player_apply_velocity(&base_transform->transform, physics, game_state->dt);
        }
    }
}
