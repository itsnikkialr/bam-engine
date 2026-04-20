#ifndef COLLISION_H
#define COLLISION_H

#include "box2d/box2d.h"

class Actor;

struct Collision {
    Actor* other = nullptr;
    b2Vec2 point = b2Vec2(-999.0f, -999.0f);
    b2Vec2 relative_velocity = b2Vec2(0.0f, 0.0f);
    b2Vec2 normal = b2Vec2(-999.0f, -999.0f);
};

#endif
