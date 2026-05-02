#include "debugui/debugui.h"
#include "debugui/debugui_internal.h"
#include "entities.h"
#include "football.h"
#include "render.h"
#include "components.h"
#include "action.h"
#include "e_player.h"
#include "e_ball.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include <assert.h>
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

static void draw_component_player(struct component_player *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Player", 0)) {
		igText(TextFormat("  state:                %s",
						  state_machine_player_state_string[c->state_machine.current_state]));
		igText(TextFormat("  prev_state:           %s",
						  state_machine_player_state_string[c->state_machine.previous_state]));
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
		igText(TextFormat("  last_kick_time:       %.3f", (double)c->last_kick_time));
		igText(TextFormat("  last_header_time:     %.3f", (double)c->last_header_time));
		igText(TextFormat("  last_possession_time: %.3f", (double)c->last_possession_time));
	}
}

static void draw_component_player_controls(struct component_player_controls *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("PlayerControls", 0)) {
		igText(TextFormat("  steering:       (%.2f, %.2f)", c->steering.x, c->steering.y));
		igText(TextFormat("  shot:           %.2f", c->shot));
		igText(TextFormat(
			"  pass:           %.2f  auto: %s", c->pass, c->automatic_pass ? "yes" : "no"));
		igText(TextFormat("  long_pass:      %.2f", c->long_pass));
		igText(TextFormat("  header_shot:    %s  header_pass: %s",
						  c->header_shot ? "yes" : "no",
						  c->header_pass ? "yes" : "no"));
		igText(TextFormat("  sliding_tackle: %s  stop: %s",
						  c->sliding_tackle ? "yes" : "no",
						  c->stop ? "yes" : "no"));
	}
}

static void draw_component_ball(struct component_ball *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Ball", 0)) {
		igText(TextFormat("  state:          %s",
						  state_machine_ball_state_string[c->state_machine.current_state]));
		igText(TextFormat("  prev_state:     %s",
						  state_machine_ball_state_string[c->state_machine.previous_state]));
		igText(TextFormat("  is_on_ground:   %s", c->is_on_the_ground ? "yes" : "no"));
		igText(TextFormat("  last_bounce:    %.3f", (double)c->last_bounce_time));
		if (c->last_touch_entity_id == EID_NOT_VALID) {
			igText("  last_touch_id:  (invalid)");
		} else {
			igText(TextFormat("  last_touch_id:  %u  time: %.3f",
							  c->last_touch_entity_id,
							  (double)c->last_touch_time));
		}
		if (c->last_kicker_entity_id == EID_NOT_VALID) {
			igText("  last_kicker_id: (invalid)");
		} else {
			igText(TextFormat("  last_kicker_id: %u  time: %.3f",
							  c->last_kicker_entity_id,
							  (double)c->last_kick_time));
		}
		if (c->pass_target_entity_id == EID_NOT_VALID) {
			igText("  pass_target_id: (invalid)");
		} else {
			igText(TextFormat("  pass_target_id: %u", c->pass_target_entity_id));
		}
		if (c->last_tackle_entity_id == EID_NOT_VALID) {
			igText("  last_tackle_id: (invalid)");
		} else {
			igText(TextFormat("  last_tackle_id: %u  time: %.3f",
							  c->last_tackle_entity_id,
							  (double)c->last_tackle_time));
		}
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
		igText(TextFormat("  side:          %s", pitch_side_to_string[c->side]));
		igText(TextFormat("  scored:        %s", c->scored ? "yes" : "no"));
		igText(TextFormat("  hit_post_time: %.3f", (double)c->hit_post_time));
	}
}

static void draw_component_goalie(struct component_goalie *c)
{
	if (!c->header.enabled)
		return;
	if (igCollapsingHeader_TreeNodeFlags("Goalie", 0)) {
		igText(TextFormat("  state:          %s",
						  state_machine_goalie_state_string[c->state_machine.current_state]));
		igText(TextFormat("  prev_state:     %s",
						  state_machine_goalie_state_string[c->state_machine.previous_state]));
		igText(TextFormat("  dive_direction: %s", save_direction_to_string[c->dive_direction]));
		igText(TextFormat("  dive_height:    %s", save_height_to_string[c->dive_height]));
		igText(TextFormat("  dive_distance:  %.2f", c->dive_distance));
		igText(TextFormat("  dive_speed:     %.2f", c->dive_speed));
		igText(TextFormat("  last_save_time: %.3f", (double)c->last_save_time));
	}
}

void debugui_entity_inspector_draw(debugui_t *handle, struct game_state *game_state)
{
	assert(handle->initialised);

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
		draw_component_player(&e->player);
		draw_component_player_controls(&e->player_controls);
		draw_component_ball(&e->ball);
		draw_component_goal(&e->goal);
		draw_component_goalie(&e->goalie);
	}

	igEnd();
}
