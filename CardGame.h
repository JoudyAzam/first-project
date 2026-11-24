// ============================================================
//  البطاقات: مخزون اللاعب (5 بطاقات رباعية الأضلاع) + منطق الزر G + الوقوف/التسطيح
// ============================================================
#pragma once
#include <array>
#include <cstdlib>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Anim.h"
#include "Config.h"
#include "Scene.h"
#include "CameraRig.h"

struct Card : Transformable {
    GLuint faceTex = 0;
    int zone = 0;       // 0 = المخزون ، 1 = الملعوب ، 2 = الخصم
    int slot = 0;
};

class CardGame {
public:
    ViewMode mode = ViewMode::Top;
    std::array<Card, cfg::DECK_SIZE> cards;
    std::vector<Card*> deck, played, opponent;      // opponent فارغ حسب المتطلبات

    void init(Scene& scene) {
        backTex = tex::upload(tex::cardBack(), false);
        box = prim::box(cfg::CARD_W, cfg::CARD_H, cfg::CARD_T);
        plane = prim::plane(cfg::CARD_W, cfg::CARD_H);
        (void)scene;
        for (int i = 0; i < cfg::DECK_SIZE; i++) {
            Card& c = cards[i];
            c.faceTex = tex::upload(tex::cardFace(i), false);
            c.zone = 0; c.slot = i;
            Pose p = poseFor(0, i, mode);
            c.pos = p.pos; c.rot = p.rot;
            deck.push_back(&c);
        }
    }

    // ---- موضع ودوران البطاقة في خانة معينة حسب حالة الكاميرا ----
    struct Pose { glm::vec3 pos; glm::quat rot; };
    Pose poseFor(int zone, int slot, ViewMode m) const {
        const cfg::Zone& z = zone == 0 ? cfg::ZONE_DECK : (zone == 1 ? cfg::ZONE_PLAYED : cfg::ZONE_OPPONENT);
        float x = z.cx + (slot - (cfg::DECK_SIZE - 1) / 2.f) * cfg::CARD_SPACING;
        bool standing = (m == ViewMode::Player);
        float y = (standing ? cfg::CARD_H / 2 : cfg::CARD_T / 2) + 0.004f;
        glm::quat q = standing ? glm::quat(1, 0, 0, 0)                                   // واقفة: الوجه نحو اللاعب
                               : glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0)); // مسطحة: الوجه للأعلى
        return { glm::vec3(x, y, z.cz), q };
    }

    // ---- تبديل الحالة: البطاقات تقف / تُسطّح بحركة (Animation) ----
    void setMode(ViewMode m) {
        if (m == mode) return;
        mode = m;
        int i = 0;
        for (auto* list : { &deck, &played, &opponent })
            for (Card* c : *list) {
                Pose p = poseFor(c->zone, c->slot, m);
                tweens.add(Tween(c, p.pos, p.rot, cfg::ANIM_FLIP, i++ * cfg::ANIM_STAGGER, 0.5f));
            }
    }

    // ---- زر G: بطاقة عشوائية من المخزون إلى منطقة اللعب ----
    // بعد 5 بطاقات يصبح المخزون فارغاً ولا يحدث شيء
    bool deal() {
        if (deck.empty()) return false;
        std::uniform_int_distribution<size_t> dist(0, deck.size() - 1);
        size_t idx = dist(rng);
        Card* c = deck[idx];
        deck.erase(deck.begin() + (long)idx);
        c->zone = 1; c->slot = (int)played.size();
        played.push_back(c);
        Pose p = poseFor(1, c->slot, mode);
        tweens.add(Tween(c, p.pos, p.rot, cfg::ANIM_DEAL, 0.f, 2.2f));
        return true;
    }

    void update(float dt) { tweens.update(dt); }

    // ---- تحويل البطاقات إلى عناصر رسم (صندوق للجوانب + وجه أمامي + ظهر) ----
    void appendRenderables(std::vector<Renderable>& out, GLuint whiteTex) {
        for (const Card& c : cards) {
            glm::mat4 base = glm::translate(glm::mat4(1.f), c.pos) * glm::mat4_cast(c.rot);
            Renderable side; side.mesh = &box; side.model = base; side.texture = whiteTex;
            side.tint = {0.95f, 0.9f, 0.78f}; side.spec = 0.1f;
            out.push_back(side);
            Renderable front; front.mesh = &plane; front.texture = c.faceTex; front.castShadow = false; front.spec = 0.25f; front.shine = 64.f;
            front.model = base * glm::translate(glm::mat4(1.f), glm::vec3(0, 0, cfg::CARD_T / 2 + 0.0006f));
            out.push_back(front);
            Renderable back; back.mesh = &plane; back.texture = backTex; back.castShadow = false;
            back.model = base * glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -cfg::CARD_T / 2 - 0.0006f)) *
                         glm::rotate(glm::mat4(1.f), glm::pi<float>(), glm::vec3(0, 1, 0));
            out.push_back(back);
        }
    }

private:
    TweenManager tweens;
    Mesh box, plane;
    GLuint backTex = 0;
    std::mt19937 rng{ std::random_device{}() };
};
