// ============================================================
//  الكاميرا: حالتان ثابتتان مع انتقال متحرك بينهما
//   TOP    : من فوق الطاولة تماماً          (زر 1)
//   PLAYER : من منظور اللاعب (جهته من الطاولة) (زر 2)
// ============================================================
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Anim.h"
#include "Config.h"

enum class ViewMode { Top, Player };

class CameraRig {
public:
    float fovDeg = 60.f;
    ViewMode mode = ViewMode::Top;

    struct Pose { glm::vec3 pos, target, up; };

    explicit CameraRig(float aspect) { setAspect(aspect); cur = poses[0]; }

    void setAspect(float a) {
        aspect = a;
        float tanV = std::tan(glm::radians(fovDeg) / 2.f), tanH = tanV * aspect;
        // من فوق: ارتفاع يكفي لعرض الطاولة كاملة مع هامش
        float topH = std::max(11.5f / (2.f * tanV), 17.5f / (2.f * tanH));
        // منظور اللاعب: مسافة تكفي لعرض صف البطاقات، دون تجاوز جدار الغرفة
        float pz = std::min(5.f + 8.6f / tanH, cfg::ROOM_Z_MAX - 0.8f);
        poses[0] = { {0, topH, 0}, {0, 0, 0}, {0, 0, -1} };           // أعلى الشاشة = جهة الخصم
        poses[1] = { {0, 5.2f, pz}, {0, 0.3f, -1.f}, {0, 1, 0} };
        if (t >= 1.f) cur = poses[(int)mode];
    }

    void setMode(ViewMode m) {
        if (m == mode && t >= 1.f) return;
        from = cur; mode = m; t = 0.f;
    }

    void update(float dt) {
        if (t >= 1.f) return;
        t = std::min(1.f, t + dt / cfg::ANIM_CAMERA);
        float e = easeInOutCubic(t);
        const Pose& to = poses[(int)mode];
        cur.pos = glm::mix(from.pos, to.pos, e);
        cur.target = glm::mix(from.target, to.target, e);
        cur.up = glm::normalize(glm::mix(from.up, to.up, e));
        if (t >= 1.f) cur = to;
    }

    glm::mat4 view() const { return glm::lookAt(cur.pos, cur.target, cur.up); }
    glm::mat4 proj() const { return glm::perspective(glm::radians(fovDeg), aspect, 0.1f, 200.f); }
    glm::vec3 position() const { return cur.pos; }

private:
    float aspect = 16.f / 9.f;
    float t = 1.f;
    Pose poses[2], from{}, cur{};
};
