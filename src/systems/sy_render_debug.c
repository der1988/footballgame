#include "assets.h"
#include "components.h"
#include "entities.h"
#include "football.h"
#include "render.h"
#include <raylib.h>
#include <raymath.h>

void systems_render_debug(struct game_state *game_state)
{
    if (game_state->constants.render_debug) {
	for_each_entity(entity, game_state->entity_repository) {
	    if (!entities_is_entity_alive_and_has_components(entity, 2, C_TRANSFORM, C_SPRITE)) {
		continue;
	    }

	    declare_entity_component(entity, transform);
	    declare_entity_component(entity, sprite);
	    declare_entity_component(entity, collider);

	    Vector2 position = transform->position;
	    f32 rotation = transform->rotation;
	    Vector2 scale = transform->scale;
	    components_transform_calculate_absolute(transform, &position, &rotation, &scale);

	    // TODO(marco): not great but I haven't found a better way to do it yet
	    position.y -= transform->virtual_height;

	    render_sprite_draw_debug_frame(
		assets_sprite_frame_get_by_id(game_state->assets.sprite_frame_cache,
					      sprite->frame_id),
		position,
		rotation,
		scale,
		sprite->pivot_override);

	    // Debug lines for pitch
	    Rectangle pitch = game_state->vars.pitch.field;
	    DrawRectangleLines(pitch.x, pitch.y, pitch.width, pitch.height, BLUE);

	    if (collider->header.enabled) {
		static Color colliders_colors[] = {RED, VIOLET, PINK, YELLOW};
		for (u16 i = 0; i < collider->num_colliders; ++i) {
		    if (collider->colliders[i].shape == CS_CIRCLE) {
			v2 pos = Vector2Add(transform->position,
					    collider->colliders[i].props.circle.offset);
			DrawCircleLinesV(
			    pos, collider->colliders[i].props.circle.radius, colliders_colors[i]);
		    } else if (collider->colliders[i].shape == CS_RECTANGLE) {
			v2 pos = Vector2Add(transform->position,
					    collider->colliders[i].props.rectangle.offset);
			DrawRectangleLines(pos.x,
					   pos.y,
					   collider->colliders[i].props.rectangle.size.x,
					   collider->colliders[i].props.rectangle.size.y,
					   colliders_colors[i]);
		    }
		}
	    }
	}

	// Ball debug
	struct component_ball *ball =
	    &entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id)
		 ->ball;
	for (u8 i = 0; i < 25; ++i) {
	    DrawCircle(ball->lookahead_positions[i].x, ball->lookahead_positions[i].z, 1, BLUE);
	}
	if (ball->future_bounce.available) {
	    DrawCircle(ball->future_bounce.position.x, ball->future_bounce.position.y, 1, PURPLE);
	}

	// Pitch
	DrawRectangleLinesEx(game_state->vars.pitch.field, 1, VIOLET);
	DrawRectangleLinesEx(game_state->vars.pitch.goal[PSI_NORTH].penalty_box, 1, YELLOW);
	DrawRectangleLinesEx(game_state->vars.pitch.goal[PSI_SOUTH].penalty_box, 1, YELLOW);
	DrawCircleV(game_state->vars.pitch.goal[PSI_NORTH].center, 1, RED);
	DrawCircleV(game_state->vars.pitch.goal[PSI_SOUTH].center, 1, RED);
    }
}
