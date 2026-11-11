#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_events.h>
#include "Input.h"

static SDL_Scancode GetSDLScanCode(Keys key){
    switch (key){
        case Keys::W     : return SDL_SCANCODE_W;
        case Keys::A     : return SDL_SCANCODE_A;
        case Keys::S     : return SDL_SCANCODE_S;
        case Keys::D     : return SDL_SCANCODE_D;
        case Keys::E     : return SDL_SCANCODE_E;        
        case Keys::Q     : return SDL_SCANCODE_Q;
        case Keys::R     : return SDL_SCANCODE_R;
        case Keys::F     : return SDL_SCANCODE_F;
        case Keys::SPACE : return SDL_SCANCODE_SPACE;

        default :     return SDL_SCANCODE_UNKNOWN;
    }
}

static SDL_GameControllerButton GetSDLGamepadButton(Gamepad button){
    switch (button){
        case Gamepad::DpadUp    : return   SDL_CONTROLLER_BUTTON_DPAD_UP;
        case Gamepad::DpadDown  : return   SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        case Gamepad::DpadLeft  : return   SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        case Gamepad::DpadRight : return   SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        case Gamepad::L1        : return   SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        case Gamepad::R1        : return   SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        case Gamepad::A         : return   SDL_CONTROLLER_BUTTON_A;
        case Gamepad::B         : return   SDL_CONTROLLER_BUTTON_B;
        case Gamepad::X         : return   SDL_CONTROLLER_BUTTON_X;
        case Gamepad::Y         : return   SDL_CONTROLLER_BUTTON_Y;
        case Gamepad::Start     : return   SDL_CONTROLLER_BUTTON_START;
        case Gamepad::Select    : return   SDL_CONTROLLER_BUTTON_BACK;

        default                 : return SDL_CONTROLLER_BUTTON_INVALID;
    }
}

Input::Input(){
    sdlKey = nullptr;
    numKey = 0;
    controller = nullptr;

    prevGamepadState.resize(SDL_CONTROLLER_BUTTON_MAX, 0);
}

Input::~Input(){
    if(controller != nullptr){
        SDL_GameControllerClose(controller);
    }
}

void Input::Init(){
    virtualKeyState.resize((int)Keys::SPACE + 1, 0);
    sdlKey = SDL_GetKeyboardState(&numKey);

    if(sdlKey != nullptr){
        previousKeyState.resize(numKey, 0);
    }

    controller = nullptr;
    if(SDL_NumJoysticks() > 0){
        controller = SDL_GameControllerOpen(0);
    }
}

void Input::Update(){
    if(sdlKey != nullptr){
        previousKeyState.assign(sdlKey, sdlKey + numKey);
    }

    if(controller != nullptr){
        for(int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; i++){
            prevGamepadState[i] = SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)i);
        }
    }
}

bool Input::isKeyDown(Keys key){
    int index = (int)key;
    if(index >= 0 && index < (int)virtualKeyState.size() && virtualKeyState[index]) return true;

    SDL_Scancode keyCode = GetSDLScanCode(key);
    if(keyCode == SDL_SCANCODE_UNKNOWN || sdlKey == nullptr) return false;

    return sdlKey[keyCode];
}

void Input::SetVirtualKey(Keys key, bool down){
    int index = (int)key;
    if(index >= 0 && index < (int)virtualKeyState.size()){
        virtualKeyState[index] = down ? 1 : 0;
    }
}

bool Input::isKeyPressed(Keys key){
    SDL_Scancode keyCode = GetSDLScanCode(key);
    if(keyCode == SDL_SCANCODE_UNKNOWN || sdlKey == nullptr) return false;

    bool isDown  = sdlKey[keyCode] != 0;
    bool wasDown = previousKeyState[keyCode] != 0;

    return isDown && !wasDown;
}

bool Input::isGamepadDown(Gamepad button){
    if(!controller) return false;
    SDL_GameControllerButton sdlButton = GetSDLGamepadButton(button);
    if(sdlButton == SDL_CONTROLLER_BUTTON_INVALID) return false;

    return SDL_GameControllerGetButton(controller, sdlButton);
}

bool Input::isGamepadPressed(Gamepad button){
    if(!controller) return false;
    SDL_GameControllerButton sdlButton = GetSDLGamepadButton(button);
    if(sdlButton == SDL_CONTROLLER_BUTTON_INVALID) return false;

    bool isDown  = SDL_GameControllerGetButton(controller, sdlButton);
    bool wasDown = prevGamepadState[sdlButton] != 0;

    return isDown && !wasDown;
}
