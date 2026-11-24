// ============================================================
//  Scene: الغرفة (جدران + أرضية)، الطاولة، 3 قطع ديكور، الملصق الممزق
// ============================================================
#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.h"
#include "Shader.h"
#include "Textures.h"
#include "Config.h"

// عنصر قابل للرسم
struct Renderable {
    const Mesh* mesh = nullptr;
    glm::mat4 model{1.f};
    GLuint texture = 0;
    glm::vec2 uvScale{1.f, 1.f};
    glm::vec3 tint{1.f};
    float emissive = 0.f;
    float spec = 0.08f;
    float shine = 32.f;
    bool castShadow = true;
};

struct PointLight { glm::vec3 pos, color; };

inline void drawRenderable(const Shader& sh, const Renderable& r, bool depthOnly) {
    sh.setMat4("uModel", r.model);
    if (!depthOnly) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, r.texture);
        sh.setVec2("uUVScale", r.uvScale);
        sh.setVec3("uTint", r.tint);
        sh.setFloat("uEmissive", r.emissive);
        sh.setFloat("uSpec", r.spec);
        sh.setFloat("uShine", r.shine);
    }
    r.mesh->draw();
}

class Scene {
public:
    std::vector<Renderable> items;
    std::vector<PointLight> lights;
    GLuint whiteTex = 0;

    // مجسمات مشتركة
    Mesh unitBox, unitPlane, unitSphere;

    void build() {
        using namespace cfg;
        whiteTex = tex::white();
        unitBox = prim::box(1, 1, 1);
        unitPlane = prim::plane(1, 1);
        unitSphere = prim::sphere(1.f, 20, 14);

        buildRoom();
        buildTable();
        buildDecor();
        buildPoster();
    }

    glm::vec3 sunDir() const { return glm::normalize(glm::vec3(5, 14, 9)); }

private:
    std::vector<std::unique_ptr<Mesh>> store;

    Mesh* keep(Mesh m) { store.push_back(std::make_unique<Mesh>(m)); return store.back().get(); }

    // خشب داكن (عروق أفقية / رأسية) + لون الذهب المشترك للزخارف
    GLuint woodH = 0, woodV = 0;
    static glm::vec3 goldCol() { return glm::vec3(0.80f, 0.60f, 0.24f); }
    void initWood() {
        if (woodH) return;
        woodH = tex::upload(tex::woodGrain(), true);
        woodV = tex::upload(tex::transposed(tex::woodGrain()), true);
    }

    // مساعد: إضافة صندوق بحجم وموضع ولون
    Renderable& addBox(glm::vec3 size, glm::vec3 pos, glm::vec3 color, GLuint t = 0, float spec = 0.08f, float angleY = 0.f) {
        Renderable r;
        r.mesh = &unitBox;
        r.model = glm::translate(glm::mat4(1.f), pos);
        if (angleY != 0.f) r.model = glm::rotate(r.model, angleY, glm::vec3(0, 1, 0));
        r.model = glm::scale(r.model, size);
        r.texture = t ? t : whiteTex; r.tint = color; r.spec = spec;
        items.push_back(r); return items.back();
    }

    // ---------------- الغرفة ----------------
    void buildRoom() {
        using namespace cfg;
        float W = ROOM_X_MAX - ROOM_X_MIN, D = ROOM_Z_MAX - ROOM_Z_MIN;
        float cx = (ROOM_X_MIN + ROOM_X_MAX) / 2, cz = (ROOM_Z_MIN + ROOM_Z_MAX) / 2;
        float cy = FLOOR_Y + ROOM_HEIGHT / 2;

        initWood();
        GLuint wallTex = tex::upload(tex::wallpaper(), true);
        GLuint floorTex = tex::upload(tex::woodFloor(), true);

        // الأرضية (خشب جوز داكن لامع)
        Renderable f; f.mesh = &unitPlane; f.texture = floorTex; f.castShadow = false; f.spec = 0.34f; f.shine = 56.f;
        f.model = glm::translate(glm::mat4(1.f), glm::vec3(cx, FLOOR_Y, cz));
        f.model = glm::rotate(f.model, -glm::half_pi<float>(), glm::vec3(1, 0, 0));
        f.model = glm::scale(f.model, glm::vec3(W, D, 1));
        f.uvScale = { W / 8.f, D / 8.f };
        items.push_back(f);

        // الجدران (كل جدار مستوي ينظر إلى داخل الغرفة)
        auto wall = [&](float w, glm::vec3 pos, float rotY) {
            Renderable r; r.mesh = &unitPlane; r.texture = wallTex; r.castShadow = false; r.spec = 0.02f;
            r.model = glm::translate(glm::mat4(1.f), pos);
            r.model = glm::rotate(r.model, rotY, glm::vec3(0, 1, 0));
            r.model = glm::scale(r.model, glm::vec3(w, ROOM_HEIGHT, 1));
            r.uvScale = { w / 5.f, ROOM_HEIGHT / 5.f };
            items.push_back(r);
        };
        wall(W, {cx, cy, ROOM_Z_MIN}, 0.f);                          // الخلفي
        wall(W, {cx, cy, ROOM_Z_MAX}, glm::pi<float>());             // الأمامي (خلف اللاعب)
        wall(D, {ROOM_X_MIN, cy, cz}, glm::half_pi<float>());        // الأيسر
        wall(D, {ROOM_X_MAX, cy, cz}, -glm::half_pi<float>());       // الأيمن

        // سجادة دائرية رمادية داكنة بحلقة خمرية وإطار ذهبي تحت الطاولة
        {
            Renderable rg; rg.mesh = &unitPlane; rg.texture = tex::upload(tex::rug(), false);
            rg.castShadow = false; rg.spec = 0.05f;
            rg.model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, FLOOR_Y + 0.03f, 0.f));
            rg.model = glm::rotate(rg.model, -glm::half_pi<float>(), glm::vec3(1, 0, 0));
            rg.model = glm::scale(rg.model, glm::vec3(RUG_R * 2.f, RUG_R * 2.f, 1.f));
            items.push_back(rg);
        }

        // قوائم سفلية: خشب ماهوجني داكن + شريط ذهبي
        const glm::vec3 sk(0.85f, 0.82f, 0.80f), gd = goldCol();
        const float fy = FLOOR_Y;
        auto wood = [&](glm::vec3 size, glm::vec3 pos, GLuint t, glm::vec3 tint = glm::vec3(1.f)) {
            Renderable& r = addBox(size, pos, tint, t, 0.28f); r.castShadow = false; return &r;
        };
        auto trim = [&](glm::vec3 size, glm::vec3 pos) {
            Renderable& r = addBox(size, pos, gd, 0, 0.75f); r.castShadow = false; r.shine = 72.f;
        };
        wood({W, 0.5f, 0.15f}, {cx, fy + 0.25f, ROOM_Z_MIN + 0.075f}, woodH, sk)->uvScale = { W / 4.f, 0.5f };
        wood({W, 0.5f, 0.15f}, {cx, fy + 0.25f, ROOM_Z_MAX - 0.075f}, woodH, sk)->uvScale = { W / 4.f, 0.5f };
        wood({0.15f, 0.5f, D}, {ROOM_X_MIN + 0.075f, fy + 0.25f, cz}, woodH, sk)->uvScale = { D / 4.f, 0.5f };
        wood({0.15f, 0.5f, D}, {ROOM_X_MAX - 0.075f, fy + 0.25f, cz}, woodH, sk)->uvScale = { D / 4.f, 0.5f };
        trim({W, 0.04f, 0.17f}, {cx, fy + 0.52f, ROOM_Z_MIN + 0.085f});
        trim({W, 0.04f, 0.17f}, {cx, fy + 0.52f, ROOM_Z_MAX - 0.085f});
        trim({0.17f, 0.04f, D}, {ROOM_X_MIN + 0.085f, fy + 0.52f, cz});
        trim({0.17f, 0.04f, D}, {ROOM_X_MAX - 0.085f, fy + 0.52f, cz});

        // ألواح خشبية سفلية (Wainscot) على الجدران الأربعة + حافة ذهبية
        // alongX: الجدار يمتد على X (ثابت z) ، n = اتجاه الداخل (+1/-1)
        auto wainscot = [&](bool alongX, float fixed, float n, float len, float mid) {
            const float t0 = 0.09f, wy = fy + 0.5f, wh = 4.0f, cyy = wy + wh / 2;
            auto P = [&](float along, float depth) {                      // موضع بحسب المحورين
                return alongX ? glm::vec3(along, 0.f, fixed + n * depth) : glm::vec3(fixed + n * depth, 0.f, along);
            };
            auto S = [&](float length, float h, float thick) {
                return alongX ? glm::vec3(length, h, thick) : glm::vec3(thick, h, length);
            };
            glm::vec3 p = P(mid, t0 / 2); p.y = cyy;
            wood(S(len, wh, t0), p, woodV, glm::vec3(0.72f, 0.70f, 0.68f))->uvScale = { len / 4.f, 1.f };
            int cnt = std::max(1, (int)(len / 3.6f));
            float step = len / cnt, pw = step - 0.55f;
            for (int i = 0; i < cnt; i++) {
                float a = mid - len / 2 + (i + 0.5f) * step;
                glm::vec3 pp = P(a, t0 + 0.0175f); pp.y = cyy;
                wood(S(pw, 2.9f, 0.035f), pp, woodV, glm::vec3(0.95f, 0.92f, 0.90f))->uvScale = { 0.6f, 0.8f };
                // إطار ذهبي رفيع حول اللوح
                const float g = 0.035f, e = 0.02f, dd = t0 + 0.04f;
                glm::vec3 pt = P(a, dd); pt.y = cyy + 1.45f; trim(S(pw, g, e), pt);
                glm::vec3 pb = P(a, dd); pb.y = cyy - 1.45f; trim(S(pw, g, e), pb);
                glm::vec3 pl = P(a - pw / 2, dd); pl.y = cyy; trim(S(g, 2.9f, e), pl);
                glm::vec3 pr = P(a + pw / 2, dd); pr.y = cyy; trim(S(g, 2.9f, e), pr);
            }
            glm::vec3 rail = P(mid, 0.08f); rail.y = wy + wh + 0.11f;       // رف الكرسي
            wood(S(len, 0.22f, 0.16f), rail, woodH, glm::vec3(0.92f, 0.89f, 0.86f))->uvScale = { len / 4.f, 0.5f };
            glm::vec3 rg = P(mid, 0.10f); rg.y = wy + wh + 0.24f; trim(S(len, 0.04f, 0.12f), rg);
        };
        wainscot(true,  ROOM_Z_MIN, +1.f, W, cx);
        wainscot(true,  ROOM_Z_MAX, -1.f, W, cx);
        wainscot(false, ROOM_X_MIN, +1.f, D - 0.2f, cz);
        wainscot(false, ROOM_X_MAX, -1.f, D - 0.2f, cz);
    }

    // ---------------- الطاولة ----------------
    void buildTable() {
        using namespace cfg;
        initWood();
        const glm::vec3 wood(1.0f, 0.97f, 0.94f), gd = goldCol();
        const float R = TABLE_R, legH = TABLE_H - TABLE_T;
        // أسطوانة: نصف قطر، ارتفاع، مركز y، لون، خامة، لمعان، تكرار الخامة، ظل
        auto cyl = [&](float rad, float h, float y, glm::vec3 tint, GLuint t, float spec, glm::vec2 uv, bool shadow = true) {
            Renderable x; x.mesh = keep(prim::cylinder(rad, rad, h, 96, true));
            x.texture = t ? t : whiteTex; x.tint = tint; x.spec = spec; x.shine = spec > 0.5f ? 72.f : 32.f;
            x.uvScale = uv; x.castShadow = shadow;
            x.model = glm::translate(glm::mat4(1.f), glm::vec3(0.f, y, 0.f));
            items.push_back(x);
        };
        cyl(R, TABLE_T, -TABLE_T / 2, wood, woodH, 0.3f, { 16.f, 0.5f });               // حافة الطاولة الخشبية
        cyl(R + 0.05f, 0.14f, -0.07f, gd, 0, 0.8f, { 1.f, 1.f }, false);                // طوق ذهبي عند حافة السطح
        cyl(R + 0.03f, 0.05f, -0.33f, gd, 0, 0.75f, { 1.f, 1.f }, false);               // شريط ذهبي سفلي
        // قماش الطاولة الدائري + الأقسام الثلاثة (طبقة رقيقة فوق السطح)
        Renderable top; top.mesh = &unitPlane; top.texture = tex::upload(tex::tableTop(), false);
        top.castShadow = false; top.spec = 0.03f;
        top.model = glm::translate(glm::mat4(1.f), glm::vec3(0, 0.002f, 0));
        top.model = glm::rotate(top.model, -glm::half_pi<float>(), glm::vec3(1, 0, 0));
        top.model = glm::scale(top.model, glm::vec3(R * 2.f, R * 2.f, 1));
        items.push_back(top);
        // قاعدة مركزية (عمود) بدل الأرجل الأربع: عمود + طوق ذهبي + قاعدة مدرجة
        cyl(0.85f, legH, -TABLE_T - legH / 2, wood, woodV, 0.3f, { 6.f, 1.f });
        cyl(0.97f, 0.14f, -TABLE_T - 0.30f, gd, 0, 0.75f, { 1.f, 1.f });
        cyl(2.3f, 0.22f, -TABLE_H + 0.11f, wood, woodH, 0.3f, { 8.f, 0.3f });
        cyl(1.5f, 0.40f, -TABLE_H + 0.42f, wood, woodH, 0.3f, { 6.f, 0.4f });
        cyl(2.34f, 0.05f, -TABLE_H + 0.23f, gd, 0, 0.75f, { 1.f, 1.f }, false);
        cyl(1.54f, 0.05f, -TABLE_H + 0.63f, gd, 0, 0.75f, { 1.f, 1.f }, false);
        lights.push_back({ {0, 8, 1}, {0.75f, 0.66f, 0.52f} });   // إضاءة فوق الطاولة
    }

    // ---------------- الديكور: مكتبة + نبتة + مصباح أرضي ----------------
    void buildDecor() {
        using namespace cfg;
        const float fy = FLOOR_Y;

        // 1) مكتبة كتب على الجدار الخلفي (يسار)
        {
            glm::vec3 wood(0.95f, 0.92f, 0.90f), o(-9.f, fy, ROOM_Z_MIN + 0.65f + 0.2f);
            const float W = 5, H = 7, D = 1.3f, t = 0.18f;
            auto part = [&](glm::vec3 s, glm::vec3 p, bool vertical = false) {
                addBox(s, o + p, wood, vertical ? woodV : woodH, 0.3f).uvScale = vertical ? glm::vec2(0.5f, 1.5f) : glm::vec2(1.2f, 0.4f);
            };
            part({t, H, D}, {-W / 2, H / 2, 0}, true); part({t, H, D}, {W / 2, H / 2, 0}, true);
            part({W, t, D}, {0, t / 2, 0});       part({W, t, D}, {0, H - t / 2, 0});
            part({W, H, 0.08f}, {0, H / 2, -D / 2}, true);
            // إكليل ذهبي أعلى المكتبة
            { Renderable& g = addBox({W + 0.1f, 0.07f, D + 0.1f}, o + glm::vec3(0, H + 0.035f, 0), goldCol(), 0, 0.75f); g.shine = 72.f; }
            const int shelves = 4;
            // ألوان كتب فاخرة: خمري، أخضر زمردي، كحلي، جلد، ذهبي، بنفسجي غامق، كريمي، بني داكن
            glm::vec3 cols[8] = {{.42f,.08f,.12f},{.42f,.38f,.33f},{.22f,.22f,.24f},{.50f,.33f,.16f},{.72f,.55f,.18f},{.30f,.10f,.25f},{.78f,.70f,.55f},{.16f,.10f,.08f}};
            Rng rn(11);
            for (int s = 0; s < shelves; s++) {
                float y = 0.18f + s * ((H - 0.36f) / shelves);
                if (s > 0) part({W - t, t, D - 0.1f}, {0, y, 0});
                float x = -W / 2 + t / 2 + 0.15f;
                while (x < W / 2 - 0.5f) {
                    float bw = 0.15f + rn.next() * 0.18f, bh = 1.0f + rn.next() * 0.55f;
                    addBox({bw, bh, D * 0.7f}, o + glm::vec3(x + bw / 2, y + t / 2 + bh / 2, 0), cols[(int)(rn.next() * 8) % 8], 0, 0.15f);
                    x += bw + 0.02f;
                }
            }
        }

        // 2) نبتة في أصيص (ركن خلفي أيمن)
        {
            glm::vec3 o(11.5f, fy, ROOM_Z_MIN + 2.2f);
            Mesh* pot = keep(prim::cylinder(0.9f, 0.65f, 1.3f, 28, true));
            Mesh* soil = keep(prim::cylinder(0.82f, 0.82f, 0.05f, 24, true));
            Renderable r; r.mesh = pot; r.texture = whiteTex; r.tint = {0.17f, 0.17f, 0.18f}; r.spec = 0.4f; r.shine = 64.f;   // خزف رمادي فحمي لامع
            r.model = glm::translate(glm::mat4(1.f), o + glm::vec3(0, 0.65f, 0)); items.push_back(r);
            { // حافة ذهبية للأصيص
                Mesh* rim = keep(prim::cylinder(0.94f, 0.94f, 0.08f, 28, true));
                Renderable g; g.mesh = rim; g.texture = whiteTex; g.tint = goldCol(); g.spec = 0.75f; g.shine = 72.f;
                g.model = glm::translate(glm::mat4(1.f), o + glm::vec3(0, 1.28f, 0)); items.push_back(g);
            }
            r.spec = 0.08f; r.shine = 32.f;
            r.mesh = soil; r.tint = {0.23f, 0.16f, 0.1f};
            r.model = glm::translate(glm::mat4(1.f), o + glm::vec3(0, 1.28f, 0)); items.push_back(r);
            for (int i = 0; i < 9; i++) {
                float a = i / 9.f * 6.2831853f;
                Renderable leaf; leaf.mesh = &unitSphere; leaf.texture = whiteTex; leaf.tint = {0.18f, 0.55f, 0.24f};
                glm::mat4 m = glm::translate(glm::mat4(1.f), o + glm::vec3(std::cos(a) * 0.35f, 2.4f + (i % 3) * 0.25f, std::sin(a) * 0.35f));
                m = glm::rotate(m, std::sin(a) * 0.6f, glm::vec3(1, 0, 0));
                m = glm::rotate(m, -a, glm::vec3(0, 1, 0));
                m = glm::rotate(m, -std::cos(a) * 0.6f, glm::vec3(0, 0, 1));
                leaf.model = glm::scale(m, glm::vec3(0.17f, 0.75f, 0.06f) * 1.0f);
                items.push_back(leaf);
            }
        }

        // 3) مصباح أرضي + ضوء نقطي دافئ
        {
            glm::vec3 o(ROOM_X_MAX - 2.2f, fy, -3.5f);
            Mesh* base = keep(prim::cylinder(0.8f, 0.9f, 0.18f, 28, true));
            Mesh* pole = keep(prim::cylinder(0.07f, 0.07f, 6.2f, 12, false));
            Mesh* shade = keep(prim::cylinder(0.7f, 1.15f, 1.3f, 28, false));
            Renderable r; r.texture = whiteTex; r.spec = 0.7f; r.shine = 72.f; r.tint = goldCol();   // نحاس/ذهب
            r.mesh = base; r.model = glm::translate(glm::mat4(1.f), o + glm::vec3(0, 0.09f, 0)); items.push_back(r);
            r.mesh = pole; r.model = glm::translate(glm::mat4(1.f), o + glm::vec3(0, 3.2f, 0)); items.push_back(r);
            Renderable s; s.mesh = shade; s.texture = whiteTex; s.tint = {0.96f, 0.87f, 0.7f}; s.emissive = 0.55f; s.castShadow = false;
            s.model = glm::translate(glm::mat4(1.f), o + glm::vec3(0, 6.5f, 0)); items.push_back(s);
            Renderable b; b.mesh = &unitSphere; b.texture = whiteTex; b.tint = {1.f, 0.95f, 0.75f}; b.emissive = 1.f; b.castShadow = false;
            b.model = glm::scale(glm::translate(glm::mat4(1.f), o + glm::vec3(0, 6.4f, 0)), glm::vec3(0.28f)); items.push_back(b);
            lights.push_back({ o + glm::vec3(0, 6.3f, 0), {1.1f, 0.8f, 0.5f} });
        }
    }

    // ---------------- الملصق الممزق على الجدار الخلفي ----------------
    void buildPoster() {
        using namespace cfg;
        Renderable p; p.mesh = &unitPlane; p.texture = tex::upload(tex::poster(), false);
        p.castShadow = false; p.spec = 0.02f;
        glm::vec3 pos(1.5f, 4.6f, ROOM_Z_MIN + 0.03f);
        p.model = glm::scale(glm::translate(glm::mat4(1.f), pos), glm::vec3(POSTER_W, POSTER_H, 1));
        items.push_back(p);
        // شريطان لاصقان في الزاويتين العلويتين
        for (int sx = -1; sx <= 1; sx += 2) {
            Renderable t; t.mesh = &unitPlane; t.texture = whiteTex; t.tint = {0.91f, 0.89f, 0.78f}; t.castShadow = false;
            glm::mat4 m = glm::translate(glm::mat4(1.f), pos + glm::vec3(sx * (POSTER_W / 2 - 0.25f), POSTER_H / 2 - 0.05f, 0.012f));
            m = glm::rotate(m, sx * 0.6f, glm::vec3(0, 0, 1));
            t.model = glm::scale(m, glm::vec3(0.7f, 0.22f, 1));
            items.push_back(t);
        }
    }
};
