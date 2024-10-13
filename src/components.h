#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "actions.h"
#include "assets.h"
#include "game.h"
#include "render.h"
#include "types.h"
#include <raylib.h>
#include "constants.h"

#define MaxConcurrentActions 5

// Forward declarations
struct StateMachineBall;
struct StateMachinePlayer;

struct ComponentHeader
{
    b32 enabled;
};

struct ComponentTransform
{
    struct ComponentHeader header;
    struct ComponentTransform *parent;
    v2 position;
    f32 rotation;
    v2 scale;
    f32 virtual_height;
};

struct ComponentPhysics
{
    struct ComponentHeader header;
    f32 mass;
    v2 accel;
    v2 velocity;
    f32 max_velocity;
    f32 max_force;
};

struct ComponentPhysics3d
{
    struct ComponentHeader header;
    f32 mass;
    v3 accel;
    v3 velocity;
    f32 max_velocity;
    f32 max_force;
};

struct ComponentSprite
{
    struct ComponentHeader header;
    Color color;
    b32 flipX;
    b32 flipY;
    enum sprite_layer layer;
    i16 order_in_layer;
    v2 pivot_override;
    AssetsSpriteFrameId frame_id;
    b32 draw_shadow;
    Color shadow_color;
    v2 shadow_offset;
    v2 shadow_scale;
};

struct ComponentAction
{
    struct ComponentHeader header;
    struct ActionRunner running_actions[MaxConcurrentActions];
};

struct ComponentTarget
{
    struct ComponentHeader header;
    entity_id_t entity_id;
};

struct Collider
{
    enum
    {
	CS_CIRCLE,
	CS_RECTANGLE
    } shape;
    union
    {
	struct
	{
	    f32 radius;
	    v2 offset;
	} circle;
	struct
	{
	    v2 size;
	    v2 offset;
	} rectangle;
    } props;
};

struct ComponentCollider
{
    struct ComponentHeader header;
    struct Collider *colliders;
    u16 num_colliders;
};

struct ComponentCamera
{
    struct ComponentHeader header;
    enum camera_type camera;
    f32 slowing_distance;
    v2 offset;
};

enum Direction
{
    D_UP,
    D_DOWN,
    D_LEFT,
    D_RIGHT,
    D_UP_LEFT,
    D_UP_RIGHT,
    D_DOWN_LEFT,
    D_DOWN_RIGHT,
    D_COUNT
};

struct ComponentPlayer
{
    struct ComponentHeader header;
    enum pitch_side side;
    struct StateMachinePlayer *state_machine;
    b32 control_ball;
    time_t time_spent_in_precision_control;
    time_t time_spent_in_run_control;
    f32 current_max_velocity;
    AssetsSpriteFrameId stand_frames[D_COUNT];
    AssetsAnimationId run_animations[D_COUNT];
    enum Direction last_direction;
    f32 last_kick_time;
    f32 last_frame_update_time;
    u16 last_running_frame_idx;
    u8 team_formation_position_idx;
};

struct ComponentBall
{
    struct ComponentHeader header;
    struct StateMachineBall *state_machine;
    b32 is_on_the_ground;
    entity_id_t last_player_on_the_ball_entity_id;
    entity_id_t last_kicker_entity_id;
    entity_id_t pass_target_entity_id;
    b32 subject_to_after_touch;
    v2 selected_after_touch_side;
    time_t last_kick_time;
    time_t last_bounce_time;
    AssetsSpriteFrameId roll_animations[D_COUNT];
    enum Direction last_direction;
    v2 future_bounce_position;
    u32 dts_to_future_bounce;
    v3 lookahead_positions[BALL_MAX_LOOKAHEADS];
    time_t last_frame_update_time;
    u16 last_running_frame_idx;
};

struct ComponentGoal
{
    struct ComponentHeader header;
    time_t hit_left_post_time;
    time_t hit_right_post_time;
    time_t hit_crossbar_time;
    b32 scored;
};

enum SaveSide
{
    SS_UP,
    SS_DOWN,
    SS_COUNT
};

enum SaveDirection
{
    SD_LEFT,
    SD_RIGHT,
    SD_COUNT
};

enum SaveHeight
{
    SH_HIGH,
    SH_LOW,
    SH_COUNT
};

enum SaveType
{
    ST_WITHOUT_BALL,
    ST_WITH_BALL,
    ST_COUNT
};

enum AnimationPhase
{
    AP_START,
    AP_END,
    AP_COUNT
};

struct ComponentGoalie
{
    struct ComponentHeader header;
    struct StateMachineGoalie *state_machine;
    enum SaveDirection dive_direction;
    enum SaveHeight dive_height;
    f32 dive_distance;
    f32 dive_speed;
    AssetsSpriteFrameId save_frames[SS_COUNT][ST_COUNT];
    AssetsSpriteFrameId dash_frames[SS_COUNT][SD_COUNT][ST_COUNT];
    AssetsAnimationId dive_animations_start[SS_COUNT][SD_COUNT][SH_COUNT];
    AssetsAnimationId dive_animations_end[SS_COUNT][SD_COUNT][SH_COUNT][ST_COUNT];
    time_t last_frame_update_time;
    time_t last_save_time;
};

running_action_idx_t components_action_run(entity_id_t id, struct Action *action,
					   struct game_state *game_state);
void components_action_stop(entity_id_t id, running_action_idx_t running_action_idx,
			    struct game_state *game_state);

void components_transform_calculate_absolute(struct ComponentTransform *transform,
					     Vector2 *position, f32 *rotation, Vector2 *scale);

#endif // !COMPONENTS
