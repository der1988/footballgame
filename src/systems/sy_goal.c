#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

void systems_goal(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentTransform *ball_transform = entities_component_get_typed(
        game_state->entity_repository, CT_TRANSFORM, game_state->vars.ball_entity_id);
    ComponentPhysics3d *ball_physics_3d = entities_component_get_typed(
        game_state->entity_repository, CT_PHYSICS_3D, game_state->vars.ball_entity_id);
    ComponentCollider *ball_collider = entities_component_get_typed(
        game_state->entity_repository, CT_COLLIDER, game_state->vars.ball_entity_id);
    ComponentBall *ball = entities_component_get_typed(game_state->entity_repository, CT_BALL,
                                                       game_state->vars.ball_entity_id);

    // First let's check if it's a goal
    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(
                entity_repository, entity_id, 4, CT_TRANSFORM, CT_SPRITE, CT_GOAL, CT_COLLIDER)) {

            ComponentTransform *transform = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_TRANSFORM, entity_id);
            ComponentSprite *sprite =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_SPRITE, entity_id);
            ComponentGoal *goal =
                ComponentCastToTypedUnchecked(game_state->entity_repository, CT_GOAL, entity_id);
            ComponentCollider *collider = ComponentCastToTypedUnchecked(
                game_state->entity_repository, CT_COLLIDER, entity_id);

            Rectangle goal_rectangle = {
                .x = transform->position.x + collider->colliders[0].props_rectangle.offset.x,
                .y = transform->position.y + collider->colliders[0].props_rectangle.offset.y,
                .width = collider->colliders[0].props_rectangle.size.x,
                .height = collider->colliders[0].props_rectangle.size.y};

            v3 goal_line_p1 = (v3){.x = game_state->vars.pitch.rectangle.x,
                                   .y = 0,
                                   .z = game_state->vars.pitch.rectangle.y};
            v3 goal_line_p2 = (v3){.x = game_state->vars.pitch.rectangle.x,
                                   .y = goal_rectangle.height,
                                   .z = game_state->vars.pitch.rectangle.y};
            v3 goal_line_p3 = (v3){.x = game_state->vars.pitch.rectangle.x +
                                        game_state->vars.pitch.rectangle.width,
                                   .y = goal_rectangle.height,
                                   .z = game_state->vars.pitch.rectangle.y};
            v3 goal_line_p4 = (v3){.x = game_state->vars.pitch.rectangle.x +
                                        game_state->vars.pitch.rectangle.width,
                                   .y = 0,
                                   .z = game_state->vars.pitch.rectangle.y};

            f32 post_width = 2;

            if (game_state->vars.match.state == MS_GOAL) {
                if (ball_transform->position.y < goal_rectangle.y - 3 ||
                    ball_transform->position.x < goal_rectangle.x ||
                    ball_transform->position.x > (goal_rectangle.x + goal_rectangle.width)) {
                    ball_physics_3d->velocity.x *= 0.8f;
                    ball_physics_3d->velocity.z *= 0.8f;
                    if (ball_transform->virtual_height > goal_rectangle.height &&
                        ball_physics_3d->velocity.y > 0) {
                        ball_physics_3d->velocity.y *= 0.9f;
                    }
                }
            } else if (game_state->vars.match.state == MS_PLAY &&
                       CheckCollisionCircleRec(ball_transform->position,
                                               ball_collider->colliders[0].props_circle.radius,
                                               goal_rectangle) &&
                       ball_transform->position.y < game_state->vars.pitch.rectangle.y) {

                v3 current_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);

                Ray ray = (Ray){.position = current_pos,
                                .direction = Vector3Negate(ball_physics_3d->velocity)};
                RayCollision collision = GetRayCollisionQuad(ray, goal_line_p1, goal_line_p2,
                                                             goal_line_p3, goal_line_p4);
                // printf("original collision %.2f, %.2f, %.2f\n", collision.point.x,
                //        collision.point.y, collision.point.z);
                if (collision.hit &&
                    collision.point.x >
                        (goal_rectangle.x - ball_collider->colliders[0].props_circle.radius) &&
                    collision.point.x < (goal_rectangle.x + goal_rectangle.width +
                                         ball_collider->colliders[0].props_circle.radius) &&
                    collision.point.y <
                        (goal_rectangle.height + ball_collider->colliders[0].props_circle.radius)) {

                    b32 collided_with_post = false;
                    b32 scored_off_post = false;
                    v3 new_velocity = ball_physics_3d->velocity;

                    if (HAS_TIME_PASSED(game_state->time, goal->hit_left_post_time, 0.1f) &&
                        collision.point.x < (goal_rectangle.x + post_width +
                                             ball_collider->colliders[0].props_circle.radius)) {
                        // hit left post
                        f32 normalised_hit_point = Clamp(
                            (collision.point.x - goal_rectangle.x - post_width * 0.5f) / post_width,
                            -1, 1);
                        new_velocity.y *= -1;
                        new_velocity =
                            Vector3RotateByAxisAngle(Vector3Negate(new_velocity), (v3){.y = 1},
                                                     120 * normalised_hit_point * DEG2RAD);
                        collided_with_post = true;
                        goal->hit_left_post_time = game_state->time;
                        // This needs to be generalised to account for goal position (top vs
                        // bottom)
                        if (new_velocity.x > 0 && new_velocity.z < 0) {
                            scored_off_post = true;
                        }
                    } else if (HAS_TIME_PASSED(game_state->time, goal->hit_right_post_time, 0.1f) &&
                               collision.point.x >
                                   (goal_rectangle.x + goal_rectangle.width - post_width -
                                    ball_collider->colliders[0].props_circle.radius)) {
                        // hit right post
                        f32 normalised_hit_point =
                            Clamp((collision.point.x - goal_rectangle.x - goal_rectangle.width +
                                   post_width * 0.5f) /
                                      post_width,
                                  -1, 1);
                        new_velocity.y *= -1;
                        new_velocity =
                            Vector3RotateByAxisAngle(Vector3Negate(new_velocity), (v3){.y = 1},
                                                     120 * normalised_hit_point * DEG2RAD);
                        collided_with_post = true;
                        goal->hit_right_post_time = game_state->time;
                        if (new_velocity.x < 0 && new_velocity.z < 0) {
                            scored_off_post = true;
                        }
                    } else if (HAS_TIME_PASSED(game_state->time, goal->hit_crossbar_time, 0.1f) &&
                               collision.point.y >
                                   (goal_rectangle.height - post_width -
                                    ball_collider->colliders[0].props_circle.radius)) {
                        // hit cross bar
                        f32 normalised_hit_point =
                            Clamp((collision.point.y - goal_rectangle.height + post_width) /
                                      post_width,
                                  -1, 1) *
                            -1;
                        new_velocity.x *= -1;
                        new_velocity =
                            Vector3RotateByAxisAngle(Vector3Negate(new_velocity), (v3){.x = 1},
                                                     120 * normalised_hit_point * DEG2RAD);
                        collided_with_post = true;
                        goal->hit_crossbar_time = game_state->time;
                        if (new_velocity.z < 0 && new_velocity.y < 0) {
                            scored_off_post = true;
                        }
                    }
                    if (collided_with_post) {
                        ball->subject_to_after_touch = false;
                        new_velocity =
                            Vector3Scale(new_velocity, game_state->constants.ball_cr_against_post);

                        v3 new_position =
                            Vector3Add(collision.point, Vector3Scale(new_velocity, game_state->dt));

                        entities_ball_set_position_and_velocity(game_state->vars.ball_entity_id,
                                                                new_position, new_velocity,
                                                                game_state);
                        printf("post at %.2f, %.2f, %.2f\n", collision.point.x, collision.point.y,
                               collision.point.z);
                        if (scored_off_post) {
                            game_state->vars.match.state = MS_GOAL;
                            printf("goal off post\n");
                        }
                    } else if (collision.point.x >
                                   (goal_rectangle.x + post_width +
                                    ball_collider->colliders[0].props_circle.radius) &&
                               collision.point.x <
                                   (goal_rectangle.x + goal_rectangle.width - post_width -
                                    ball_collider->colliders[0].props_circle.radius) &&
                               collision.point.y <
                                   (goal_rectangle.height - post_width -
                                    ball_collider->colliders[0].props_circle.radius)) {
                        game_state->vars.match.state = MS_GOAL;
                        printf("goal at %.2f, %.2f, %.2f\n", collision.point.x, collision.point.y,
                               collision.point.z);
                    } else {
                        game_state->vars.match.state = MS_CORNER_KICK;
                        printf("ball is out\n");
                    }
                } else {
                    game_state->vars.match.state = MS_CORNER_KICK;
                    printf("ball is out\n");
                }
            }
        }
    }
    if (game_state->vars.match.state == MS_PLAY &&
        !CheckCollisionPointRec(ball_transform->position, game_state->vars.pitch.rectangle)) {
        // Ball is out of bounds
        Rectangle *pitch_rectangle = &game_state->vars.pitch.rectangle;
        if (ball_transform->position.x < pitch_rectangle->x) {
            // Throw in left
            game_state->vars.match.state = MS_THROW_IN;
        } else if (ball_transform->position.x > (pitch_rectangle->x + pitch_rectangle->width)) {
            // Throw in right
            game_state->vars.match.state = MS_THROW_IN;
        }
    }
}
