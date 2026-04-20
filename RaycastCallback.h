#ifndef RAYCASTCALLBACK_H
#define RAYCASTCALLBACK_H

#include "box2d/box2d.h"
#include "HitResult.h"
#include <vector>

class RaycastCallback : public b2RayCastCallback {
public:
    std::vector<HitResult> hits;

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        Actor* actor = reinterpret_cast<Actor*>(fixture->GetUserData().pointer);
        if (!actor) return -1;

        // skip phantom fixtures
        if (fixture->GetFilterData().categoryBits == 0x0000) return -1;

        HitResult hit;
        hit.actor = actor;
        hit.point = point;
        hit.normal = normal;
        hit.is_trigger = fixture->IsSensor();
        hit.fraction = fraction;

        hits.push_back(hit);
        return 1; // continue raycast to find all fixtures
    }
};

#endif
