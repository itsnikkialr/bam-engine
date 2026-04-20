#include "Renderer.h"

Renderer::Renderer() {}

Renderer::~Renderer() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void Renderer::Init(const std::string& title, int w, int h, int r, int g, int b) {
    clr_r = r;
    clr_g = g;
    clr_b = b;
    win_w = w;
    win_h = h;

    window = Helper::SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w,
        h,
        SDL_WINDOW_SHOWN
    );

    renderer = Helper::SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );

    // store static pointer so ImageDB can access it
    sdl_renderer_ptr = renderer;
}
