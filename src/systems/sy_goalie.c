#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "assets.h"
#include "input.h"
#include "layers/layers.h"
#include "memory.h"
#include "steering.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

void systems_goalie(GameState *game_state, Input *input) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentTransform *ball_transform = entities_component_get_typed(
        game_state->entity_repository, CT_TRANSFORM, game_state->vars.ball_entity_id);
    ComponentPhysics3d *ball_physics_3d = entities_component_get_typed(
        game_state->entity_repository, CT_PHYSICS_3D, game_state->vars.ball_entity_id);
    ComponentCollider *ball_collider = entities_component_get_typed(
        game_state->entity_repository, CT_COLLIDER, game_state->vars.ball_entity_id);
    ComponentBall *ball = entities_component_get_typed(game_state->entity_repository, CT_BALL,
                                                       game_state->vars.ball_entity_id);

    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(entity_repository, entity_id, 5,
                                                        CT_TRANSFORM, CT_PHYSICS, CT_SPRITE,
                                                        CT_GOALIE, CT_COLLIDER)) {

            ComponentTransform *transform = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_TRANSFORM, entity_id);
            ComponentPhysics *physics =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_PHYSICS, entity_id);
            ComponentSprite *sprite =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_SPRITE, entity_id);
            ComponentPlayer *player =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_PLAYER, entity_id);
            ComponentGoalie *goalie =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_GOALIE, entity_id);
            ComponentCollider *collider = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_COLLIDER, entity_id);

            StateMachineGoalie *state_machine = goalie->state_machine;

            v2 goal_center = {.x = game_state->vars.pitch_rectangle.x +
                                   game_state->vars.pitch_rectangle.width * 0.5f,
                              .y = 80};

            f32 distance_from_ball =
                Vector3Distance(V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
                                V2_TO_V3(transform->position, 0));

            f32 distance_from_goal =
                Vector3Distance(V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
                                V2_TO_V3(goal_center, 0));

            collider->colliders[0].props_circle.radius =
                game_state->constants.player_control_radius;

            b32 is_colliding_with_ball =
                CheckCollisionCircles(
                    transform->position, collider->colliders[1].props_circle.radius,
                    ball_transform->position, ball_collider->colliders[0].props_circle.radius) &&
                // Player height
                ball_transform->virtual_height < game_state->constants.player_control_height;

            if (player->state_machine->current_state == PS_GOALIE) {
                switch (state_machine->current_state) {
                case GS_NONE:
                    break;
                case GS_PROTECT_GOAL: {
                    static f64 last_save_time;
                    if (HAS_TIME_PASSED(game_state->time, last_save_time, 0.3f) &&
                        is_colliding_with_ball) {
                        v3 new_velocity = Vector3Negate(ball_physics_3d->velocity);
                        entities_ball_set_position_and_velocity(
                            game_state->vars.ball_entity_id,
                            V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
                            new_velocity, game_state);
                        last_save_time = game_state->time;
                    }

                    v2 direction =
                        Vector2Normalize(Vector2Subtract(ball_transform->position, goal_center));
                    v2 position = Vector2Add(goal_center, Vector2Scale(direction, 40));

                    v2 steering = steering_seek(transform->position, physics->velocity,
                                                physics->max_velocity, position);
                    // Run force
                    v2 force_to_apply =
                        Vector2Scale(steering, game_state->constants.player_movement_force);

                    entities_player_apply_force(entity_id, force_to_apply, game_state);
                    entities_player_apply_velocity(entity_id, game_state);

                    if (distance_from_goal < 100 && ball_transform->virtual_height < 20) {
                        entities_goalie_state_machine_change_state_to(goalie->state_machine,
                                                                      GS_SEEK_BALL, game_state);
                    }

                    break;
                }
                case GS_SAVE: {
                    break;
                }
                case GS_SEEK_BALL: {
                    v2 steering = steering_seek(transform->position, physics->velocity,
                                                physics->max_velocity, ball_transform->position);
                    // Run force
                    v2 force_to_apply =
                        Vector2Scale(steering, game_state->constants.player_movement_force);

                    entities_player_apply_force(entity_id, force_to_apply, game_state);
                    entities_player_apply_velocity(entity_id, game_state);

                    if (distance_from_goal > 100) {
                        entities_goalie_state_machine_change_state_to(goalie->state_machine,
                                                                      GS_PROTECT_GOAL, game_state);
                    }

                    break;
                }
                case GS_COUNT:
                    break;
                }
            }

            v2 direction =
                Vector2Normalize(Vector2Subtract(ball_transform->position, transform->position));
            entities_player_calculate_running_frame(entity_id, direction, 5, game_state);
        }
    }
}
