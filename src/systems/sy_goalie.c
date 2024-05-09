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

            collider->colliders[0].props_circle.radius =
                game_state->constants.player_control_radius;

            switch (state_machine->current_state) {
            case GS_NONE:
                break;
            case GS_PROTECT_GOAL: {
                if (player->state_machine->current_state == PS_GOALIE) {

                    b32 is_colliding_with_ball =
                        CheckCollisionCircles(transform->position,
                                              collider->colliders[1].props_circle.radius,
                                              ball_transform->position,
                                              ball_collider->colliders[0].props_circle.radius) &&
                        // Player height
                        ball_transform->virtual_height <
                            game_state->constants.player_control_height;

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

                    v2 goal_center = {.x = game_state->vars.pitch_rectangle.x +
                                           game_state->vars.pitch_rectangle.width * 0.5f,
                                      .y = 80};
                    v2 direction =
                        Vector2Normalize(Vector2Subtract(ball_transform->position, goal_center));
                    v2 position = Vector2Add(goal_center, Vector2Scale(direction, 40));

                    v2 steering = steering_seek(transform->position, physics->velocity,
                                                physics->max_velocity, position);
                    // Run force
                    v2 force_to_apply =
                        Vector2Scale(steering, game_state->constants.player_movement_force);

                    entities_player_apply_force(force_to_apply, physics, game_state->dt);
                    entities_player_apply_velocity(transform, physics, game_state->dt);

                    // Calculate frame to display
                    if (Vector2Length(physics->velocity) > 5) {
                        Direction pd = entities_common_calculate_direction(direction);
                        AssetsAnimation *animation = assets_animation_get_by_id(
                            game_state->animation_cache, player->run_animations[pd]);
                        f32 max_player_velocity = game_state->constants.player_run_max_velocity;
                        if ((game_state->time - player->last_frame_update_time) >
                            (animation->delay *
                             (2 - Vector2Length(physics->velocity) / max_player_velocity))) {
                            player->last_frame_update_time = game_state->time;
                            u16 new_frame_id =
                                (player->last_running_frame_idx + 1) % animation->frames_count;
                            sprite->frame_id = animation->frames[new_frame_id];
                            player->last_running_frame_idx = new_frame_id;
                        }
                        player->last_direction = pd;
                    } else {
                        sprite->frame_id = player->stand_frames[player->last_direction];
                    }
                }
                break;
            }
            case GS_SAVE: {
                break;
            }
            case GS_COUNT:
                break;
            }
        }
    }
}
