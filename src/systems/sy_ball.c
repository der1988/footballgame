#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../utils/utils.h"
#include "input.h"
#include <raylib.h>
#include <raymath.h>

void systems_ball(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    EntityId ball_id = game_state->vars.ball_entity_id;
    if (entity_repository->entities[ball_id].alive) {
        ComponentTransform *transform =
            &entities_component_get(game_state->entity_repository, CT_TRANSFORM, ball_id)
                 ->transform;
        ComponentPhysics3d *physics_3d =
            &entities_component_get(game_state->entity_repository, CT_PHYSICS_3D, ball_id)
                 ->physics_3d;
        ComponentCollider *collider =
            &entities_component_get(game_state->entity_repository, CT_COLLIDER, ball_id)->collider;
        ComponentSprite *sprite =
            &entities_component_get(game_state->entity_repository, CT_SPRITE, ball_id)->sprite;
        ComponentBall *ball =
            &entities_component_get(game_state->entity_repository, CT_BALL, ball_id)->ball;
        Component *component =
            entities_component_get(game_state->entity_repository, CT_STATE_MACHINE, ball_id);
        StateMachineBall *state_machine = component->state_machine.ptr;

        if (input->buttons[IBT_BUTTON_START].pressed) {
            ComponentTransform *p1_transform =
                &entities_component_get(game_state->entity_repository, CT_TRANSFORM,
                                        game_state->vars.p1_entity_id)
                     ->transform;
            transform->position = p1_transform->position;
            transform->virtual_height = 0;
            physics_3d->accel = Vector3Zero();
            physics_3d->velocity = Vector3Zero();
            return;
        }

        switch (state_machine->current_state) {
        case BS_NONE:
            break;
        case BS_IN_PLAY: {

            Vector3 ball_position_3d = V2_TO_V3(transform->position, transform->virtual_height);
            if (ball_position_3d.y > 0) {
                // Gravity
                physics_3d->velocity = Vector3Add(
                    physics_3d->velocity, Vector3Scale(game_state->constants.gravity_force, 1));
                // Drag in the air
                physics_3d->velocity.x += physics_3d->velocity.x *
                                          -game_state->constants.ball_drag_in_air * game_state->dt;
                physics_3d->velocity.z += physics_3d->velocity.z *
                                          -game_state->constants.ball_drag_in_air * game_state->dt;
            } else {
                // Drag on the pitch
                physics_3d->velocity.x += physics_3d->velocity.x *
                                          -game_state->constants.ball_drag_on_pitch *
                                          game_state->dt;
                physics_3d->velocity.z += physics_3d->velocity.z *
                                          -game_state->constants.ball_drag_on_pitch *
                                          game_state->dt;
            }

            // Apply velocity to ball
            ball_position_3d =
                Vector3Add(ball_position_3d, Vector3Scale(physics_3d->velocity, game_state->dt));

            // Make the ball bounce
            if (ball_position_3d.y < 0) {
                ball_position_3d.y = 0;
                physics_3d->velocity.y *= -game_state->constants.ball_cr;
                if (fabsf(physics_3d->velocity.y) < 10.0f) {
                    physics_3d->velocity.y = 0;
                }
            }

            // Project position to 2D
            transform->position = V3_TO_V2(ball_position_3d);
            transform->virtual_height = ball_position_3d.y;

            // This has to be done after all forces have been applied
            if (transform->virtual_height == 0) {
                ball->subject_to_after_touch = false;
            }
        } break;
        case BS_OUT_OF_PLAY: {
        } break;
        case BS_COUNT:
            break;
        }
    }
}
