#include "ContactListener.h"
#include "Actor.h"

void ContactListener::BeginContact(b2Contact* contact) {
    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Fixture* fixtureB = contact->GetFixtureB();

    Actor* actorA = reinterpret_cast<Actor*>(fixtureA->GetUserData().pointer);
    Actor* actorB = reinterpret_cast<Actor*>(fixtureB->GetUserData().pointer);

    if (!actorA || !actorB) return;

    bool is_trigger = fixtureA->IsSensor() && fixtureB->IsSensor();

    CollisionEvent event;
    event.actorA = actorA;
    event.actorB = actorB;
    event.is_begin = true;
    event.is_trigger = is_trigger;

    if (is_trigger) {
        event.point = b2Vec2(-999.0f, -999.0f);
        event.normal = b2Vec2(-999.0f, -999.0f);
    } else {
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        event.point = manifold.points[0];
        event.normal = manifold.normal;
    }

    event.relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();
    events.push_back(event);
}

void ContactListener::EndContact(b2Contact* contact) {
    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Fixture* fixtureB = contact->GetFixtureB();

    Actor* actorA = reinterpret_cast<Actor*>(fixtureA->GetUserData().pointer);
    Actor* actorB = reinterpret_cast<Actor*>(fixtureB->GetUserData().pointer);

    if (!actorA || !actorB) return;

    bool is_trigger = fixtureA->IsSensor() && fixtureB->IsSensor();

    CollisionEvent event;
    event.actorA = actorA;
    event.actorB = actorB;
    event.point = b2Vec2(-999.0f, -999.0f);
    event.normal = b2Vec2(-999.0f, -999.0f);
    event.relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();
    event.is_begin = false;
    event.is_trigger = is_trigger;

    events.push_back(event);
}
