#include "Rigidbody.h"

b2World* Rigidbody::physics_world = nullptr;

void Rigidbody::SetWorld(b2World* w) {
    physics_world = w;
}

b2World* Rigidbody::GetWorld() {
    return physics_world;
}

void Rigidbody::OnStart() {
    if (started) return;
    
    if (!physics_world) {
        physics_world = new b2World(b2Vec2(0.0f, 9.8f));
        static ContactListener listener;
        physics_world->SetContactListener(&listener);
    }

    b2BodyDef body_def;
    body_def.position.Set(x, y);

    if (body_type == "dynamic") body_def.type = b2_dynamicBody;
    else if (body_type == "static") body_def.type = b2_staticBody;
    else if (body_type == "kinematic") body_def.type = b2_kinematicBody;
    else body_def.type = b2_dynamicBody;

    body_def.bullet = precise;
    body_def.gravityScale = gravity_scale;
    body_def.angularDamping = angular_friction;

    body_def.angle = rotation * (b2_pi / 180.0f);
    
//    std::cout << "Creating body: angular_friction=" << angular_friction
//              << " gravity_scale=" << gravity_scale
//              << " body_type=" << body_type << std::endl;

    body = physics_world->CreateBody(&body_def);

        if (!has_collider && !has_trigger) {
            b2PolygonShape shape;
            shape.SetAsBox(width * 0.5f, height * 0.5f);

            b2FixtureDef fixture_def;
            fixture_def.shape = &shape;
            fixture_def.density = density;
            fixture_def.isSensor = true;
            fixture_def.filter.categoryBits = 0x0000;
            fixture_def.filter.maskBits = 0x0000;
            fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
            
            body->CreateFixture(&fixture_def);
        } else {
            if (has_collider) {
                b2FixtureDef fixture_def;
                fixture_def.density = density;
                fixture_def.friction = friction;
                fixture_def.restitution = bounciness;
                fixture_def.isSensor = false;
                fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
                fixture_def.filter.categoryBits = 0x0001;  // collider category
                fixture_def.filter.maskBits = 0x0001;       // only collide with other colliders

                if (collider_type == "circle") {
                    b2CircleShape circle;
                    circle.m_radius = radius;
                    fixture_def.shape = &circle;
                    body->CreateFixture(&fixture_def);
                } else {
                    b2PolygonShape box;
                    box.SetAsBox(width * 0.5f, height * 0.5f);
                    fixture_def.shape = &box;
                    body->CreateFixture(&fixture_def);
                }
            }

            if (has_trigger) {
                b2FixtureDef fixture_def;
                fixture_def.density = density;
                fixture_def.isSensor = true;
                fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
                fixture_def.filter.categoryBits = 0x0002;  // trigger category
                fixture_def.filter.maskBits = 0x0002;       // only collide with other triggers

                if (trigger_type == "circle") {
                    b2CircleShape circle;
                    circle.m_radius = trigger_radius;
                    fixture_def.shape = &circle;
                    body->CreateFixture(&fixture_def);
                } else {
                    b2PolygonShape box;
                    box.SetAsBox(trigger_width * 0.5f, trigger_height * 0.5f);
                    fixture_def.shape = &box;
                    body->CreateFixture(&fixture_def);
                }
            }
            
        }


    started = true;
}

b2Vec2 Rigidbody::GetPosition() const {
    if (body) return body->GetPosition();
    return b2Vec2(x, y);
}

float Rigidbody::GetRotation() const {
    if (body) {
            float angle = body->GetAngle();
            //std::cout << "GetRot: rad=" << angle << std::endl;
            return angle * (180.0f / b2_pi);
        }
        return rotation;
}

void Rigidbody::AddForce(const b2Vec2& force) {
    if (!body) return;
    body->ApplyForceToCenter(force, true);
}

void Rigidbody::SetVelocity(const b2Vec2& vel) {
    if (!body) return;
    body->SetLinearVelocity(vel);
}

void Rigidbody::SetPosition(const b2Vec2& pos) {
    if (!body) {
        x = pos.x;
        y = pos.y;
        return;
    }
    body->SetTransform(pos, body->GetAngle());
}

void Rigidbody::SetRotation(float degrees_clockwise) {
    rotation = degrees_clockwise;
    if (!body) return;
    float radians = degrees_clockwise * (b2_pi / 180.0f);
    body->SetTransform(body->GetPosition(), radians);
}

void Rigidbody::SetAngularVelocity(float degrees_clockwise) {
    //std::cout << "SetAngVel: body=" << body << " deg=" << degrees_clockwise << std::endl;
    if (!body) return;
    float radians = degrees_clockwise * (b2_pi / 180.0f);
    body->SetAngularVelocity(radians);
}

void Rigidbody::SetGravityScale(float scale) {
    gravity_scale = scale;
    if (!body) return;
    body->SetGravityScale(scale);
}

void Rigidbody::SetUpDirection(b2Vec2 dir) {
    dir.Normalize();
    float angle = glm::atan(dir.x, -dir.y);
    if (!body) {
        rotation = angle * (180.0f / b2_pi);
        return;
    }
    body->SetTransform(body->GetPosition(), angle);
}

void Rigidbody::SetRightDirection(b2Vec2 dir) {
    dir.Normalize();
    float angle = glm::atan(dir.x, -dir.y) - (b2_pi / 2.0f);
    if (!body) {
        rotation = angle * (180.0f / b2_pi);
        return;
    }
    body->SetTransform(body->GetPosition(), angle);
}

b2Vec2 Rigidbody::GetVelocity() const {
    if (!body) return b2Vec2(0.0f, 0.0f);
    return body->GetLinearVelocity();
}

float Rigidbody::GetAngularVelocity() const {
    if (!body) return 0.0f;
    return body->GetAngularVelocity() * (180.0f / b2_pi);
}

float Rigidbody::GetGravityScale() const {
    if (!body) return gravity_scale;
    return body->GetGravityScale();
}

b2Vec2 Rigidbody::GetUpDirection() const {
    float angle = body ? body->GetAngle() : (rotation * (b2_pi / 180.0f));
    return b2Vec2(glm::sin(angle), -glm::cos(angle));
}

b2Vec2 Rigidbody::GetRightDirection() const {
    float angle = body ? body->GetAngle() : (rotation * (b2_pi / 180.0f));
    return b2Vec2(glm::cos(angle), glm::sin(angle));
}

void Rigidbody::OnDestroy() {
    if (body && physics_world) {
        physics_world->DestroyBody(body);
        body = nullptr;
    }
}
