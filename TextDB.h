#ifndef TEXTDB_H
#define TEXTDB_H

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <string>
#include <vector>
#include <unordered_map>

#include "SDL2_ttf/SDL_ttf.h"
#include "Helper.h"

class TextDB {
private:
    struct TextRequest {
        std::string text;
        std::string font_name;
        int font_size;
        SDL_Color color;
        int x, y;
    };

    // font cache: font_name -> size -> TTF_Font*
    std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> font_cache;

    // helper to load or retrieve a font
    TTF_Font* GetFont(const std::string& font_name, int font_size);

public:
    TextDB();
    ~TextDB();
    // queue of text draw calls for this frame
    static std::vector<TextRequest> text_requests;

    // called from Lua API (Text.Draw)
    static void DrawText(const std::string& text, int font_size, SDL_Color color, int x, int y);
    static void QueueDraw(const std::string& text, int x, int y,
                          const std::string& font_name, int font_size,
                          int r, int g, int b, int a);

    // called at end of frame
    void Flush(SDL_Renderer* renderer);
};

#endif
