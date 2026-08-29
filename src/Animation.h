#pragma once

#include <functional>
#include <vector>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

class Object;
class Camera;

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

struct PathOrient {
    glm::vec3 forward = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
    float bank = 0.0f;
};

inline glm::vec3 rotationFromOrientation(const PathOrient& orient) {
    glm::vec3 f = glm::length(orient.forward) > 1e-6f ? glm::normalize(orient.forward)
                                                      : glm::vec3(1.0f, 0.0f, 0.0f);
    float yaw = glm::degrees(std::atan2(f.y, f.x));
    float pitch = glm::degrees(std::asin(glm::clamp(f.z, -1.0f, 1.0f)));
    return glm::vec3(pitch, yaw, orient.bank);
}

struct PathTrack {
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> samples;
    std::vector<float> cumulative;
    std::vector<glm::vec3> tangents;
    std::vector<float> banks;
    float totalLength = 0.0f;

    void build() {
        samples.clear();
        cumulative.clear();
        tangents.clear();
        banks.clear();
        totalLength = 0.0f;
        if (points.size() < 3) return;

        size_t segments = (points.size() - 1) / 2;
        const int STEPS = 32;
        glm::vec3 previous(0.0f);
        for (size_t s = 0; s < segments; s++) {
            const glm::vec3& a = points[s * 2];
            const glm::vec3& c = points[s * 2 + 1];
            const glm::vec3& b = points[s * 2 + 2];
            for (int k = (s == 0 ? 0 : 1); k <= STEPS; k++) {
                float t = static_cast<float>(k) / STEPS;
                float mt = 1.0f - t;
                glm::vec3 p = mt * mt * a + 2.0f * mt * t * c + t * t * b;
                if (!samples.empty()) {
                    totalLength += glm::length(p - previous);
                }
                samples.push_back(p);
                cumulative.push_back(totalLength);
                previous = p;
            }
        }

        size_t n = samples.size();
        tangents.resize(n);
        banks.resize(n, 0.0f);
        if (n == 0) return;
        if (n == 1) {
            tangents[0] = glm::vec3(1.0f, 0.0f, 0.0f);
            return;
        }
        for (size_t i = 0; i < n; i++) {
            size_t ip = i + 1 < n ? i + 1 : n - 1;
            size_t im = i > 0 ? i - 1 : 0;
            glm::vec3 diff = samples[ip] - samples[im];
            tangents[i] = glm::length(diff) > 1e-6f ? glm::normalize(diff) : glm::vec3(1.0f, 0.0f, 0.0f);
        }
        glm::vec3 up(0.0f, 0.0f, 1.0f);
        for (size_t i = 1; i + 1 < n; i++) {
            size_t im = i - 1, ip = i + 1;
            float ds = cumulative[ip] - cumulative[im];
            glm::vec3 tPrev = tangents[im] != tangents[ip] ? tangents[im] : tangents[i];
            float angle = std::atan2(glm::length(glm::cross(tPrev, tangents[ip])),
                                     glm::dot(tPrev, tangents[ip]));
            float curv = ds > 1e-6f ? angle / ds : 0.0f;
            float sign = glm::dot(glm::cross(tPrev, tangents[ip]), up);
            if (sign < 0.0f) curv = -curv;
            banks[i] = glm::clamp(std::atan(curv * 2.4f), -0.7f, 0.7f);
        }
    }

    glm::vec3 sample(float u) const {
        if (samples.empty()) return glm::vec3(0.0f);
        if (totalLength <= 0.0f || samples.size() < 2) return samples.front();
        u = glm::clamp(u, 0.0f, 1.0f);
        float target = u * totalLength;

        size_t lo = 0;
        size_t hi = cumulative.size() - 1;
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (cumulative[mid] <= target) lo = mid;
            else hi = mid;
        }

        float span = cumulative[hi] - cumulative[lo];
        float f = span > 0.0f ? (target - cumulative[lo]) / span : 0.0f;
        return glm::mix(samples[lo], samples[hi], f);
    }

    glm::vec3 sampleTangent(float u) const {
        if (tangents.empty()) return glm::vec3(1.0f, 0.0f, 0.0f);
        if (totalLength <= 0.0f || cumulative.size() < 2 || tangents.size() != cumulative.size())
            return tangents.front();
        u = glm::clamp(u, 0.0f, 1.0f);
        float target = u * totalLength;

        size_t lo = 0;
        size_t hi = cumulative.size() - 1;
        while (lo + 1 < hi) {
            size_t mid = (lo + hi) / 2;
            if (cumulative[mid] <= target) lo = mid;
            else hi = mid;
        }

        float span = cumulative[hi] - cumulative[lo];
        float f = span > 0.0f ? (target - cumulative[lo]) / span : 0.0f;
        return glm::normalize(glm::mix(tangents[lo], tangents[hi], f));
    }

    PathOrient sampleOrientation(float u) const {
        PathOrient orient;
        if (tangents.empty() || banks.empty()) {
            orient.forward = glm::vec3(1.0f, 0.0f, 0.0f);
            orient.up = glm::vec3(0.0f, 0.0f, 1.0f);
            orient.bank = 0.0f;
            return orient;
        }
        orient.forward = sampleTangent(u);
        if (totalLength <= 0.0f || cumulative.size() < 2 || banks.size() != cumulative.size()) {
            orient.bank = banks.front();
        } else {
            u = glm::clamp(u, 0.0f, 1.0f);
            float target = u * totalLength;
            size_t lo = 0;
            size_t hi = cumulative.size() - 1;
            while (lo + 1 < hi) {
                size_t mid = (lo + hi) / 2;
                if (cumulative[mid] <= target) lo = mid;
                else hi = mid;
            }
            float span = cumulative[hi] - cumulative[lo];
            float f = span > 0.0f ? (target - cumulative[lo]) / span : 0.0f;
            orient.bank = glm::mix(banks[lo], banks[hi], f);
        }
        orient.up = glm::vec3(0.0f, 0.0f, 1.0f);
        return orient;
    }
};

struct TimelineEntry {
    float startTime;
    Object* target;
    glm::vec3 from;
    glm::vec3 to;
    float duration;
    EasingFunc easing;
    bool started = false;

    enum Property { Position, Rotation, Scale } property;
    PathTrack path;
    bool orientAlongPath = false;
};

struct CameraAnim {
    float startTime;
    Camera* camera;
    float duration;
    EasingFunc easing;
    bool started = false;

    enum Kind { Move, Look } kind;
    glm::vec3 from;
    glm::vec3 to;
    PathTrack path;
    bool orientAlongPath = false;
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

struct DrawEvent {
    float startTime;
    Object* target;
    float duration;
    EasingFunc easing;
    bool started = false;
};

class Timeline {
public:
    Timeline& move(Object& obj, const glm::vec3& to, float duration, EasingFunc easing = Easing::linear);
    Timeline& move(Object& obj, const std::vector<glm::vec3>& path, float duration, EasingFunc easing = Easing::linear);
    Timeline& move(Object& obj, const std::vector<glm::vec3>& path, float duration, EasingFunc easing, bool oriented);
    Timeline& moveCamera(Camera& camera, const glm::vec3& to, float duration, EasingFunc easing = Easing::linear);
    Timeline& moveCamera(Camera& camera, const std::vector<glm::vec3>& path, float duration, EasingFunc easing = Easing::linear);
    Timeline& moveCamera(Camera& camera, const std::vector<glm::vec3>& path, float duration, EasingFunc easing, bool oriented);
    Timeline& lookCamera(Camera& camera, const glm::vec3& at, float duration, EasingFunc easing = Easing::linear);
    Timeline& rotate(Object& obj, const glm::vec3& to, float duration, EasingFunc easing = Easing::linear);
    Timeline& scale(Object& obj, const glm::vec3& to, float duration, EasingFunc easing = Easing::linear);
    Timeline& wait(float duration);
    Timeline& hide(Object& obj);
    Timeline& show(Object& obj);
    Timeline& fadeIn(Object& obj, float duration, EasingFunc easing = Easing::linear);
    Timeline& fadeOut(Object& obj, float duration, EasingFunc easing = Easing::linear);
    Timeline& draw(Object& obj, float duration, EasingFunc easing = Easing::linear);
    void parallel(std::function<void(Timeline&)> fn);

    void update(float dt);
    void reset();
    float getTime() const;
    float getCursor() const;
    float getDuration() const;

private:
    void addInterpolation(Object& obj, TimelineEntry::Property prop, const glm::vec3& to, float duration, EasingFunc easing);

    float currentTime = 0.0f;
    float cursor = 0.0f;
    std::vector<TimelineEntry> entries;
    std::vector<CameraAnim> cameraAnims;
    std::vector<VisibilityEvent> visibilityEvents;
    std::vector<FadeEvent> fadeEvents;
    std::vector<DrawEvent> drawEvents;
};
