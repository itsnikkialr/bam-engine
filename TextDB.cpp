#include "TextDB.h"

namespace fs = std::filesystem;

std::vector<TextDB::TextRequest> TextDB::text_requests = {};

TextDB::TextDB() {
	TTF_Init();
}

TextDB::~TextDB() {
}


void TextDB::QueueDraw(const std::string& text, int x, int y,
                      const std::string& font_name, int font_size,
                      int r, int g, int b, int a) {
    TextDB::text_requests.push_back({text, font_name, font_size, {(Uint8)r,(Uint8)g,(Uint8)b,(Uint8)a}, x, y});
}

void TextDB::Flush(SDL_Renderer* renderer) {
    for (const auto& req : text_requests) {
        TTF_Font* font = GetFont(req.font_name, req.font_size);
        if (!font) continue;
        
        SDL_Surface* surf = TTF_RenderText_Solid(font, req.text.c_str(), req.color);
        if (!surf) continue;
        
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (!tex) continue;
        
        float w = 0, h = 0;
        Helper::SDL_QueryTexture(tex, &w, &h);
        SDL_FRect dst{ static_cast<float>(req.x), static_cast<float>(req.y), w, h };
        Helper::SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    text_requests.clear();
}

TTF_Font* TextDB::GetFont(const std::string& font_name, int font_size) {
    auto& size_map = font_cache[font_name];
    auto it = size_map.find(font_size);
    if (it != size_map.end()) return it->second;
    
    std::string path = "resources/fonts/" + font_name + ".ttf";
    TTF_Font* font = TTF_OpenFont(path.c_str(), font_size);
    size_map[font_size] = font;
    return font;
}
