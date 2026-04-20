#ifndef IMAGEDB_H
#define IMAGEDB_H

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"

#include "SDL2_image/SDL_image.h"

#include "Renderer.h"
#include "Helper.h"

struct ImageDrawRequest
{
    std::string image_name;
    float x = 0;
    float y = 0;
    int rotation_degrees = 0;
    float scale_x = 1;
    float scale_y = 1;
    float pivot_x = 0.5f;
    float pivot_y = 0.5f;
    int r = 255;
    int g = 255;
    int b = 255;
    int a = 255;
    int sorting_order = 0;
};

class ImageDB {
private:
	// actors
	std::unordered_map<std::string, SDL_Texture*> actor_imgs = {};
    static bool compare_image_requests(const ImageDrawRequest& a, const ImageDrawRequest& b);

public:
	// helpers
	void LoadActorImages(SDL_Renderer* sdl_renderer, const rapidjson::Document& doc, const char* key);
	SDL_Texture* GetActorImage(const std::string& name);
    void LoadActorImageByName(SDL_Renderer* sdl_renderer, const std::string& image_name);
    
    static inline std::vector<ImageDrawRequest> image_draw_request_queue; // scene
    static inline std::vector<ImageDrawRequest> ui_draw_request_queue; // ui
    static inline std::vector<ImageDrawRequest> pixel_draw_request_queue; // pixel
    
    // ui
    static void DrawUI(const std::string& image_name, float x, float y);
    static void DrawUIEx(const std::string& image_name,
                         float x, float y, float r, float g, float b, float a,
                         float sorting_order);
    static void Draw(const std::string& image_name, float x, float y);
    static void DrawEx(const std::string& image_name,
                       float x, float y, float rotation_degrees, float scale_x, float scale_y,
                       float pivot_x, float pivot_y, float r, float g, float b, float a,
                       float sorting_order);
    void RenderAndClearUI(SDL_Renderer* renderer);
    void RenderAndClearImages(SDL_Renderer* renderer);
    
    // pixel
    static void DrawPixel(float x, float y, float r, float g, float b, float a);
    void RenderAndClearPixels(SDL_Renderer* renderer, int clr_r, int clr_g, int clr_b);
    
    void CreateDefaultParticleTexture();

};

#endif
