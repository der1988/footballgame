#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../steering/steering.h"
#include <raylib.h>
#include <raymath.h>

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
                // Goal!
                ball_physics_3d->velocity = Vector3Negate(ball_physics_3d->velocity);
            }
        }
    }
}
