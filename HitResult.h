#ifndef HITRESULT_H
#define HITRESULT_H

#include "box2d/box2d.h"

class Actor;

struct HitResult {
    Actor* actor = nullptr;
    b2Vec2 point = b2Vec2(0.0f, 0.0f);
    b2Vec2 normal = b2Vec2(0.0f, 0.0f);
    bool is_trigger = false;
    float fraction = 0.0f; 
};

#endif
