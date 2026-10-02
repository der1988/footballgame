#include "macro.h"
#include "debugui.h"
#include "debugui_internal.h"
#include "entity.h"
#include "football.h"
#include "game.h"
#include "render.h"
#include "entity_components.h"
#include "action.h"
#include "entity_player.h"
#include "entity_ball.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>
#include <string.h>
#include <ctype.h>

static b32 str_contains_ci(const char *haystack, const char *needle)
{
	if (needle[0] == '\0')
		return true;
	char h[ENTITY_NAME_MAX_SIZE];
	char n[ENTITY_INSPECTOR_FILTER_BUFFER_SIZE];
	usize i;
	for (i = 0; i < ENTITY_NAME_MAX_SIZE - 1 && haystack[i]; i++)
		h[i] = (char)tolower((unsigned char)haystack[i]);
	h[i] = '\0';
	for (i = 0; i < ENTITY_INSPECTOR_FILTER_BUFFER_SIZE - 1 && needle[i]; i++)
		n[i] = (char)tolower((unsigned char)needle[i]);
	n[i] = '\0';
	return strstr(h, n) != NULL;
}

static void draw_component_transform(struct component_transform *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Transform", 0)) {
		igText(TextFormat("  position:       (%.2f, %.2f)", c->position.x, c->position.y));
		igText(TextFormat("  rotation:       %.2f", c->rotation));
		igText(TextFormat("  scale:          (%.2f, %.2f)", c->scale.x, c->scale.y));
		igText(TextFormat("  virtual_height: %.2f", c->virtual_height));
		igText(TextFormat("  parent:         %s", c->parent ? "set" : "null"));
	}
}

static void draw_component_physics(struct component_physics *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Physics", 0)) {
		igText(TextFormat("  mass:         %.2f", c->mass));
		igText(TextFormat("  accel:        (%.2f, %.2f)", c->accel.x, c->accel.y));
		igText(TextFormat("  velocity:     (%.2f, %.2f)", c->velocity.x, c->velocity.y));
		igText(TextFormat("  max_velocity: %.2f", c->max_velocity));
		igText(TextFormat("  max_force:    %.2f", c->max_force));
	}
}

static void draw_component_physics3d(struct component_physics3d *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Physics3D", 0)) {
		igText(TextFormat("  mass:         %.2f", c->mass));
		igText(
			TextFormat("  accel:        (%.2f, %.2f, %.2f)", c->accel.x, c->accel.y, c->accel.z));
		igText(TextFormat(
			"  velocity:     (%.2f, %.2f, %.2f)", c->velocity.x, c->velocity.y, c->velocity.z));
		igText(TextFormat("  max_velocity: %.2f", c->max_velocity));
		igText(TextFormat("  max_force:    %.2f", c->max_force));
	}
}

static void draw_component_sprite(struct component_sprite *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Sprite", 0)) {
		igText(TextFormat(
			"  color:          (%d,%d,%d,%d)", c->color.r, c->color.g, c->color.b, c->color.a));
		igText(TextFormat("  flipX:          %s", c->flipX ? "yes" : "no"));
		igText(TextFormat("  flipY:          %s", c->flipY ? "yes" : "no"));
		igText(TextFormat("  orientation:    %s", sprite_orientation_to_string[c->orientation]));
		igText(TextFormat("  layer:          %s", sprite_layer_to_string(c->layer)));
		igText(TextFormat("  order_in_layer: %d", c->order_in_layer));
		igText(
			TextFormat("  pivot_override: (%.2f, %.2f)", c->pivot_override.x, c->pivot_override.y));
		igText(TextFormat("  frame_id:       %u", c->frame_id));
		igText(TextFormat("  draw_shadow:    %s", c->draw_shadow ? "yes" : "no"));
		igText(TextFormat("  before_draw:    %s", c->before_draw ? "set" : "null"));
		igText(TextFormat("  after_draw:     %s", c->after_draw ? "set" : "null"));
	}
}

static void draw_component_camera(struct component_camera *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Camera", 0)) {
		igText(TextFormat("  camera:           %s", camera_type_to_string[c->camera]));
		igText(TextFormat("  slowing_distance: %.2f", c->slowing_distance));
		igText(TextFormat("  offset:           (%.2f, %.2f)", c->offset.x, c->offset.y));
	}
}

static void draw_component_collider(struct component_collider *c,
									const char **names,
									u16 names_count)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Collider", 0)) {
		igText(TextFormat("  num_colliders: %u", c->num_colliders));
		for (u16 i = 0; i < c->num_colliders; i++) {
			igPushID_Int((int)i);
			const char *label = (names && i < names_count) ? names[i] : TextFormat("%u", i);
			if (c->colliders[i].shape == CS_CIRCLE) {
				igText(TextFormat("  [%s] shape: circle", label));
				igText(TextFormat("       radius: %.2f", c->colliders[i].props.circle.radius));
				igText(TextFormat("       offset: (%.2f, %.2f)",
								  c->colliders[i].props.circle.offset.x,
								  c->colliders[i].props.circle.offset.y));
			} else {
				igText(TextFormat("  [%s] shape: rectangle", label));
				igText(TextFormat("       size:   (%.2f, %.2f)",
								  c->colliders[i].props.rectangle.size.x,
								  c->colliders[i].props.rectangle.size.y));
				igText(TextFormat("       offset: (%.2f, %.2f)",
								  c->colliders[i].props.rectangle.offset.x,
								  c->colliders[i].props.rectangle.offset.y));
			}
			igPopID();
		}
	}
}

static void draw_component_action(struct component_action *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Action", 0)) {
		for (i32 i = 0; i < MAX_CONCURRENT_ACTIONS; i++) {
			struct action_runner *r = &c->running_actions[i];
			igPushID_Int(i);
			if (!r->action) {
				igText(TextFormat("  [%d] empty", i));
			} else {
				igText(TextFormat("  [%d] type:      %s", i, ActionTypeToString[r->action->type]));
				igText(TextFormat("      duration:  %.3f  loop: %s",
								  r->action->duration,
								  r->action->loop ? "yes" : "no"));
				igText(TextFormat("      started:   %s  completed: %s  paused: %s",
								  r->started ? "yes" : "no",
								  r->completed ? "yes" : "no",
								  r->paused ? "yes" : "no"));
			}
			igPopID();
		}
	}
}

static void draw_component_target(struct component_target *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Target", 0)) {
		if (c->entity_id == EID_NOT_VALID) {
			igText("  entity_id: (invalid)");
		} else {
			igText(TextFormat("  entity_id: %u", c->entity_id));
		}
	}
}

static void draw_component_player(struct component_player *c, struct game_state *game_state)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Player", 0)) {
		igText(TextFormat("  state:                %s",
						  state_machine_player_state_string[c->state_machine.current_state]));
		igText(TextFormat("  prev_state:           %s",
						  state_machine_player_state_string[c->state_machine.previous_state]));
		igText(TextFormat(
			"  dribble:              %s",
			c->state_machine.current_state == PS_NORMAL &&
					c->state_machine.state_data.normal.dribble.active ?
				player_dribble_turn_string[c->state_machine.state_data.normal.dribble.turn] :
				"-"));
		if (c->state_machine.current_state == PS_TACKLE) {
			const struct state_machine_player_tackle_data *tackle =
				&c->state_machine.state_data.tackle;
			igText(TextFormat("  tackle_victim_id:     %u", tackle->victim_entity_id));
			// Touch times are shown from the start of the tackle
			igText(TextFormat("  tackle_ball_touch:    %s",
							  tackle->touched_ball ?
								  TextFormat("%.3f", tackle->ball_touch_time - tackle->start_time) :
								  "-"));
			igText(
				TextFormat("  tackle_victim_touch:  %s",
						   tackle->touched_player ?
							   TextFormat("%.3f", tackle->victim_touch_time - tackle->start_time) :
							   "-"));
		}
		igText(TextFormat("  control_ball:         %s", c->control_ball ? "yes" : "no"));
		igText(TextFormat("  team_side:            %s", team_side_to_string[c->team_side]));
		igText(TextFormat("  formation_pos_idx:    %u", c->team_formation_position_idx));
		igText(TextFormat("  current_max_velocity: %.2f", c->current_max_velocity));
		igText(TextFormat("  kicked_ball:          %s", c->kicked_ball ? "yes" : "no"));
		igText(TextFormat("  is_colliding_ball:    %s", c->is_colliding_with_ball ? "yes" : "no"));
		igText(TextFormat("  collision_force:      %.2f", c->collision_force));
		igText(TextFormat("  characteristics:      spd=%u acc=%u str=%u wgt=%u",
						  c->characteristics.speed,
						  c->characteristics.acceleration,
						  c->characteristics.strength,
						  c->characteristics.weight));
		igText(TextFormat("  precision_control:    %.3f", c->time_spent_in_precision_control));
		igText(TextFormat("  run_control:          %.3f", c->time_spent_in_run_control));
		igText(TextFormat("  last_kick_time:       %s",
						  debugui_time_string(c->last_kick_time, game_state)));
		igText(TextFormat("  last_tackle_dispossession_time: %s",
						  debugui_time_string(c->last_tackle_dispossession_time, game_state)));
		igText(TextFormat("  last_header_time:     %s",
						  debugui_time_string(c->last_header_time, game_state)));
		igText(TextFormat("  last_possession_time: %s",
						  debugui_time_string(c->last_possession_time, game_state)));
		igText(TextFormat("  last_body_collision:  %s  impulse: %.2f",
						  debugui_time_string(c->last_body_collision_time, game_state),
						  (double)c->last_body_collision_impulse));
	}
}

static void draw_component_player_controls(struct component_player_controls *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("PlayerControls", 0)) {
		igText(TextFormat("  steering:       (%.2f, %.2f)", c->steering.x, c->steering.y));
		igText(TextFormat("  shot:           %.2f", c->shot));
		const char *automatic_pass = c->automatic_pass ? "yes" : "no";
		if (c->pass_target_entity_id == EID_NOT_VALID) {
			igText(TextFormat(
				"  pass:           %.2f  auto: %s  target: (invalid)", c->pass, automatic_pass));
		} else {
			igText(TextFormat("  pass:           %.2f  auto: %s  target: %u",
							  c->pass,
							  automatic_pass,
							  c->pass_target_entity_id));
		}
		igText(TextFormat("  long_pass:      %.2f", c->long_pass));
		igText(TextFormat("  header_shot:    %s  header_pass: %s",
						  c->header_shot ? "yes" : "no",
						  c->header_pass ? "yes" : "no"));
		igText(TextFormat("  tackle:         %s  sliding_tackle: %s",
						  c->tackle ? "yes" : "no",
						  c->sliding_tackle ? "yes" : "no"));
		igText(TextFormat("  stop:           %s", c->stop ? "yes" : "no"));
		igText(TextFormat("  put_down:       %s  charging_throw: %s",
						  c->put_down ? "yes" : "no",
						  c->charging_throw ? "yes" : "no"));
		igText(TextFormat(
			"  jump:           %s  dive: %s", c->jump ? "yes" : "no", c->dive ? "yes" : "no"));
	}
}

static void draw_component_ball(struct component_ball *c, struct game_state *game_state)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Ball", 0)) {
		igText(TextFormat("  state:          %s",
						  state_machine_ball_state_string[c->state_machine.current_state]));
		igText(TextFormat("  prev_state:     %s",
						  state_machine_ball_state_string[c->state_machine.previous_state]));
		igText(TextFormat("  is_on_ground:   %s", c->is_on_the_ground ? "yes" : "no"));
		igText(TextFormat("  last_bounce:    %s",
						  debugui_time_string(c->last_bounce_time, game_state)));
		if (c->last_touch_entity_id == EID_NOT_VALID) {
			igText("  last_touch_id:  (invalid)");
		} else {
			igText(TextFormat("  last_touch_id:  %u  time: %s",
							  c->last_touch_entity_id,
							  debugui_time_string(c->last_touch_time, game_state)));
		}
		igText(TextFormat("  first_player_possession: %s",
						  debugui_time_string(c->first_player_possession_time, game_state)));
		if (c->last_play_entity_id == EID_NOT_VALID) {
			igText("  last_play_id:   (invalid)");
		} else {
			igText(TextFormat("  last_play_id:   %u  time: %s  type: %s",
							  c->last_play_entity_id,
							  debugui_time_string(c->last_play_time, game_state),
							  ball_play_type_string[c->last_play_type]));
		}
		igText(TextFormat("  last_loose:     %s",
						  debugui_time_string(c->last_loose_time, game_state)));
		igText(
			TextFormat("  not_pickable_by: %s", ball_not_pickable_by_string[c->not_pickable_by]));
		if (c->pass_target_entity_id == EID_NOT_VALID) {
			igText("  pass_target_id: (invalid)");
		} else {
			igText(TextFormat("  pass_target_id: %u", c->pass_target_entity_id));
		}
		if (c->last_tackle_entity_id == EID_NOT_VALID) {
			igText("  last_tackle_id: (invalid)");
		} else {
			igText(TextFormat("  last_tackle_id: %u  time: %s",
							  c->last_tackle_entity_id,
							  debugui_time_string(c->last_tackle_time, game_state)));
		}
		igText(TextFormat("  after_touch:    %s  activated_at: %s",
						  c->after_touch.active ? "yes" : "no",
						  debugui_time_string(c->after_touch.activated_at, game_state)));
		for (i32 i = 0; i < FUTURE_POSITION_COUNT; i++) {
			igText(TextFormat("  future_pos[%d]:  available=%s  dts=%u",
							  i,
							  c->future_positions[i].available ? "yes" : "no",
							  c->future_positions[i].dts));
		}
	}
}

static void draw_component_goal(struct component_goal *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Goal", 0)) {
		igText(TextFormat("  side:   %s", pitch_side_to_string[c->side]));
		igText(TextFormat("  scored: %s", c->scored ? "yes" : "no"));
	}
}

static void draw_component_goalie(struct component_goalie *c, struct game_state *game_state)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Goalie", 0)) {
		igText(TextFormat("  state:          %s",
						  state_machine_goalie_state_string[c->state_machine.current_state]));
		igText(TextFormat("  prev_state:     %s",
						  state_machine_goalie_state_string[c->state_machine.previous_state]));
		if (c->state_machine.current_state == GS_DIVE) {
			const struct goalie_dive *dive = &c->state_machine.state_data.dive.dive;
			igText(TextFormat("  dive_direction: %s", save_direction_to_string[dive->direction]));
			igText(TextFormat("  dive_height:    %s", save_height_to_string[dive->height]));
			igText(TextFormat("  dive_distance:  %.2f", dive->distance));
			igText(TextFormat("  dive_speed:     %.2f", dive->speed));
		}
		if (c->state_machine.current_state == GS_THROW) {
			const struct state_machine_goalie_throw_data *throw_ball =
				&c->state_machine.state_data.throw_ball;
			igText(TextFormat("  throw_dir:      (%.2f, %.2f)  power: %.2f",
							  throw_ball->direction.x,
							  throw_ball->direction.y,
							  throw_ball->power));
		}
		igText(
			TextFormat("  last_save_time: %s", debugui_time_string(c->last_save_time, game_state)));
	}
}

void debugui_entity_inspector_draw(debugui_t *handle, struct game_state *game_state)
{
	ASSERT(handle->initialised);

	igSetNextWindowSize((ImVec2){handle->window_size.x, handle->window_size.y},
						ImGuiCond_FirstUseEver);

	igBegin("Entity Inspector", NULL, 0);

	// Validate current selection
	if (handle->entity_inspector_selected_entity_id != EID_NOT_VALID) {
		struct entity *sel =
			entity_get(game_state->entity_repository, handle->entity_inspector_selected_entity_id);
		if (!sel || !sel->header.alive) {
			handle->entity_inspector_selected_entity_id = EID_NOT_VALID;
		}
	}

	// Filter input
	igText("Filter:");
	igSameLine(0, -1);
	igPushItemWidth(-1);
	igInputText("##filter",
				handle->entity_inspector_filter_buffer,
				ENTITY_INSPECTOR_FILTER_BUFFER_SIZE,
				0,
				NULL,
				NULL);
	igPopItemWidth();

	igSeparator();

	// Entity list
	if (igBeginListBox("##entities", (ImVec2){-1, 200})) {
		FOR_EACH_ENTITY(entity, game_state->entity_repository)
		{
			if (!entity->header.alive)
				continue;
			if (!str_contains_ci(entity->header.name, handle->entity_inspector_filter_buffer))
				continue;

			bool is_selected = (handle->entity_inspector_selected_entity_id == entity->header.id);
			b32 is_human =
				game_input_human_player_by_player_entity_id(game_state, entity->header.id) != NULL;
			const char *label = TextFormat("%s (%s)%s",
										   entity->header.name,
										   entity_type_to_string[entity->header.type],
										   is_human ? " [H]" : "");
			igPushID_Int((int)entity->header.id);
			if (igSelectable_Bool(label, is_selected, 0, (ImVec2){0, 0})) {
				handle->entity_inspector_selected_entity_id = entity->header.id;
			}
			igPopID();
		}
		igEndListBox();
	}

	igSeparator();

	// Component inspector
	if (handle->entity_inspector_selected_entity_id == EID_NOT_VALID) {
		igText("No entity selected.");
	} else {
		struct entity *e =
			entity_get(game_state->entity_repository, handle->entity_inspector_selected_entity_id);

		igText(TextFormat("ID: %u  Name: %s  Type: %s",
						  e->header.id,
						  e->header.name,
						  entity_type_to_string[e->header.type]));

		igSeparator();

		draw_component_transform(&e->transform);
		draw_component_physics(&e->physics);
		draw_component_physics3d(&e->physics3d);
		draw_component_sprite(&e->sprite);
		draw_component_camera(&e->camera);
		b32 is_player = e->header.type == ET_PLAYER;
		draw_component_collider(&e->collider,
								is_player ? player_collider_type_to_string : NULL,
								is_player ? PCT_COUNT : 0);
		draw_component_action(&e->action);
		draw_component_target(&e->target);
		draw_component_player(&e->player, game_state);
		draw_component_player_controls(&e->player_controls);
		draw_component_ball(&e->ball, game_state);
		draw_component_goal(&e->goal);
		draw_component_goalie(&e->goalie, game_state);
	}

	igEnd();
}
