#ifndef RENDERER_H
#define RENDERER_H

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <string>
#include <cstdlib>
#include <iostream>

#include "SDL2/SDL.h"
#include "glm/glm.hpp"
#include "Helper.h"

struct SDL_Window;
struct SDL_Renderer;

class Renderer {
public:
    Renderer();
    ~Renderer();

    void Init(const std::string& title, int w, int h, int r, int g, int b);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    // cam vars
    static inline float cam_x = 0.0f;
    static inline float cam_y = 0.0f;
    static inline float zoom_factor = 1.0f;
    static inline int win_w = 640;
    static inline int win_h = 360;
    static inline SDL_Renderer* sdl_renderer_ptr = nullptr;

    // static getters
    static glm::vec2 GetCameraPosition() { return glm::vec2(cam_x, cam_y); }
    static float GetCameraZoomFactor() { return zoom_factor; }
    static glm::ivec2 GetCameraDimensions() { return glm::ivec2(win_w, win_h); }
    static SDL_Renderer* GetSDLRenderer() { return sdl_renderer_ptr; }

    // setters
    static void SetCameraPosition(float x, float y) { cam_x = x; cam_y = y; }
    static void SetZoomFactor(float z) { zoom_factor = z; }

private:
    int clr_r = 255, clr_g = 255, clr_b = 255;
};

#endif
