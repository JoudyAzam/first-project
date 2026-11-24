// ============================================================
//  الحركة (Animation) بالتحويلات: انتقال (translate) + دوران (quaternion slerp)
//  بدل القفز المباشر نستوفي بين الوضع الحالي والهدف مع Easing
// ============================================================
#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

inline float easeInOutCubic(float t) {
    return t < 0.5f ? 4.f * t * t * t : 1.f - std::pow(-2.f * t + 2.f, 3.f) / 2.f;
}

// أي كائن له موضع ودوران
struct Transformable {
    glm::vec3 pos{0.f};
    glm::quat rot{1.f, 0.f, 0.f, 0.f};
};

struct Tween {
    Transformable* obj;
    glm::vec3 fromP, toP;
    glm::quat fromQ, toQ;
    float duration, delay, lift, t = 0.f;
    bool done = false;

    Tween(Transformable* o, glm::vec3 p, glm::quat q, float dur, float del, float liftHeight)
        : obj(o), fromP(o->pos), toP(p), fromQ(o->rot), toQ(q), duration(dur), delay(del), lift(liftHeight) {}

    void update(float dt) {
        if (done) return;
        if (delay > 0.f) { delay -= dt; return; }
        t = std::min(1.f, t + dt / duration);
        float e = easeInOutCubic(t);
        obj->pos = glm::mix(fromP, toP, e);
        obj->pos.y += std::sin(3.14159265f * e) * lift;     // قوس ارتفاع أثناء الحركة
        obj->rot = glm::slerp(fromQ, toQ, e);
        if (t >= 1.f) { obj->pos = toP; obj->rot = toQ; done = true; }
    }
};

class TweenManager {
public:
    void add(const Tween& tw) {
        for (auto& t : list) if (t.obj == tw.obj) t.done = true;   // حركة جديدة تلغي القديمة لنفس الجسم
        list.push_back(tw);
    }
    void update(float dt) {
        for (auto& t : list) t.update(dt);
        std::vector<Tween> keep;
        for (auto& t : list) if (!t.done) keep.push_back(t);
        list.swap(keep);
    }
private:
    std::vector<Tween> list;
};
