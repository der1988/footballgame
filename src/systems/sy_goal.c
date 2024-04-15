#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../steering/steering.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

void systems_goal(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentTransform *ball_transform = entities_component_get_internal(
        game_state->entity_repository, CT_TRANSFORM, game_state->vars.ball_entity_id);
    ComponentPhysics3d *ball_physics_3d = entities_component_get_internal(
        game_state->entity_repository, CT_PHYSICS_3D, game_state->vars.ball_entity_id);
    ComponentCollider *ball_collider = entities_component_get_internal(
        game_state->entity_repository, CT_COLLIDER, game_state->vars.ball_entity_id);
    ComponentBall *ball = entities_component_get_internal(game_state->entity_repository, CT_BALL,
                                                          game_state->vars.ball_entity_id);

    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(
                entity_repository, entity_id, 4, CT_TRANSFORM, CT_SPRITE, CT_GOAL, CT_COLLIDER)) {

            ComponentTransform *transform = ComponentCastToInternalUnchecked(
                game_state->entity_repository, CT_TRANSFORM, entity_id);
            ComponentSprite *sprite = ComponentCastToInternalUnchecked(
                game_state->entity_repository, CT_SPRITE, entity_id);
            ComponentGoal *goal =
                ComponentCastToInternalUnchecked(game_state->entity_repository, CT_GOAL, entity_id);
            ComponentCollider *collider = ComponentCastToInternalUnchecked(
                game_state->entity_repository, CT_COLLIDER, entity_id);

            Rectangle goal_rectangle = {
                .x = transform->position.x + collider->props.rectangle.offset.x,
                .y = transform->position.y + collider->props.rectangle.offset.y,
                .width = collider->props.rectangle.size.x,
                .height = collider->props.rectangle.size.y};

            if (CheckCollisionCircleRec(ball_transform->position,
                                        ball_collider->props.circle.radius, goal_rectangle)) {
                v3 current_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
                printf("original collision %.2f, %.2f, %.2f\n", current_pos.x, current_pos.y,
                       current_pos.z);
                v3 goal_line_p1 = (v3){.x = game_state->vars.pitch_rectangle.x,
                                       .y = 0,
                                       .z = game_state->vars.pitch_rectangle.y};
                v3 goal_line_p2 = (v3){.x = game_state->vars.pitch_rectangle.x,
                                       .y = 75.3f,
                                       .z = game_state->vars.pitch_rectangle.y};
                v3 goal_line_p3 = (v3){.x = game_state->vars.pitch_rectangle.x +
                                            game_state->vars.pitch_rectangle.width,
                                       .y = 75.3f,
                                       .z = game_state->vars.pitch_rectangle.y};
                v3 goal_line_p4 = (v3){.x = game_state->vars.pitch_rectangle.x +
                                            game_state->vars.pitch_rectangle.width,
                                       .y = 0,
                                       .z = game_state->vars.pitch_rectangle.y};
                Ray ray = {.position = current_pos,
                           .direction = Vector3Negate(ball_physics_3d->velocity)};
                RayCollision collision = GetRayCollisionQuad(ray, goal_line_p1, goal_line_p2,
                                                             goal_line_p3, goal_line_p4);
                if (collision.hit) {
                    ball->subject_to_after_touch = false;
                    if (collision.point.x > 930 && collision.point.x < 960) {
                        // hit left post
                        ball_physics_3d->velocity = Vector3Reflect(
                            ball_physics_3d->velocity,
                            Vector3Normalize((Vector3){.x = GetRandomValue(-100, 100),
                                                       .z = GetRandomValue(-100, 100)}));
                        v3 final_pos =
                            Vector3Add(collision.point,
                                       Vector3Scale(ball_physics_3d->velocity, game_state->dt));
                        ball_transform->position = V3_TO_V2(final_pos);
                        ball_transform->virtual_height = final_pos.y;
                        ball_physics_3d->accel =
                            Vector3Scale(Vector3Normalize(ball_physics_3d->velocity),
                                         Vector3Length(ball_physics_3d->accel));
                    } else if (collision.point.x > 1166 && collision.point.x < 1196) {
                        // hit right post
                        ball_physics_3d->velocity = Vector3Reflect(
                            ball_physics_3d->velocity,
                            Vector3Normalize((Vector3){.x = GetRandomValue(-100, 100),
                                                       .z = GetRandomValue(-100, 100)}));
                        v3 final_pos =
                            Vector3Add(collision.point,
                                       Vector3Scale(ball_physics_3d->velocity, game_state->dt));
                        ball_transform->position = V3_TO_V2(final_pos);
                        ball_transform->virtual_height = final_pos.y;
                        ball_physics_3d->accel =
                            Vector3Scale(Vector3Normalize(ball_physics_3d->velocity),
                                         Vector3Length(ball_physics_3d->accel));
                    } else if (collision.point.y > 60 && collision.point.y < 75.3) {
                        // hit cross bar
                        ball_physics_3d->velocity = Vector3Reflect(
                            ball_physics_3d->velocity,
                            Vector3Normalize((Vector3){.y = GetRandomValue(-100, 100),
                                                       .z = GetRandomValue(-100, 100)}));
                        v3 final_pos =
                            Vector3Add(collision.point,
                                       Vector3Scale(ball_physics_3d->velocity, game_state->dt));
                        ball_transform->position = V3_TO_V2(final_pos);
                        ball_transform->virtual_height = final_pos.y;
                        ball_physics_3d->accel =
                            Vector3Scale(Vector3Normalize(ball_physics_3d->velocity),
                                         Vector3Length(ball_physics_3d->accel));
                    }
                    printf("collided with goal at %.2f, %.2f, %.2f\n", collision.point.x,
                           collision.point.y, collision.point.z);
                }
                // 930, 1166
            }
        }
    }
}
