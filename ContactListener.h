#ifndef CONTACTLISTENER_H
#define CONTACTLISTENER_H

#include "box2d/box2d.h"
#include <vector>

class RigidBody;

struct CollisionEvent {
    class Actor* actorA = nullptr;
    class Actor* actorB = nullptr;
    b2Vec2 point = b2Vec2(-999.0f, -999.0f);
    b2Vec2 relative_velocity = b2Vec2(0.0f, 0.0f);
    b2Vec2 normal = b2Vec2(-999.0f, -999.0f);
    bool is_begin = true;
    bool is_trigger = false;
};

class ContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override;
    void EndContact(b2Contact* contact) override;

    static inline std::vector<CollisionEvent> events;
};

#endif
