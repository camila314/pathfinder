#include <util.hpp>
#include <cmath>
#include <complex>

float slerp(float fromAngle, float toAngle, float t) {
    std::complex<float> fromVec = std::polar(1.0f, fromAngle * 0.5f);
    std::complex<float> toVec = std::polar(1.0f, toAngle * 0.5f);


    float dot = std::imag(fromVec) * std::imag(toVec) + std::real(fromVec) * std::real(toVec);
    if (dot < 0.0f) {
        dot *= -1;
        toVec *=  -1;
    }

    std::complex<float> weight = std::complex(1.0f - t, t);
    if (dot < 0.9999) {
        float between = std::acos(dot);

        weight *= between;
        weight = std::complex(std::sin(weight.real()), std::sin(weight.imag()));
        weight /= std::sin(between);
    }

    std::complex<float> interpVec = (weight.imag() * toVec) + (weight.real() * fromVec);
    return std::atan2(std::imag(interpVec), std::real(interpVec)) * 2;
}


Vec2D Vec2D::rotate(float angle, Vec2D const& pivot) const {
    if (angle == 0) return *this;

    Vec2D tmp = *this - pivot;

    float rad = deg2rad(angle);
    float s = std::sin(rad);
    float c = std::cos(rad);

    tmp = {tmp.x * c - tmp.y * s, tmp.x * s + tmp.y * c};
    tmp += pivot;

    return tmp;
}

// Very complicated (but very fast) way to check intersection
bool intersectOneWay(Entity const& a, Entity const& b) {
    float big = std::max(a.size.x, a.size.y) + std::max(b.size.x, b.size.y);
    if (std::abs(a.pos.x - b.pos.x) > big || std::abs(a.pos.y - b.pos.y) > big) {
        return false;
    }

    Entity tmp = b;

    tmp.rotation -= a.rotation;
    tmp.pos = tmp.pos.rotate(-a.rotation, a.pos);

    Vec2D corners[4] = {
        Vec2D(tmp.getLeft(), tmp.getBottom()).rotate(tmp.rotation, tmp.pos),
        Vec2D(tmp.getRight(), tmp.getBottom()).rotate(tmp.rotation, tmp.pos),
        Vec2D(tmp.getRight(), tmp.getTop()).rotate(tmp.rotation, tmp.pos),
        Vec2D(tmp.getLeft(), tmp.getTop()).rotate(tmp.rotation, tmp.pos)
    };

    float lastDiffX = 0;
    bool overlapX = false;

    float lastDiffY = 0;
    bool overlapY = false;

    for (auto vert : corners) {
        if (!overlapX) {
            float diffX = vert.x - a.pos.x;
            if ((vert.x >= a.getLeft() && vert.x <= a.getRight()) || (lastDiffX != 0 && std::signbit(lastDiffX) != std::signbit(diffX))) {
                overlapX = true;
            }
            lastDiffX = diffX;
        }
        if (!overlapY) {
            float diffY = vert.y - a.pos.y;
            if ((vert.y >= a.getBottom() && vert.y <= a.getTop()) || (lastDiffY != 0 && std::signbit(lastDiffY) != std::signbit(diffY))) {
                overlapY = true;
            }
            lastDiffY = diffY;
        }
    }

    return overlapX && overlapY;
}

bool Entity::intersects(Entity const& b) const {
    return intersectOneWay(*this, b) && intersectOneWay(b, *this);
}
