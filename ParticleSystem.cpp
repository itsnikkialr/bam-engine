#include "ParticleSystem.h"

void ParticleSystem::OnStart() {
    if (started) return;
    
    // Clamp values as per spec
    if (frames_between_bursts < 1) frames_between_bursts = 1;
    if (burst_quantity < 1) burst_quantity = 1;
    if (duration_frames < 1) duration_frames = 1;
    
    // Test Suite #0 seeds
    emit_angle_distribution = new RandomEngine(emit_angle_min, emit_angle_max, 298);
    emit_radius_distribution = new RandomEngine(emit_radius_min, emit_radius_max, 404);
    
    // Test Suite #1 seeds
    rotation_distribution = new RandomEngine(rotation_min, rotation_max, 440);
    scale_distribution = new RandomEngine(start_scale_min, start_scale_max, 494);
    
    // Test Suite #2 seeds
    speed_distribution = new RandomEngine(start_speed_min, start_speed_max, 498);
    rotation_speed_distribution = new RandomEngine(rotation_speed_min, rotation_speed_max, 305);
    
    started = true;
}

void ParticleSystem::OnUpdate() {
    if (!enabled) return;
    
    if (is_playing && (local_frame_number % frames_between_bursts == 0)) {
        DoBurst();
    }
    
    const std::string& texture_name = image.empty() ? "default_particle" : image;
    const size_t count = particles.size();
    
    for (size_t i = 0; i < count; i++) {
        Particle& p = particles[i];
        if (!p.active) continue;
        
        if (p.frames_alive >= duration_frames) {
            p.active = false;
            free_list.push(i);
            continue;
        }
        
        p.vel_x += gravity_scale_x;
        p.vel_y += gravity_scale_y;
        p.vel_x *= drag_factor;
        p.vel_y *= drag_factor;
        p.x += p.vel_x;
        p.y += p.vel_y;
        p.rotation_speed *= angular_drag_factor;
        p.rotation += p.rotation_speed;
        
        if (end_scale >= 0.0f || end_color_r >= 0 || end_color_g >= 0 || end_color_b >= 0 || end_color_a >= 0) {
            float t = static_cast<float>(p.frames_alive) / static_cast<float>(duration_frames);
            if (end_scale >= 0.0f) {
                p.scale = p.start_scale + (end_scale - p.start_scale) * t;
            }
            if (end_color_r >= 0) p.color_r = static_cast<int>(p.start_color_r + (end_color_r - p.start_color_r) * t);
            if (end_color_g >= 0) p.color_g = static_cast<int>(p.start_color_g + (end_color_g - p.start_color_g) * t);
            if (end_color_b >= 0) p.color_b = static_cast<int>(p.start_color_b + (end_color_b - p.start_color_b) * t);
            if (end_color_a >= 0) p.color_a = static_cast<int>(p.start_color_a + (end_color_a - p.start_color_a) * t);
        }
        
        ImageDB::DrawEx(texture_name, p.x, p.y, p.rotation, p.scale, p.scale,
                        0.5f, 0.5f, p.color_r, p.color_g, p.color_b, p.color_a, sorting_order);
        
        p.frames_alive++;
    }
    
    local_frame_number++;
}

void ParticleSystem::DoBurst() {
    for (int i = 0; i < burst_quantity; i++) {
        EmitParticle();
    }
}

void ParticleSystem::EmitParticle() {
    Particle p;
    p.active = true;
    p.frames_alive = 0;
    
    float angle_radians = glm::radians(emit_angle_distribution->Sample());
    float radius = emit_radius_distribution->Sample();
    
    float cos_angle = glm::cos(angle_radians);
    float sin_angle = glm::sin(angle_radians);
    
    p.x = x + cos_angle * radius;
    p.y = y + sin_angle * radius;
    
    p.scale = scale_distribution->Sample();
    p.start_scale = p.scale;
    p.rotation = rotation_distribution->Sample();
    
    p.color_r = start_color_r;
    p.color_g = start_color_g;
    p.color_b = start_color_b;
    p.color_a = start_color_a;
    p.start_color_r = start_color_r;
    p.start_color_g = start_color_g;
    p.start_color_b = start_color_b;
    p.start_color_a = start_color_a;
    
    float speed = speed_distribution->Sample();
    // Use same angle as position for velocity direction
    p.vel_x = cos_angle * speed;
    p.vel_y = sin_angle * speed;
    
    p.rotation_speed = rotation_speed_distribution->Sample();
    
    if (!free_list.empty()) {
        int idx = free_list.front();
        free_list.pop();
        particles[idx] = p;
    } else {
        particles.push_back(p);
    }
}

void ParticleSystem::Play() {
    is_playing = true;
}

void ParticleSystem::Stop() {
    is_playing = false;
}

void ParticleSystem::Burst() {
    DoBurst();
}
