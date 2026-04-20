#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include "glm/glm.hpp"

#include <string>
#include <iostream>
#include "box2d/box2d.h"
#include "ContactListener.h"

class Actor;

class Rigidbody {
public:
    Actor* actor = nullptr;
    b2Body* body = nullptr;

    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    std::string body_type = "dynamic";
    bool precise = true;
    float gravity_scale = 1.0f;
    float density = 1.0f;
    float angular_friction = 0.3f;
    float rotation = 0.0f;
    bool has_collider = true;
    bool has_trigger = true;
    bool started = false;
    bool enabled = true;
    std::string collider_type = "box";
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;

    void OnStart();
    b2Vec2 GetPosition() const;
    float GetRotation() const;

    static void SetWorld(b2World* w);
    static b2World* GetWorld();
    
    // manipulation
    void AddForce(const b2Vec2& force);
    void SetVelocity(const b2Vec2& vel);
    void SetPosition(const b2Vec2& pos);
    void SetRotation(float degrees_clockwise);
    void SetAngularVelocity(float degrees_clockwise);
    void SetGravityScale(float scale);
    void SetUpDirection(b2Vec2 dir);
    void SetRightDirection(b2Vec2 dir);

    // queries
    b2Vec2 GetVelocity() const;
    float GetAngularVelocity() const;
    float GetGravityScale() const;
    b2Vec2 GetUpDirection() const;
    b2Vec2 GetRightDirection() const;
    
    std::string trigger_type = "box";
    float trigger_width = 1.0f;
    float trigger_height = 1.0f;
    float trigger_radius = 0.5f;
    
    void OnDestroy();

private:
    static b2World* physics_world;
};

#endif
