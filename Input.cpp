#include "Input.h"

void Input::Init() {
    // set mapping to up state for all keys at the start
    for (int code = SDL_SCANCODE_UNKNOWN; code < SDL_NUM_SCANCODES; code++) {
        keyboard_states[static_cast<SDL_Scancode>(code)] = INPUT_STATE_UP;
    }
    
    for (int i = 1; i < 4; i++) {
        mouse_button_states[i] = INPUT_STATE_UP;
    }
}

void Input::ProcessEvent(const SDL_Event & e,
                         bool & saw_quit) {
    if (e.type == SDL_QUIT) {
        saw_quit = true;
    }
    // keyboard detection,
    else if (e.type == SDL_KEYDOWN) {
        auto sc = e.key.keysym.scancode;
        
        // KEY JUST BECAME DOWN
        keyboard_states[sc] = INPUT_STATE_JUST_BECAME_DOWN;
        just_became_down_scancodes.emplace_back(sc);

    }
    else if (e.type == SDL_KEYUP) {
        // KEY JUST BECAME UP
        keyboard_states[e.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_UP;
        just_became_up_scancodes.emplace_back(e.key.keysym.scancode);
    }
    
    else if (e.type == SDL_MOUSEBUTTONDOWN) {
        // BUTTON JUST BECAME DOWN
        mouse_button_states[e.button.button] = INPUT_STATE_JUST_BECAME_DOWN;
        just_became_down_buttons.emplace_back(e.button.button);
    }
    
    else if (e.type == SDL_MOUSEBUTTONUP) {
        // BUTTON JUST BECAME UP
        mouse_button_states[e.button.button] = INPUT_STATE_JUST_BECAME_UP;
        just_became_up_buttons.emplace_back(e.button.button);
    }
    
    else if (e.type == SDL_MOUSEMOTION) {
        // x and y vec
        mouse_position.x = e.motion.x;
        mouse_position.y = e.motion.y;
    }
    
    else if (e.type == SDL_MOUSEWHEEL) {
        // mouse scroll data
        mouse_scroll_delta = e.wheel.preciseY;
    }
}

void Input::LateUpdate() {
    // reset keyboard keys
    // set everything in map !
    for (const SDL_Scancode & code : just_became_down_scancodes) {
        keyboard_states[code] = INPUT_STATE_DOWN;
    }
    for (const SDL_Scancode & code : just_became_up_scancodes) {
        keyboard_states[code] = INPUT_STATE_UP;
    }
    
    just_became_down_scancodes.clear();
    just_became_up_scancodes.clear();
    
    // reset mouse buttons
    for (const int & num : just_became_down_buttons) {
        mouse_button_states[num] = INPUT_STATE_DOWN;
    }
    for (const int & num : just_became_up_buttons) {
        mouse_button_states[num] = INPUT_STATE_UP;
    }
    
    just_became_down_buttons.clear();
    just_became_up_buttons.clear();
    
    // reset scroll every frame
    mouse_scroll_delta = 0.0f;
}

bool Input::GetKey(std::string key_name) {
    auto keycode = __keycode_to_scancode.find(key_name);
    if (keycode == __keycode_to_scancode.end()) return false;
    return keyboard_states[keycode->second] == INPUT_STATE_DOWN || keyboard_states[keycode->second] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetKeyDown(std::string key_name) {
    auto keycode = __keycode_to_scancode.find(key_name);
    if (keycode == __keycode_to_scancode.end()) return false;
    return keyboard_states[keycode->second] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetKeyUp(std::string key_name) {
    auto keycode = __keycode_to_scancode.find(key_name);
    if (keycode == __keycode_to_scancode.end()) return false;
    return keyboard_states[keycode->second] == INPUT_STATE_JUST_BECAME_UP;
}

glm::vec2 Input::GetMousePosition() {
    return mouse_position;
}

bool Input::GetMouseButton(int button) {
    return mouse_button_states[button] == INPUT_STATE_DOWN || mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetMouseButtonDown(int button) {
    return mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetMouseButtonUp(int button) {
    return mouse_button_states[button] == INPUT_STATE_JUST_BECAME_UP;
}

float Input::GetMouseScrollDelta() {
    return mouse_scroll_delta;
}

void Input::HideCursor() {
    SDL_ShowCursor(SDL_DISABLE);
}

void Input::ShowCursor() {
    SDL_ShowCursor(SDL_ENABLE);
}
