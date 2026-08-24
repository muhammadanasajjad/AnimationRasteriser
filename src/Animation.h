#pragma once

#include <functional>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

class Object;

namespace Easing {
    inline float linear(float t) { return t; }

    inline float easeInQuad(float t) { return t * t; }
    inline float easeOutQuad(float t) { return t * (2.0f - t); }
    inline float easeInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }

    inline float easeInCubic(float t) { return t * t * t; }
    inline float easeOutCubic(float t) { float u = 1.0f - t; return 1.0f - u * u * u; }
    inline float easeInOutCubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) / 2.0f;
    }

    inline float easeInSine(float t) { return 1.0f - glm::cos(t * 1.5707963267948966f); }
    inline float easeOutSine(float t) { return glm::sin(t * 1.5707963267948966f); }
    inline float easeInOutSine(float t) { return -0.5f * (glm::cos(3.1415926535897932f * t) - 1.0f); }

    inline float easeOutBack(float t) {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        return 1.0f + c3 * glm::pow(t - 1.0f, 3.0f) + c1 * glm::pow(t - 1.0f, 2.0f);
    }

    inline float easeOutBounce(float t) {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;
        if (t < 1.0f / d1) return n1 * t * t;
        else if (t < 2.0f / d1) return n1 * (t -= 1.5f / d1) * t + 0.75f;
        else if (t < 2.5f / d1) return n1 * (t -= 2.25f / d1) * t + 0.9375f;
        else return n1 * (t -= 2.625f / d1) * t + 0.984375f;
    }

    inline float easeInBounce(float t) { return 1.0f - easeOutBounce(1.0f - t); }
}

using EasingFunc = float(*)(float);

struct TimelineEntry {
    float startTime;
    Object* target;
    glm::vec3 from;
    glm::vec3 to;
    float duration;
    EasingFunc easing;
    bool started = false;

    enum Property { Position, Rotation, Scale } property;
};

struct VisibilityEvent {
    float time;
    Object* target;
    bool visible;
};

struct FadeEvent {
    float startTime;
    Object* target;
    float from;
    float to;
    float duration;
    EasingFunc easing;
    bool started = false;
};

class Timeline {
public:
    Timeline& move(Object& obj, const glm::vec3& to, float duration, EasingFunc easing = Easing::linear);
    Timeline& rotate(Object& obj, const glm::vec3& to, float duration, EasingFunc easing = Easing::linear);
    Timeline& scale(Object& obj, const glm::vec3& to, float duration, EasingFunc easing = Easing::linear);
    Timeline& wait(float duration);
    Timeline& hide(Object& obj);
    Timeline& show(Object& obj);
    Timeline& fadeIn(Object& obj, float duration, EasingFunc easing = Easing::linear);
    Timeline& fadeOut(Object& obj, float duration, EasingFunc easing = Easing::linear);
    void parallel(std::function<void(Timeline&)> fn);

    void update(float dt);
    void reset();
    float getTime() const;
    float getDuration() const;

private:
    void addInterpolation(Object& obj, TimelineEntry::Property prop, const glm::vec3& to, float duration, EasingFunc easing);

    float currentTime = 0.0f;
    float cursor = 0.0f;
    std::vector<TimelineEntry> entries;
    std::vector<VisibilityEvent> visibilityEvents;
    std::vector<FadeEvent> fadeEvents;
};
