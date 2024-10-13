#ifndef INPUT_H
#define INPUT_H

#include "types.h"
#include <raylib.h>

// GameState forward declaration
struct game_state;

enum input_button_type
{
    IBT_BUTTON_START,
    IBT_BUTTON_SELECT,
    IBT_BUTTON_DOWN,
    IBT_BUTTON_RIGHT,
    IBT_BUTTON_LEFT,
    IBT_BUTTON_UP,
    IBT_BUTTON_TRIGGER_LEFT,
    IBT_BUTTON_TRIGGER_RIGHT,
    IBT_BUTTON_COUNT
};

struct input_button
{
    b32 down;
    b32 up;
    b32 pressed;
    b32 released;
};

struct input
{
    v2 left_stick;
    v2 right_stick;
    f32 left_trigger;
    f32 right_trigger;
    struct input_button buttons[IBT_BUTTON_COUNT];
};

struct input input_handle(struct game_state *game_state);

#endif // !INPUT_H
