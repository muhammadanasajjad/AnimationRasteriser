#include <Animation.h>
#include <World.h>

Timeline& Timeline::move(Object& obj, const glm::vec3& to, float duration, EasingFunc easing) {
    addInterpolation(obj, TimelineEntry::Position, to, duration, easing);
    return *this;
}

Timeline& Timeline::rotate(Object& obj, const glm::vec3& to, float duration, EasingFunc easing) {
    addInterpolation(obj, TimelineEntry::Rotation, to, duration, easing);
    return *this;
}

Timeline& Timeline::scale(Object& obj, const glm::vec3& to, float duration, EasingFunc easing) {
    addInterpolation(obj, TimelineEntry::Scale, to, duration, easing);
    return *this;
}

Timeline& Timeline::wait(float duration) {
    cursor += duration;
    return *this;
}

Timeline& Timeline::hide(Object& obj) {
    visibilityEvents.push_back({ cursor, &obj, false });
    return *this;
}

Timeline& Timeline::show(Object& obj) {
    visibilityEvents.push_back({ cursor, &obj, true });
    return *this;
}

Timeline& Timeline::fadeIn(Object& obj, float duration, EasingFunc easing) {
    fadeEvents.push_back({ cursor, &obj, obj.opacity, 1.0f, duration, easing, false });
    return *this;
}

Timeline& Timeline::fadeOut(Object& obj, float duration, EasingFunc easing) {
    fadeEvents.push_back({ cursor, &obj, obj.opacity, 0.0f, duration, easing, false });
    return *this;
}

void Timeline::parallel(std::function<void(Timeline&)> fn) {
    Timeline sub;
    fn(sub);

    if (sub.entries.empty() && sub.visibilityEvents.empty()) return;

    float maxEnd = 0.0f;
    for (auto& entry : sub.entries) {
        entry.startTime = cursor;
        entries.push_back(entry);
        float end = entry.startTime + entry.duration;
        if (end > maxEnd) maxEnd = end;
    }
    for (auto& evt : sub.visibilityEvents) {
        evt.time = cursor;
        visibilityEvents.push_back(evt);
    }
    for (auto& fade : sub.fadeEvents) {
        fade.startTime = cursor;
        fadeEvents.push_back(fade);
    }
    cursor = maxEnd > cursor ? maxEnd : cursor;
}

void Timeline::update(float dt) {
    currentTime += dt;

    for (auto& e : entries) {
        if (currentTime < e.startTime) continue;
        if (!e.started) {
            switch (e.property) {
                case TimelineEntry::Position: e.from = e.target->transform.position; break;
                case TimelineEntry::Rotation: e.from = e.target->transform.rotation; break;
                case TimelineEntry::Scale:    e.from = e.target->transform.scale; break;
            }
            e.started = true;
        }
        float t = (currentTime - e.startTime) / e.duration;
        t = glm::clamp(t, 0.0f, 1.0f);
        float eased = e.easing(t);
        glm::vec3 value = glm::mix(e.from, e.to, eased);

        switch (e.property) {
            case TimelineEntry::Position: e.target->transform.position = value; break;
            case TimelineEntry::Rotation: e.target->transform.rotation = value; break;
            case TimelineEntry::Scale:    e.target->transform.scale = value; break;
        }
    }

    for (auto& evt : visibilityEvents) {
        if (currentTime >= evt.time) {
            evt.target->visible = evt.visible;
        }
    }

    for (auto& fade : fadeEvents) {
        if (currentTime < fade.startTime) continue;
        if (!fade.started) {
            fade.from = fade.target->opacity;
            fade.target->visible = true;
            fade.started = true;
        }
        float t = glm::clamp((currentTime - fade.startTime) / fade.duration, 0.0f, 1.0f);
        float eased = fade.easing(t);
        fade.target->opacity = glm::mix(fade.from, fade.to, eased);
        if (fade.to <= 0.0f && t >= 1.0f) {
            fade.target->visible = false;
        }
    }
}

void Timeline::reset() {
    currentTime = 0.0f;
    cursor = 0.0f;
    entries.clear();
    visibilityEvents.clear();
    fadeEvents.clear();
}

float Timeline::getTime() const { return currentTime; }

float Timeline::getDuration() const {
    float maxEnd = cursor;
    for (const auto& e : entries) {
        float end = e.startTime + e.duration;
        if (end > maxEnd) maxEnd = end;
    }
    for (const auto& f : fadeEvents) {
        float end = f.startTime + f.duration;
        if (end > maxEnd) maxEnd = end;
    }
    return maxEnd;
}

void Timeline::addInterpolation(Object& obj, TimelineEntry::Property prop, const glm::vec3& to, float duration, EasingFunc easing) {
    glm::vec3 from;
    switch (prop) {
        case TimelineEntry::Position: from = obj.transform.position; break;
        case TimelineEntry::Rotation: from = obj.transform.rotation; break;
        case TimelineEntry::Scale:    from = obj.transform.scale; break;
    }
    entries.push_back({ cursor, &obj, from, to, duration, easing, false, prop });
    cursor += duration;
}
