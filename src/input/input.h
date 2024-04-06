#ifndef INPUT_H
#define INPUT_H

#include "../types.h"
#include <raylib.h>

// GameState forward declaration
struct GameState;

enum InputButtonType {
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

typedef struct InputButton {
    b32 down;
    b32 up;
    b32 pressed;
    b32 released;
} InputButton;

typedef struct Input {
    Vector2 left_stick;
    Vector2 right_stick;
    f32 left_trigger;
    f32 right_trigger;
    InputButton buttons[IBT_BUTTON_COUNT];
} Input;

Input input_handle(struct GameState *game_state);
#endif // !INPUT_H
