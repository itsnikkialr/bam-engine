#include "ImageDB.h"

namespace fs = std::filesystem;

// helpers
bool ImageDB::compare_image_requests(const ImageDrawRequest& a, const ImageDrawRequest& b) {
    return a.sorting_order < b.sorting_order;
}


// actors
void ImageDB::LoadActorImageByName(SDL_Renderer* sdl_renderer, const std::string& image_name) {
    if (image_name.empty()) return;

    if (actor_imgs.find(image_name) != actor_imgs.end()) {
        return;
    }

    std::string image_path = "resources/images/" + image_name + ".png";

    if (!fs::exists(image_path)) {
        std::cout << "error: missing image " << image_name;
        exit(0);
    }

    SDL_Texture* tex = IMG_LoadTexture(sdl_renderer, image_path.c_str());
    actor_imgs.emplace(image_name, tex);
}

void ImageDB::LoadActorImages(SDL_Renderer* sdl_renderer, const rapidjson::Document& doc, const char* key) {
	if (!doc.HasMember("actors") || !doc["actors"].IsArray()) {
		return;
	}

	const auto& actors = doc[key];
	
	for (auto& actor : actors.GetArray()) {

		// <item>.png
		if (!actor.HasMember("view_image") || std::string(actor["view_image"].GetString()).empty()) {
			continue; // blank image for player? bug?
		}
        
        // bakc image
        if (actor.HasMember("view_image_back")) {
            std::string back_png_name = std::string(actor["view_image_back"].GetString());
            if (actor_imgs.find(back_png_name) == actor_imgs.end()) {
                std::string back_image_path = "resources/images/" + back_png_name + ".png";
                actor_imgs.emplace(back_png_name, IMG_LoadTexture(sdl_renderer, back_image_path.c_str()));
            }
        }
        
        // damage img
        if (actor.HasMember("view_image_damage")) {
            std::string dmg_png_name = std::string(actor["view_image_damage"].GetString());
            if (actor_imgs.find(dmg_png_name) == actor_imgs.end()) {
                std::string dmg_image_path = "resources/images/" + dmg_png_name + ".png";
                actor_imgs.emplace(dmg_png_name, IMG_LoadTexture(sdl_renderer, dmg_image_path.c_str()));
            }
        }
        
        // attack image
        if (actor.HasMember("view_image_attack")) {
            std::string atk_png_name = std::string(actor["view_image_attack"].GetString());
            if (actor_imgs.find(atk_png_name) == actor_imgs.end()) {
                std::string atk_image_path = "resources/images/" + atk_png_name + ".png";
                actor_imgs.emplace(atk_png_name, IMG_LoadTexture(sdl_renderer, atk_image_path.c_str()));
            }
        }
        

		std::string image_png_name = std::string(actor["view_image"].GetString());

		// already loaded? skip
		if (actor_imgs.find(image_png_name) != actor_imgs.end()) {
			continue;
		}

		std::string image_path = "resources/images/" + image_png_name + ".png";

		// if path doesn't exist, exit
		if (!fs::exists(image_path)) continue;

		// cache it !
		actor_imgs.emplace(image_png_name, IMG_LoadTexture(sdl_renderer, image_path.c_str()));
	}
}

SDL_Texture* ImageDB::GetActorImage(const std::string& name) {
    auto it = actor_imgs.find(name);
    if (it != actor_imgs.end()) return it->second;
    
    // Check for default particle 
    if (name == "default_particle") {
        CreateDefaultParticleTexture();
        return actor_imgs[name];
    }
    
    // lazy load
    std::string path = "resources/images/" + name + ".png";
    if (!fs::exists(path)) {
        std::cout << "error: missing image " << name;
        exit(0);
    }
    
    SDL_Texture* tex = IMG_LoadTexture(Renderer::GetSDLRenderer(), path.c_str());
    actor_imgs[name] = tex;
    return tex;
}



// Lua Funcs
void ImageDB::DrawUI(const std::string &image_name, float x, float y) {
    ImageDrawRequest draw_ui_img;
    draw_ui_img.image_name = image_name;
    draw_ui_img.x = static_cast<int>(x);
    draw_ui_img.y = static_cast<int>(y);
    draw_ui_img.sorting_order = 0; // default
    ui_draw_request_queue.emplace_back(draw_ui_img);
}

void ImageDB::DrawUIEx(const std::string &image_name,
                       float x, float y, float r, float g, float b, float a,
                       float sorting_order) {
    ImageDrawRequest draw_ui_ex_img;
    draw_ui_ex_img.image_name = image_name;
    draw_ui_ex_img.x = static_cast<int>(x);
    draw_ui_ex_img.y = static_cast<int>(y);
    draw_ui_ex_img.r = static_cast<int>(r);
    draw_ui_ex_img.g = static_cast<int>(g);
    draw_ui_ex_img.b = static_cast<int>(b);
    draw_ui_ex_img.a = static_cast<int>(a);
    draw_ui_ex_img.sorting_order = static_cast<int>(sorting_order);
    
    ui_draw_request_queue.emplace_back(draw_ui_ex_img);
}

void ImageDB::Draw(const std::string& image_name, float x, float y) {
    ImageDrawRequest draw_img;
    draw_img.image_name = image_name;
    draw_img.x = x;
    draw_img.y = y;
    
    image_draw_request_queue.emplace_back(draw_img);
}

void ImageDB::DrawEx(const std::string& image_name,
                     float x, float y, float rotation_degrees, float scale_x, float scale_y,
                     float pivot_x, float pivot_y, float r, float g, float b, float a,
                     float sorting_order) {
    ImageDrawRequest draw_img;
    draw_img.image_name = image_name;
    draw_img.x = x;
    draw_img.y = y;
    draw_img.rotation_degrees = static_cast<int>(rotation_degrees);
    draw_img.scale_x = scale_x;
    draw_img.scale_y = scale_y;
    draw_img.pivot_x = pivot_x;
    draw_img.pivot_y = pivot_y;
    draw_img.r = static_cast<int>(r);
    draw_img.g = static_cast<int>(g);
    draw_img.b = static_cast<int>(b);
    draw_img.a = static_cast<int>(a);
    draw_img.sorting_order = static_cast<int>(sorting_order);
    
    image_draw_request_queue.emplace_back(draw_img);
}

void ImageDB::DrawPixel(float x, float y, float r, float g, float b, float a) {
    ImageDrawRequest pixel_img;
    pixel_img.x = static_cast<int>(x);
    pixel_img.y = static_cast<int>(y);
    pixel_img.r = static_cast<int>(r);
    pixel_img.g = static_cast<int>(g);
    pixel_img.b = static_cast<int>(b);
    pixel_img.a = static_cast<int>(a);
    
    pixel_draw_request_queue.emplace_back(pixel_img);
}

// renders
void ImageDB::RenderAndClearUI(SDL_Renderer* renderer) {
    std::stable_sort(ui_draw_request_queue.begin(), ui_draw_request_queue.end(), compare_image_requests);
    
    for (auto& request : ui_draw_request_queue) {
            SDL_Texture* tex = GetActorImage(request.image_name);
            if (!tex) continue;

            SDL_FRect tex_rect;
            Helper::SDL_QueryTexture(tex, &tex_rect.w, &tex_rect.h);
            tex_rect.x = static_cast<int>(request.x);
            tex_rect.y = static_cast<int>(request.y);

            SDL_SetTextureColorMod(tex, request.r, request.g, request.b);
            SDL_SetTextureAlphaMod(tex, request.a);

            Helper::SDL_RenderCopy(renderer, tex, NULL, &tex_rect);

            SDL_SetTextureColorMod(tex, 255, 255, 255);
            SDL_SetTextureAlphaMod(tex, 255);
        }

        ui_draw_request_queue.clear();
}

void ImageDB::RenderAndClearPixels(SDL_Renderer* renderer, int clr_r, int clr_g, int clr_b) {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (auto& req : pixel_draw_request_queue) {
        SDL_SetRenderDrawColor(renderer, req.r, req.g, req.b, req.a);
        SDL_RenderDrawPoint(renderer, static_cast<int>(req.x), static_cast<int>(req.y));
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, clr_r, clr_g, clr_b, 255);
    pixel_draw_request_queue.clear();
}

void ImageDB::RenderAndClearImages(SDL_Renderer* renderer) {
    std::stable_sort(image_draw_request_queue.begin(), image_draw_request_queue.end(), compare_image_requests);

    float zoom_factor = Renderer::GetCameraZoomFactor();
    SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);

    for (auto& request : image_draw_request_queue) {
        const int pixels_per_meter = 100;
        glm::vec2 final_rendering_position = glm::vec2(request.x, request.y) - Renderer::GetCameraPosition();

        SDL_Texture* tex = GetActorImage(request.image_name);
        if (!tex) continue;

        SDL_FRect tex_rect;
        Helper::SDL_QueryTexture(tex, &tex_rect.w, &tex_rect.h);

        // Apply scale
        int flip_mode = SDL_FLIP_NONE;
        if (request.scale_x < 0)
            flip_mode |= SDL_FLIP_HORIZONTAL;
        if (request.scale_y < 0)
            flip_mode |= SDL_FLIP_VERTICAL;

        float x_scale = glm::abs(request.scale_x);
        float y_scale = glm::abs(request.scale_y);

        tex_rect.w *= x_scale;
        tex_rect.h *= y_scale;

        SDL_FPoint pivot_point = {
            request.pivot_x * tex_rect.w,
            request.pivot_y * tex_rect.h
        };

        glm::ivec2 cam_dimensions = Renderer::GetCameraDimensions();

        tex_rect.x = final_rendering_position.x * pixels_per_meter
                      + cam_dimensions.x * 0.5f * (1.0f / zoom_factor) - pivot_point.x;
        tex_rect.y = final_rendering_position.y * pixels_per_meter
                      + cam_dimensions.y * 0.5f * (1.0f / zoom_factor) - pivot_point.y;

        // Apply tint / alpha to texture
        SDL_SetTextureColorMod(tex, request.r, request.g, request.b);
        SDL_SetTextureAlphaMod(tex, request.a);

        // Perform Draw
        SDL_FRect dst_rect = {(float)tex_rect.x, (float)tex_rect.y, (float)tex_rect.w, (float)tex_rect.h};
        SDL_FPoint pivot_fpoint = {(float)pivot_point.x, (float)pivot_point.y};

        Helper::SDL_RenderCopyEx(0, "", renderer, tex, NULL,
            &dst_rect,
            request.rotation_degrees,
            &pivot_fpoint,
            static_cast<SDL_RendererFlip>(flip_mode));

        SDL_RenderSetScale(renderer, zoom_factor, zoom_factor);

        // Remove tint / alpha from texture
        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }

    SDL_RenderSetScale(renderer, 1, 1);
    image_draw_request_queue.clear();
}

void ImageDB::CreateDefaultParticleTexture() {
    std::string name = "default_particle";
    
    if (actor_imgs.find(name) != actor_imgs.end())
        return;
    
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);
    
    // Ensure color set to white (255, 255, 255, 255)
    Uint32 white_color = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
    SDL_FillRect(surface, NULL, white_color);
    
    // Create a gpu-side texture from the cpu-side surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(Renderer::GetSDLRenderer(), surface);
    
    // Clean up the surface and cache this default texture
    SDL_FreeSurface(surface);
    actor_imgs[name] = texture;  
}
