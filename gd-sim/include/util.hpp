#pragma once
#include <cmath>
#include <iostream>
#include <utility>
#include <string>

#include <vector>
#include <unordered_set>
#include <variant>
#include <memory>
#include <unordered_map>

class hash_tuple {
    template<class T>
    struct component {
        const T& value;
        component(const T& value) : value(value) {}
        uintmax_t operator,(uintmax_t n) const {
            n ^= std::hash<T>()(value);
            n ^= n << (sizeof(uintmax_t) * 4 - 1);
            return n ^ std::hash<uintmax_t>()(n);
        }
    };

public:
    template<class Tuple>
    size_t operator()(const Tuple& tuple) const {
        return std::hash<uintmax_t>()(
            std::apply([](const auto& ... xs) { return (component(xs), ..., 0); }, tuple));
    }
};

template <typename ...Args>
struct velocity_map : public std::unordered_map<std::tuple<Args...>, std::vector<double>, hash_tuple> {
    using std::unordered_map<std::tuple<Args...>, std::vector<double>, hash_tuple>::unordered_map;
    double get(Args... args, int sec) const {
        return *(this->at(std::tuple(args...)).begin() + sec);
    }
};

/**
 * Using normal sets is extremely bad for performance since we would need to make
 * a copy for every frame, due it saving all previous states. I wrote this to ensure
 * that unnecessary copies do not happen if nothing is changed between frames.
 */
template <typename T>
class cow_set {
    std::shared_ptr<std::unordered_set<T>> data;
    bool written;
public:
    cow_set() : data(std::make_shared<std::unordered_set<T>>()), written(false) {}
    cow_set(cow_set const& other) : data(other.data), written(false) {}
    cow_set(cow_set&& other) : data(std::move(other.data)), written(false) {}
    cow_set(std::unordered_set<T> const& other) : data(std::make_shared<std::unordered_set<T>>(other)), written(false) {}

    cow_set& operator=(cow_set const& other) {
        data = other.data;
        written = false;
        return *this;
    }
    cow_set& operator=(cow_set&& other) {
        data = std::move(other.data);
        written = false;
        return *this;
    }
    void insert(T const& value) {
        if (!written) {
            data = std::make_shared<std::unordered_set<T>>(*data);
            written = true;
        }
        data->insert(value);
    }
    void erase(T const& value) {
        if (!written) {
            data = std::make_shared<std::unordered_set<T>>(*data);
            written = true;
        }
        data->erase(value);
    }
    bool contains(T const& value) const {
        return data->contains(value);
    }
    bool empty() const {
        return data->empty();
    }
    size_t size() const {
        return data->size();
    }
    void clear() {
        if (!written) {
            data = std::make_shared<std::unordered_set<T>>();
            written = true;
        }
        data->clear();
    }
    std::unordered_set<T> const& get() const {
        return *data;
    }
};

/// Combine two values into one for the purposes of switches
constexpr int case_and(auto a, auto b) {
    return static_cast<int>(a) | (static_cast<int>(b) << 4);
}

inline constexpr float deg2rad(float deg) {
    return deg * 3.141592653 / 180;
}
inline constexpr float rad2deg(float rad) {
    return rad * 180 / 3.141592653;
}

inline float stod_def(std::string const& str, float def = 0) {
    char const* begin = str.c_str();
    char* end = nullptr;

    errno = 0;
    double out = std::strtod(begin, &end);
    if (begin == end || *end != '\0' || errno == ERANGE) {
        return def;
    }

    return out;
}

struct Vec2D {
    float x;
    float y;

    inline Vec2D() : x(0), y(0) {}
    inline Vec2D(float x, float y) : x(x), y(y) {}

    inline bool operator==(Vec2D const& other) const {
        return x == other.x && y == other.y;
    }

    inline Vec2D operator-(Vec2D const& other) const {
        return {x - other.x, y - other.y};
    }
    inline Vec2D operator+(Vec2D const& other) const {
        return {x + other.x, y + other.y};
    }
    inline Vec2D operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }
    inline Vec2D operator/(float scalar) const {
        return {x / scalar, y / scalar};
    }
    inline Vec2D& operator+=(Vec2D const& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    inline Vec2D& operator-=(Vec2D const& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    inline Vec2D& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    inline Vec2D& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    Vec2D rotate(float angle, Vec2D const& pivot = {0, 0}) const;
};

struct Entity {
    Vec2D pos;
    Vec2D size;
    float rotation;
    bool intersects(Entity const& other) const;

    inline float getLeft() const { return pos.x - size.x / 2; }
    inline float getRight() const { return pos.x + size.x / 2; }
    inline float getTop() const { return pos.y + size.y / 2; }
    inline float getBottom() const { return pos.y - size.y / 2; }
};

