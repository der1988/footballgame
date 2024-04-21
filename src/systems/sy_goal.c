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

            v3 goal_line_p1 = (v3){.x = game_state->vars.pitch_rectangle.x,
                                   .y = 0,
                                   .z = game_state->vars.pitch_rectangle.y};
            v3 goal_line_p2 = (v3){.x = game_state->vars.pitch_rectangle.x,
                                   .y = goal_rectangle.height,
                                   .z = game_state->vars.pitch_rectangle.y};
            v3 goal_line_p3 = (v3){.x = game_state->vars.pitch_rectangle.x +
                                        game_state->vars.pitch_rectangle.width,
                                   .y = goal_rectangle.height,
                                   .z = game_state->vars.pitch_rectangle.y};
            v3 goal_line_p4 = (v3){.x = game_state->vars.pitch_rectangle.x +
                                        game_state->vars.pitch_rectangle.width,
                                   .y = 0,
                                   .z = game_state->vars.pitch_rectangle.y};

            f32 post_width = 4.0f;

            if (game_state->vars.someone_scored) {
                if (ball_transform->position.y < goal_rectangle.y ||
                    ball_transform->position.x < goal_rectangle.x ||
                    ball_transform->position.x > (goal_rectangle.x + goal_rectangle.width)) {
                    ball_physics_3d->velocity.x = 0;
                    ball_physics_3d->velocity.z = 0;
                }
            } else if (game_state->vars.ball_is_playable && !game_state->vars.someone_scored &&
                       CheckCollisionCircleRec(ball_transform->position,
                                               ball_collider->props.circle.radius,
                                               goal_rectangle) &&
                       ball_transform->position.y < game_state->vars.pitch_rectangle.y) {

                v3 current_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);

                Ray ray = (Ray){.position = current_pos,
                                .direction = Vector3Negate(ball_physics_3d->velocity)};
                RayCollision collision = GetRayCollisionQuad(ray, goal_line_p1, goal_line_p2,
                                                             goal_line_p3, goal_line_p4);
                // printf("original collision %.2f, %.2f, %.2f\n", collision.point.x,
                //        collision.point.y, collision.point.z);
                if (collision.hit &&
                    collision.point.x > (goal_rectangle.x - ball_collider->props.circle.radius) &&
                    collision.point.x < (goal_rectangle.x + goal_rectangle.width +
                                         ball_collider->props.circle.radius) &&
                    collision.point.y <
                        (goal_rectangle.height + ball_collider->props.circle.radius)) {

                    ball->subject_to_after_touch = false;
                    b32 collided_with_post = false;
                    if (HAS_TIME_PASSED(game_state->time, goal->hit_left_post_time, 0.1f) &&
                        collision.point.x <
                            (goal_rectangle.x + post_width + ball_collider->props.circle.radius)) {
                        // hit left post
                        f32 normalised_hit_point = Clamp(
                            (collision.point.x - goal_rectangle.x - post_width * 0.5f) / post_width,
                            -1, 1);
                        ball_physics_3d->velocity.y *= -1;
                        ball_physics_3d->velocity = Vector3RotateByAxisAngle(
                            Vector3Negate(ball_physics_3d->velocity), (v3){.y = 1},
                            120 * normalised_hit_point * DEG2RAD);
                        collided_with_post = true;
                        goal->hit_left_post_time = game_state->time;
                    } else if (HAS_TIME_PASSED(game_state->time, goal->hit_right_post_time, 0.1f) &&
                               collision.point.x >
                                   (goal_rectangle.x + goal_rectangle.width - post_width -
                                    ball_collider->props.circle.radius)) {
                        // hit right post
                        f32 normalised_hit_point =
                            Clamp((collision.point.x - goal_rectangle.x - goal_rectangle.width +
                                   post_width * 0.5f) /
                                      post_width,
                                  -1, 1);
                        ball_physics_3d->velocity.y *= -1;
                        ball_physics_3d->velocity = Vector3RotateByAxisAngle(
                            Vector3Negate(ball_physics_3d->velocity), (v3){.y = 1},
                            120 * normalised_hit_point * DEG2RAD);
                        collided_with_post = true;
                        goal->hit_right_post_time = game_state->time;
                    } else if (HAS_TIME_PASSED(game_state->time, goal->hit_crossbar_time, 0.1f) &&
                               collision.point.y > (goal_rectangle.height - post_width -
                                                    ball_collider->props.circle.radius)) {
                        // hit cross bar
                        f32 normalised_hit_point =
                            Clamp((collision.point.y - goal_rectangle.height + post_width) /
                                      post_width,
                                  -1, 1) *
                            -1;
                        ball_physics_3d->velocity.x *= -1;
                        ball_physics_3d->velocity = Vector3RotateByAxisAngle(
                            Vector3Negate(ball_physics_3d->velocity), (v3){.x = 1},
                            120 * normalised_hit_point * DEG2RAD);
                        collided_with_post = true;
                        goal->hit_crossbar_time = game_state->time;
                    }
                    if (collided_with_post) {
                        ball_physics_3d->velocity = Vector3Scale(
                            ball_physics_3d->velocity, game_state->constants.ball_cr_against_post);

                        v3 final_pos =
                            Vector3Add(collision.point,
                                       Vector3Scale(ball_physics_3d->velocity, game_state->dt));
                        // v3 final_pos = collision.point;

                        ball_transform->position = V3_TO_V2(final_pos);
                        ball_transform->virtual_height = final_pos.y;
                        printf("post at %.2f, %.2f, %.2f\n", final_pos.x, final_pos.y, final_pos.z);
                    } else {
                        game_state->vars.someone_scored = true;
                        printf("goal at %.2f, %.2f, %.2f\n", collision.point.x, collision.point.y,
                               collision.point.z);
                    }
                } else {
                    game_state->vars.ball_is_playable = false;
                    printf("ball is out\n");
                }
            }
        }
    }
}
