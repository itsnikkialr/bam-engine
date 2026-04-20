#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <vector>
#include <queue>
#include <string>
#include "Helper.h"
#include "glm/glm.hpp"

#include "ImageDB.h"
#include "Renderer.h"
#include "Actor.h"

class Actor;

struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    bool active = false;
    int frames_alive = 0;
    
    // For later test suites
    float scale = 1.0f;
    float rotation = 0.0f;
    float vel_x = 0.0f;
    float vel_y = 0.0f;
    float rotation_speed = 0.0f;
    
    // Starting values (for interpolation later)
    float start_scale = 1.0f;
    int start_color_r = 255;
    int start_color_g = 255;
    int start_color_b = 255;
    int start_color_a = 255;
    
    // Current color
    int color_r = 255;
    int color_g = 255;
    int color_b = 255;
    int color_a = 255;
};

class ParticleSystem {
public:
    Actor* actor = nullptr;
    bool enabled = true;
    bool started = false;
    
    // Position offset (Test Suite #1)
    float x = 0.0f;
    float y = 0.0f;
    
    // Emission shape (Test Suite #0)
    float emit_radius_min = 0.0f;
    float emit_radius_max = 0.5f;
    float emit_angle_min = 0.0f;
    float emit_angle_max = 360.0f;
    
    // Burst settings (Test Suite #1)
    int frames_between_bursts = 1;
    int burst_quantity = 1;
    
    // Scale (Test Suite #1)
    float start_scale_min = 1.0f;
    float start_scale_max = 1.0f;
    
    // Rotation (Test Suite #1)
    float rotation_min = 0.0f;
    float rotation_max = 0.0f;
    
    // Color (Test Suite #1)
    int start_color_r = 255;
    int start_color_g = 255;
    int start_color_b = 255;
    int start_color_a = 255;
    
    // Image/Sorting (Test Suite #1)
    std::string image = "";
    int sorting_order = 9999;
    
    // Duration (Test Suite #2)
    int duration_frames = 300;
    
    // Speed (Test Suite #2)
    float start_speed_min = 0.0f;
    float start_speed_max = 0.0f;
    
    // Rotation speed (Test Suite #2)
    float rotation_speed_min = 0.0f;
    float rotation_speed_max = 0.0f;
    
    // Gravity (Test Suite #2)
    float gravity_scale_x = 0.0f;
    float gravity_scale_y = 0.0f;
    
    // Drag (Test Suite #2)
    float drag_factor = 1.0f;
    float angular_drag_factor = 1.0f;
    
    // End values for interpolation (Test Suite #2)
    float end_scale = -1.0f;  // -1 means "not configured"
    int end_color_r = -1;
    int end_color_g = -1;
    int end_color_b = -1;
    int end_color_a = -1;
    
    // Lifecycle
    void OnStart();
    void OnUpdate();
    
    // Runtime control (Test Suite #3)
    void Play();
    void Stop();
    void Burst();
    
private:
    int local_frame_number = 0;
    bool is_playing = true;
    
    std::vector<Particle> particles;
    std::queue<int> free_list;
    
    // RandomEngines - created once in OnStart()
    RandomEngine* emit_angle_distribution = nullptr;
    RandomEngine* emit_radius_distribution = nullptr;
    RandomEngine* scale_distribution = nullptr;
    RandomEngine* rotation_distribution = nullptr;
    RandomEngine* speed_distribution = nullptr;
    RandomEngine* rotation_speed_distribution = nullptr;
    
    void EmitParticle();
    void DoBurst();
};

#endif
