#include "components.h"
#include "entities.h"
#include "game.h"
#include "math_ext.h"
#include "types.h"
#include <raylib.h>
#include <raymath.h>

static void draw_collider(v2 position, struct collider *collider, color_t color)
{
	if (collider->shape == CS_CIRCLE) {
		v2 adjusted_position = Vector2Add(position, collider->props.circle.offset);
		DrawCircleLinesV(adjusted_position, collider->props.circle.radius, color);
	} else if (collider->shape == CS_RECTANGLE) {
		v2 adjusted_position = Vector2Add(position, collider->props.rectangle.offset);
		DrawRectangleLines(adjusted_position.x - collider->props.rectangle.size.x * 0.5f,
						   adjusted_position.y - collider->props.rectangle.size.y * 0.5f,
						   collider->props.rectangle.size.x,
						   collider->props.rectangle.size.y,
						   color);
	}
}

void systems_render_debug(struct game_state *game_state)
{
	if (game_state->debug_flags.debug_render) {
		FOR_EACH_ENTITY(entity, game_state->entity_repository)
		{
			if (!entity_is_alive_and_has_components(entity, 2, C_TRANSFORM, C_SPRITE)) {
				continue;
			}

			DECLARE_ENTITY_COMPONENT(entity, transform);
			DECLARE_ENTITY_COMPONENT(entity, sprite);
			DECLARE_ENTITY_COMPONENT(entity, collider);

			v2 position = transform->position;
			f32 rotation = transform->rotation;
			v2 scale = transform->scale;
			component_transform_local_to_world(transform, &position, &rotation, &scale);

			// TODO(marco): not great but I haven't found a better way to do it yet
			position.y -= transform->virtual_height;

			// Bounding boxes
			if (game_state->debug_flags.show_bounding_boxes) {
				render_sprite_draw_debug_frame(
					asset_sprite_frame_get_by_id(game_state->assets.sprite_frame_cache,
												 sprite->frame_id),
					position,
					rotation,
					scale,
					sprite->pivot_override);
			}

			if (collider->header.enabled) {
				static color_t colliders_colors[] = {
					RED, VIOLET, PINK, YELLOW, BLUE, MAGENTA, SKYBLUE};
				switch (entity->header.type) {
				case ET_PLAYER:
					for (usize i = 0; i < PCT_COUNT; ++i) {
						if (game_state->debug_flags.show_player_collider[i]) {
							draw_collider(
								transform->position, &collider->colliders[i], colliders_colors[i]);
						}
					}
					break;
				case ET_BALL:
					if (game_state->debug_flags.show_ball_collider) {
						draw_collider(
							transform->position, &collider->colliders[0], colliders_colors[0]);
					}
					break;
				case ET_GOAL:
					if (game_state->debug_flags.show_goal_collider) {
						draw_collider(
							transform->position, &collider->colliders[0], colliders_colors[0]);
					}
					break;
				default:
				}
			}

			// Ball lookahead positions
			struct component_ball *ball = &game_state->vars.ball_entity->ball;
			if (game_state->debug_flags.show_ball_lookahead_positions) {
				for (u8 i = 0; i < game_state->constants.ball_lookaheads; ++i) {
					f32 size = 1 + Clamp01(ball->lookahead_positions[i].y / 100.0f) * 4;
					DrawCircle(
						ball->lookahead_positions[i].x, ball->lookahead_positions[i].z, size, GRAY);
				}
			}

			// Ball future bounce position
			if (game_state->debug_flags.show_ball_future_bounce_position) {
				struct future_position *future_bounce =
					&ball->future_positions[FUTURE_POSITION_BOUNCE];
				if (future_bounce->available) {
					DrawCircle(future_bounce->position.x, future_bounce->position.z, 1, PURPLE);
				}
			}

			// Pitch
			if (game_state->debug_flags.show_pitch_lines) {
				DrawRectangleLinesEx(game_state->vars.pitch.field, 1, VIOLET);
				DrawRectangleLinesEx(game_state->vars.pitch.goal[PSI_NORTH].penalty_box, 1, YELLOW);
				DrawRectangleLinesEx(game_state->vars.pitch.goal[PSI_SOUTH].penalty_box, 1, YELLOW);
				DrawCircleV(game_state->vars.pitch.goal[PSI_NORTH].center, 1, RED);
				DrawCircleV(game_state->vars.pitch.goal[PSI_SOUTH].center, 1, RED);
			}
		}
	}
}
