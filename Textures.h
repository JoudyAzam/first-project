// ============================================================
//  توليد التكستشرات برمجياً (ورق الحائط، الخشب، الطاولة، البطاقات، الملصق الممزق)
//  وتحميل صور المستخدم (إن وُجدت) مع رسم بديل تلقائي (fallback)
// ============================================================
#pragma once
#include <glad/gl.h>
#include <cstring>
#include "Canvas.h"
#include "Config.h"

namespace tex {

// ---------- رفع Canvas إلى OpenGL ----------
// الـ Canvas: الصف 0 = أعلى الصورة. نقلبه رأسياً لأن OpenGL يعتبر v=0 الأسفل.
inline GLuint upload(const Canvas& c, bool repeat) {
    std::vector<uint8_t> flipped(c.px.size());
    for (int y = 0; y < c.h; y++)
        std::memcpy(&flipped[(size_t)(c.h - 1 - y) * c.w * 4], &c.px[(size_t)y * c.w * 4], (size_t)c.w * 4);
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, c.w, c.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    if (GLAD_GL_EXT_texture_filter_anisotropic)
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.f);
    return t;
}

inline GLuint white() { Canvas c(2, 2); c.fill({1, 1, 1, 1}); return upload(c, false); }

// ---------- أدوات رسم مساعدة للزخارف (تنعيم + دمج مرة واحدة لكل بكسل) ----------
inline void diamondFill(Canvas& c, float cx, float cy, float rx, float ry, const Color& col) {
    for (int y = (int)std::floor(cy - ry); y <= (int)std::ceil(cy + ry); y++)
        for (int x = (int)std::floor(cx - rx); x <= (int)std::ceil(cx + rx); x++)
            if (std::fabs(x + .5f - cx) / rx + std::fabs(y + .5f - cy) / ry <= 1.f) c.blend(x, y, col);
}

// قوس/حلقة بسماكة محددة (a0..a1 بالراديان؛ فرق >= 2π يعني حلقة كاملة)
inline void arcStroke(Canvas& c, float cx, float cy, float r, float a0, float a1, float thick, const Color& col) {
    const float pi2 = 6.2831853f, span = a1 - a0, h = thick * 0.5f;
    for (int y = (int)std::floor(cy - r - h - 1); y <= (int)std::ceil(cy + r + h + 1); y++)
        for (int x = (int)std::floor(cx - r - h - 1); x <= (int)std::ceil(cx + r + h + 1); x++) {
            float dx = x + .5f - cx, dy = y + .5f - cy;
            float cov = std::min(1.f, std::max(0.f, h - std::fabs(std::sqrt(dx * dx + dy * dy) - r) + .5f));
            if (cov <= 0.f) continue;
            if (span < pi2) {
                float da = std::fmod(std::atan2(dy, dx) - a0, pi2);
                if (da < 0) da += pi2;
                if (da > span) continue;
            }
            c.blend(x, y, { col.r, col.g, col.b, col.a * cov });
        }
}
inline void ringStroke(Canvas& c, float cx, float cy, float r, float thick, const Color& col) {
    arcStroke(c, cx, cy, r, 0.f, 6.2831853f, thick, col);
}

// خط بسماكة (مسافة إلى القطعة المستقيمة)
inline void strokeLine(Canvas& c, glm::vec2 a, glm::vec2 b, float thick, const Color& col) {
    float h = thick * 0.5f;
    int x0 = (int)std::floor(std::min(a.x, b.x) - h - 1), x1 = (int)std::ceil(std::max(a.x, b.x) + h + 1);
    int y0 = (int)std::floor(std::min(a.y, b.y) - h - 1), y1 = (int)std::ceil(std::max(a.y, b.y) + h + 1);
    glm::vec2 ab = b - a; float ab2 = std::max(glm::dot(ab, ab), 1e-6f);
    for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) {
        glm::vec2 p(x + .5f, y + .5f);
        float t = glm::clamp(glm::dot(p - a, ab) / ab2, 0.f, 1.f);
        float cov = std::min(1.f, std::max(0.f, h - glm::length(p - (a + ab * t)) + .5f));
        if (cov > 0.f) c.blend(x, y, { col.r, col.g, col.b, col.a * cov });
    }
}

// نقل الـ Canvas (لجعل عروق الخشب رأسية بدل أفقية)
inline Canvas transposed(const Canvas& s) {
    Canvas d(s.h, s.w);
    for (int y = 0; y < s.h; y++) for (int x = 0; x < s.w; x++)
        std::memcpy(&d.px[((size_t)x * d.w + y) * 4], &s.px[((size_t)y * s.w + x) * 4], 4);
    return d;
}

// ---------- ورق الحائط: خمري غامق + دمشقي ذهبي (يتكرر بسلاسة) ----------
inline void damaskMotif(Canvas& c, float cx, float cy, float S = 1.f) {
    const Color body = rgb(0x3a587a, 0.50f), deep = rgb(0x08111c, 0.45f);
    const Color gold = rgb(0xd4ab55, 0.92f), goldD = rgb(0xc89f4a, 0.55f);
    const float pi = 3.14159265f;
    c.ellipse(cx, cy, 24 * S, 50 * S, body);                 // لوزة النسيج
    c.ellipse(cx, cy, 17 * S, 40 * S, deep);
    // البتلة المركزية
    diamondFill(c, cx, cy - 6 * S, 8.f * S, 26.f * S, goldD);
    diamondFill(c, cx, cy - 6 * S, 4.f * S, 17.f * S, deep);
    diamondFill(c, cx, cy - 36 * S, 3.5f * S, 6.f * S, gold);
    // الأوراق الجانبية (قوسان متناظران)
    for (int sd = -1; sd <= 1; sd += 2) {
        float a0 = sd < 0 ? 0.15f * pi : -1.05f * pi, a1 = sd < 0 ? 1.05f * pi : -0.15f * pi;
        arcStroke(c, cx + sd * 17.f * S, cy + 12.f * S, 13.f * S, a0, a1, 2.2f * S, gold);
        c.circle(cx + sd * 30.f * S, cy + 12.f * S, 2.4f * S, gold);
        arcStroke(c, cx + sd * 11.f * S, cy - 18.f * S, 9.f * S, sd < 0 ? -0.2f * pi : 1.2f * pi, sd < 0 ? 0.95f * pi : 2.0f * pi, 1.6f * S, goldD);
    }
    // القاعدة: زهرة صغيرة
    c.circle(cx, cy + 30.f * S, 5.f * S, gold);
    c.circle(cx, cy + 30.f * S, 2.4f * S, deep);
    diamondFill(c, cx, cy + 43.f * S, 4.f * S, 6.f * S, gold);
    diamondFill(c, cx, cy - 52.f * S, 2.f * S, 3.f * S, goldD);
}

inline Canvas wallpaper() {
    const int N = 512;
    Canvas c(N, N);
    c.fill(rgb(0x19161a));                                   // أسود (غيّر اللون من هنا)
    // أشرطة خمرية غامقة بالتناوب مع الأسود (دورة 128 بكسل)
    for (int x = 0; x < N; x += 128) {
        c.rect(x, 0, 64, N, rgb(0x4a1020));                  // الخمري (غيّر اللون من هنا)
        c.rect(x + 20, 0, 24, N, { 1.f, 0.8f, 0.85f, 0.035f });   // لمعة ساتان بوسط الشريط
        c.rect(x, 0, 2, N, rgb(0x73213a, 0.65f));            // حواف أفتح قليلاً تفصل اللونين
        c.rect(x + 62, 0, 2, N, rgb(0x73213a, 0.65f));
        c.rect(x + 66, 0, 1, N, { 1.f, 1.f, 1.f, 0.04f });   // انعكاس رفيع على الأسود
    }
    // حبيبات قماش ناعمة
    Rng r(7);
    for (int i = 0; i < 50000; i++) {
        int x = (int)(r.next() * N), y = (int)(r.next() * N);
        c.blend(x, y, r.next() < 0.5f ? Color{1.f, 0.9f, 0.9f, r.next() * 0.05f} : Color{0.f, 0.f, 0.f, r.next() * 0.07f});
    }
    return c;
}

// ---------- أرضية خشب جوز داكن لامع (ألواح) ----------
inline Canvas woodFloor() {
    Canvas c(512, 512);
    Rng r(21);
    const int plankH = 64;
    for (int row = 0; row < 8; row++) {
        int y0 = row * plankH;
        int x = -(int)(r.next() * 200);
        while (x < 512) {
            int len = 150 + (int)(r.next() * 190);
            float hue = r.next(), ph = r.next() * 6.28f, ph2 = r.next() * 6.28f, ph3 = r.next() * 6.28f;
            glm::vec3 base = glm::mix(glm::vec3(0.17f, 0.115f, 0.08f), glm::vec3(0.29f, 0.195f, 0.125f), hue);
            bool knot = r.next() < 0.25f;
            float kx = x + 30 + r.next() * (len - 60), ky = y0 + 16 + r.next() * 32;
            for (int yy = 0; yy < plankH; yy++)
                for (int xx = std::max(0, x); xx < std::min(512, x + len); xx++) {
                    float g1 = std::sin(yy * 0.45f + ph + std::sin(xx * 0.03f + ph2) * 2.2f) * 0.5f + 0.5f;
                    float g2 = std::sin(yy * 1.9f + ph3 + std::sin(xx * 0.011f + ph) * 3.1f) * 0.5f + 0.5f;
                    float k = 0.78f + 0.15f * g1 + 0.07f * g2 + (r.next() - 0.5f) * 0.05f;
                    if (knot) {
                        float dx = (xx - kx) / 14.f, dy = (y0 + yy - ky) / 7.f, d = std::sqrt(dx * dx + dy * dy);
                        if (d < 1.f) k *= 0.62f + 0.3f * std::fabs(std::sin(d * 9.f));
                    }
                    float gloss = 1.f + 0.07f * (1.f - std::fabs(yy - plankH * 0.5f) / (plankH * 0.5f));
                    k *= gloss;
                    int ex = xx - x;
                    if (yy < 1 || yy >= plankH - 1 || ex < 1) k *= 0.30f;               // فاصل بين الألواح
                    else if (yy == 1 || ex == 1) k *= 1.28f;                             // حافة مشطوفة تلتقط الضوء
                    c.set(xx, y0 + yy, { base.x * k, base.y * k, base.z * k, 1.f });
                }
            x += len;
        }
    }
    return c;
}

// ---------- خشب ماهوجني/جوز غامق للأثاث (العروق أفقية؛ transposed للرأسي) ----------
inline Canvas woodGrain() {
    const int N = 256; const float pi2 = 6.2831853f;
    Canvas c(N, N);
    Rng r(5);
    float ph1 = r.next() * pi2, ph2 = r.next() * pi2, ph3 = r.next() * pi2;
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        float fx = x * pi2 / N, fy = y * pi2 / N;
        float warp = std::sin(fx * 1.f + ph1) * 0.55f + std::sin(fx * 3.f + ph2) * 0.18f;
        float g1 = std::sin(fy * 14.f + warp + ph3) * 0.5f + 0.5f;        // عروق عريضة
        float g2 = std::sin(fy * 53.f + warp * 2.f + ph1) * 0.5f + 0.5f;   // عروق رفيعة
        float g3 = std::sin(fy * 5.f + warp * 0.6f + ph2) * 0.5f + 0.5f;   // تدرج لوني واسع
        float k = 0.86f + 0.07f * g1 + 0.05f * g2 + (r.next() - 0.5f) * 0.035f;
        glm::vec3 base = glm::mix(glm::vec3(0.16f, 0.105f, 0.07f), glm::vec3(0.29f, 0.19f, 0.12f), g3 * 0.6f + g1 * 0.4f);
        c.set(x, y, { base.x * k * 1.1f, base.y * k * 1.1f, base.z * k * 1.1f, 1.f });
    }
    return c;
}

// ---------- سجادة دائرية رمادية بحلقة خمرية وإطار ذهبي (تحت الطاولة) ----------
inline Canvas rug() {
    const int S = 768; const float C = S * 0.5f;
    Canvas c(S, S);
    const glm::vec3 field(0.17f, 0.165f, 0.165f), band(0.085f, 0.08f, 0.085f);
    const Color gold = rgb(0xd0a84f, 0.92f), goldD = rgb(0xc89f4a, 0.5f);
    Rng r(13);
    for (int y = 0; y < S; y++) for (int x = 0; x < S; x++) {
        float rad = std::sqrt((x + .5f - C) * (x + .5f - C) + (y + .5f - C) * (y + .5f - C));
        float cov = std::min(1.f, std::max(0.f, C - rad + .5f));
        glm::vec3 col = (rad > C - 24.f ? band : field) * (1.08f - 0.2f * (rad / C) * (rad / C) + (r.next() - 0.5f) * 0.12f);
        c.set(x, y, { col.x, col.y, col.z, cov });
    }
    ringStroke(c, C, C, C - 8.f, 3.f, gold);
    ringStroke(c, C, C, C - 14.f, 1.f, goldD);
    ringStroke(c, C, C, C - 24.f, 6.f, rgb(0x5a1424, 0.9f));   // حلقة خمرية
    ringStroke(c, C, C, C - 33.f, 3.f, gold);
    ringStroke(c, C, C, C - 39.f, 1.f, goldD);
    for (int i = 0; i < 96; i++) {                                    // نقاط ذهبية على الإطار
        float a = i * 6.2831853f / 96.f;
        (void)a;
    }
    for (int i = 0; i < 72; i++) {                                    // معينات داخل الحقل
        float a = i * 6.2831853f / 72.f;
        diamondFill(c, C + std::cos(a) * (C - 56.f), C + std::sin(a) * (C - 56.f), 3.f, 3.f, rgb(0xc89f4a, 0.4f));
    }
    ringStroke(c, C, C, C - 72.f, 1.2f, goldD);
    return c;
}

// ---------- سطح الطاولة الدائرية: جوخ خمري + إطار وتفاصيل ذهبية + الأقسام الثلاثة ----------
inline Canvas tableTop() {
    const int S = 1024;
    const float R = cfg::TABLE_R, ppu = S / (2.f * R), C = S * 0.5f;     // ppu = بكسل لكل وحدة عالم
    Canvas c(S, S);
    auto X = [&](float wx) { return C + wx * ppu; };
    auto Y = [&](float wz) { return C + wz * ppu; };
    const Color gold = rgb(0xdab45c, 0.95f), goldS = rgb(0xd0a84f, 0.65f), goldF = rgb(0xe6c97a, 0.95f);
    const glm::vec3 leather(0.09f, 0.012f, 0.025f);

    // 1) القماش الخمري + حافة جلدية داكنة؛ خارج الدائرة شفاف
    Rng r(3);
    const glm::vec3 felt(0.25f, 0.040f, 0.080f);          // خمري غامق جداً
    for (int y = 0; y < S; y++) for (int x = 0; x < S; x++) {
        float dx = x + .5f - C, dy = y + .5f - C;
        float rad = std::sqrt(dx * dx + dy * dy) / ppu;                 // بوحدات العالم
        float cov = std::min(1.f, std::max(0.f, (R - rad) * ppu + .5f));
        if (cov <= 0.f) { c.set(x, y, { leather.x, leather.y, leather.z, 0.f }); continue; }
        float t = rad / R;
        float vig = 1.14f - 0.34f * t * t;
        float mott = std::sin(x * 0.013f) * std::sin(y * 0.017f) * 0.035f + std::sin(x * 0.05f + y * 0.031f) * 0.015f;
        float k = (1.f + (r.next() - 0.5f) * 0.11f + mott) * vig;
        glm::vec3 col = felt * k;
        if (rad > R - 0.18f) col = leather * (0.95f + (r.next() - 0.5f) * 0.12f);
        c.set(x, y, { col.x, col.y, col.z, cov });
    }
    for (int i = 0; i < 12000; i++) {                       // شعيرات قماش لامعة
        float a = r.next() * 6.2831853f, rr = std::sqrt(r.next()) * (R - 0.35f) * ppu;
        c.rect((int)(C + std::cos(a) * rr), (int)(C + std::sin(a) * rr), 3 + (int)(r.next() * 5), 1, { 1.f, 0.7f, 0.75f, 0.05f });
    }

    // 2) الإطار الدائري الذهبي (خطان) + نقاط زخرفية على الحافة الجلدية
    ringStroke(c, C, C, (R - 0.22f) * ppu, 0.12f * ppu, gold);          // حافة ذهبية سميكة
    ringStroke(c, C, C, (R - 0.40f) * ppu, 1.4f, goldS);
    for (int i = 0; i < 96; i++) {
        float a = i * 6.2831853f / 96.f;
        float px = C + std::cos(a) * (R - 0.08f) * ppu, py = C + std::sin(a) * (R - 0.08f) * ppu;
        if (i % 6 == 0) diamondFill(c, px, py, 3.5f, 3.5f, goldF);
        else c.circle(px, py, 1.1f, goldS);
    }

    // 3) فاصل مركزي بين نصفي الطاولة (الخصم / اللاعب)
    {
        float cx = C, hl = 6.0f * ppu;
        // الفاصل في منتصف الفراغ بين صف الخصم وصفّي اللاعب
        float cy = Y((cfg::ZONE_OPPONENT.cz + cfg::ZONE_OPPONENT.d / 2 + cfg::ZONE_DECK.cz - cfg::ZONE_DECK.d / 2) * 0.5f);
        strokeLine(c, { cx - hl, cy }, { cx - 46.f, cy }, 1.6f, goldS);
        strokeLine(c, { cx + 46.f, cy }, { cx + hl, cy }, 1.6f, goldS);
        for (int s = -1; s <= 1; s += 2) {
            strokeLine(c, { cx + s * hl, cy - 4.f }, { cx + s * 70.f, cy - 4.f }, 0.8f, rgb(0xd0a84f, 0.35f));
            strokeLine(c, { cx + s * hl, cy + 4.f }, { cx + s * 70.f, cy + 4.f }, 0.8f, rgb(0xd0a84f, 0.35f));
            diamondFill(c, cx + s * 32.f, cy, 5.f, 4.f, gold);
            c.circle(cx + s * 52.f, cy, 2.f, gold);
            c.circle(cx + s * hl, cy, 3.f, gold);
        }
        diamondFill(c, cx, cy, 18.f, 11.f, gold);
        diamondFill(c, cx, cy, 11.f, 6.f, rgb(0x3a0912));
        diamondFill(c, cx, cy, 3.5f, 2.f, goldF);
    }

    // 4) الأقسام الثلاثة (المواضع والأبعاد كما هي تماماً)
    auto zone = [&](const cfg::Zone& z, const char* label) {
        int x = (int)X(z.cx - z.w / 2), y = (int)Y(z.cz - z.d / 2);       // z السالب (الخصم) = أعلى الصورة
        int w = (int)(z.w * ppu), h = (int)(z.d * ppu);
        c.rect(x, y, w, h, rgb(0x14030a, 0.32f));
        c.strokeRect(x, y, w, h, 2, rgb(0xdab45c, 0.92f));
        c.dashedRect(x + 6, y + 6, w - 12, h - 12, 1, 7, rgb(0xe6c97a, 0.55f));
        float px[2] = { (float)x, (float)(x + w) }, py[2] = { (float)y, (float)(y + h) };
        for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) {
            diamondFill(c, px[i], py[j], 6.f, 6.f, gold);
            diamondFill(c, px[i], py[j], 2.f, 2.f, rgb(0x2b0710));
        }
        int tw = Canvas::textWidth(label, 3);
        int tx = x + (w - tw) / 2, ty = y + 9;
        c.text(label, tx, ty, 3, rgb(0xeed48a, 0.95f));
        float ly = ty + 10.f;
        for (int s = -1; s <= 1; s += 2) {
            float x0 = s < 0 ? tx - 14.f : tx + tw + 14.f;
            float x1 = s < 0 ? x + 22.f : x + w - 22.f;
            strokeLine(c, { x0, ly }, { x1, ly }, 1.2f, goldS);
            diamondFill(c, x0 + s * 6.f, ly, 4.f, 4.f, gold);
            diamondFill(c, x1, ly, 3.f, 3.f, gold);
        }
    };
    zone(cfg::ZONE_DECK, "DECK");
    zone(cfg::ZONE_PLAYED, "PLAYED");
    zone(cfg::ZONE_OPPONENT, "OPPONENT");
    return c;
}

// ---------- ظهر البطاقة ----------
inline Canvas cardBack() {
    Canvas c(256, 362);
    c.fill(rgb(0x7a1f2b));
    c.strokeRect(10, 10, 236, 342, 5, rgb(0xe8c872));
    for (int i = -400; i < 400; i += 20) {
        c.line({(float)i, 0}, {(float)i + 362, 362}, 1.5f, rgb(0xe8c872, 0.8f));
        c.line({(float)i + 362, 0}, {(float)i, 362}, 1.5f, rgb(0xe8c872, 0.8f));
    }
    c.circle(128, 181, 46, rgb(0xe8c872));
    c.circle(128, 181, 40, rgb(0x7a1f2b));
    // نجمة
    glm::vec2 ctr(128, 181);
    for (int i = 0; i < 5; i++) {
        float a1 = i * 1.2566f - 1.5708f, a2 = a1 + 2.5133f;
        c.triangle(ctr + 30.f * glm::vec2(std::cos(a1), std::sin(a1)), ctr + 30.f * glm::vec2(std::cos(a2), std::sin(a2)), ctr, rgb(0xe8c872));
    }
    return c;
}

// ---------- شخصيات بديلة أصلية (تُستخدم إن لم توجد صورة المستخدم) ----------
inline void fallbackCharacter(Canvas& g, int kind, int ax, int ay, int aw, int ah) {
    float cx = ax + aw / 2.f;
    static const int skyTop[5] = {0x2b1a5e, 0x3a6ea5, 0x1b1b2f, 0x16384a, 0x5a1a10};
    static const int skyBot[5] = {0x7b4fb8, 0xcfe3f3, 0x4a3f7a, 0x3fa7c9, 0xe8742c};
    Color a = rgb(skyTop[kind]), b = rgb(skyBot[kind]);
    for (int y = 0; y < ah; y++) {
        float t = (float)y / ah;
        g.rect(ax, ay + y, aw, 1, { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, 1.f });
    }
    Rng r(kind * 31 + 5);
    for (int i = 0; i < 40; i++) g.rect(ax + (int)(r.next() * aw), ay + (int)(r.next() * ah * 0.6f), 2, 2, {1, 1, 1, 0.6f});
    float by = ay + ah - 70.f;
    g.rect(ax, (int)by, aw, 70, {0, 0, 0, 0.35f});

    // لوحات أصلية مبسطة مستوحاة من الأعمال المطلوبة (لا تعتمد على ملفات خارجية).
    switch (kind) {
    case 0: { // Girl with a Pearl Earring
        g.ellipse(cx, by - 255, 92, 128, rgb(0x24334b));
        g.ellipse(cx, by - 265, 65, 96, rgb(0xe4b58c));
        g.ellipse(cx - 24, by - 280, 10, 7, rgb(0x28211d));
        g.ellipse(cx + 24, by - 280, 10, 7, rgb(0x28211d));
        g.line({cx - 18, by - 238}, {cx + 18, by - 238}, 5, rgb(0x9b4b43));
        g.circle(cx + 58, by - 205, 15, rgb(0xf4ead2)); g.circle(cx + 58, by - 205, 8, rgb(0xffffff));
        g.triangle({cx - 64, by - 340}, {cx + 75, by - 330}, {cx + 62, by - 405}, rgb(0x253047));
        g.triangle({cx - 68, by - 390}, {cx + 18, by - 426}, {cx + 73, by - 391}, rgb(0xc39a52));
        g.triangle({cx - 90, by}, {cx - 58, by - 170}, {cx + 70, by}, rgb(0x31516c));
        break; }
    case 1: { // The Scream
        g.rect(ax, ay, aw, ah, rgb(0xed8a46));
        for (int k = 0; k < 8; ++k) g.line({(float)ax, (float)(ay + 35 + k*45)}, {(float)(ax+aw), (float)(ay + 10 + k*45)}, 7, rgb(k%2 ? 0xe9c95d : 0x51436f, .8f));
        g.rect((int)cx - 125, (int)by - 150, 250, 150, rgb(0x182c39));
        g.ellipse(cx, by - 290, 74, 125, rgb(0xf0c79a));
        g.ellipse(cx, by - 280, 35, 74, rgb(0x3b2925));
        g.ellipse(cx - 27, by - 330, 10, 13, rgb(0x171717)); g.ellipse(cx + 27, by - 330, 10, 13, rgb(0x171717));
        g.ellipse(cx, by - 250, 21, 38, rgb(0x301d20));
        g.line({cx - 130, by}, {cx - 95, by - 250}, 22, rgb(0x25252e)); g.line({cx + 130, by}, {cx + 95, by - 250}, 22, rgb(0x25252e));
        break; }
    case 2: { // The Starry Night
        g.fill(rgb(0x173b78));
        for (int i=0;i<9;i++) { float yy=ay+45+i*48; g.line({(float)ax,yy},{(float)(ax+aw),yy-25}, 5, rgb(i%2?0x4b82bc:0xead56b,.8f)); }
        g.circle(cx+120, ay+115, 48, rgb(0xf4db72)); g.circle(cx+120, ay+115, 32, rgb(0x173b78));
        for (int i=0;i<16;i++) { float x=ax+30+(i*83)%aw, y=ay+40+(i*61)%250; g.circle(x,y, 9+(i%3)*4, rgb(0xf4dd77)); }
        for (int i=0;i<5;i++) g.line({ax+10.f,ay+130.f+i*58},{ax+aw-10.f,ay+110.f+i*58}, 4, rgb(0x77a9d2));
        g.triangle({(float)ax, (float)(ay+ah)}, {cx, (float)(ay+260)}, {(float)(ax+aw), (float)(ay+ah)}, rgb(0x102b48));
        g.ellipse(cx-110, by-95, 36, 150, rgb(0x111c35));
        break; }
    case 3: { // The Son of Man
        g.fill(rgb(0x78b7b3)); g.rect(ax, ay+ah-100, aw, 100, rgb(0x426c52));
        g.rect((int)cx-125,(int)by-190,250,190,rgb(0x242c3a));
        g.rect((int)cx-90,(int)by-290,180,110,rgb(0xb9c8c5));
        g.ellipse(cx,by-320,76,100,rgb(0xe1bd98));
        g.ellipse(cx,by-385,85,32,rgb(0x26333c)); g.rect((int)cx-85,(int)by-385,170,55,rgb(0x26333c));
        g.ellipse(cx,by-320,38,30,rgb(0x83b83e)); g.circle(cx,by-320,26,rgb(0x5b9c31));
        g.line({cx-18,by-305},{cx+18,by-335},5,rgb(0x79553d));
        break; }
    default: { // The Birth of Venus
        g.fill(rgb(0x91c9c2));
        for (int i=0;i<12;i++) g.line({ax+0.f,ay+35.f+i*42},{ax+aw-1.f,ay+20.f+i*42},3,rgb(0xd9e5cf,.65f));
        g.ellipse(cx,by-275,70,145,rgb(0xe5b99b));
        g.circle(cx,by-420,58,rgb(0xe9c1a2));
        for (int i=0;i<11;i++) g.line({cx-48.f+i*9,by-465.f},{cx-95.f+i*18,by-315.f},5,rgb(0x9a633f));
        g.triangle({cx-160,by},{cx-70,by-205},{cx+80,by},rgb(0x557b52));
        g.triangle({cx+170,by},{cx+65,by-205},{cx-80,by},rgb(0x668a54));
        g.circle(cx+145,by-120,20,rgb(0xf0db88));
        break; }
    }
}

// ---------- وجه البطاقة: إطار + صورة الشخصية ----------
inline Canvas cardFace(int index, bool* loadedUserImage = nullptr) {
    Canvas c(512, 725);
    static const int bg[5] = {0x2b2358, 0x233a58, 0x23583a, 0x58402a, 0x582a2a};
    c.fill(rgb(bg[index % 5]));
    c.strokeRect(10, 10, 492, 705, 8, rgb(0xe8c872));
    c.strokeRect(24, 24, 464, 677, 3, rgb(0xe8c872));
    const int ax = 36, ay = 90, aw = 440, ah = 520;
    const auto& def = cfg::CARD_DEFS[index % cfg::DECK_SIZE];
    std::vector<uint8_t> img; int iw = 0, ih = 0;
    if (loadImageRGBA(cfg::assetPath(def.image), img, iw, ih)) {
        c.drawCover(img, iw, ih, ax, ay, aw, ah);
        if (loadedUserImage) *loadedUserImage = true;
    } else {
        Canvas art(512, 725);                    // نرسم على لوحة مؤقتة ثم ننسخ المستطيل (يعمل كقص Clip)
        art.fill(rgb(bg[index % 5]));
        fallbackCharacter(art, def.fallback, ax, ay, aw, ah);
        for (int y = ay; y < ay + ah; y++)
            std::memcpy(&c.px[((size_t)y * c.w + ax) * 4], &art.px[((size_t)y * art.w + ax) * 4], (size_t)aw * 4);
    }
    c.strokeRect(ax, ay, aw, ah, 4, rgb(0xe8c872));
    c.rect(36, 34, 440, 40, {0, 0, 0, 0.45f});      // شريط الاسم (زخرفي)
    for (int i = 0; i < 3 + index % 3; i++) c.circle(206.f + i * 30.f, 654, 8, rgb(0xe8c872));  // نجوم الندرة
    return c;
}

// ---------- الملصق: عمل فني + تمزيق بقناع ألفا ----------
inline bool pointInPoly(const std::vector<glm::vec2>& p, float x, float y) {
    bool in = false;
    for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++)
        if (((p[i].y > y) != (p[j].y > y)) && (x < (p[j].x - p[i].x) * (y - p[i].y) / (p[j].y - p[i].y) + p[i].x)) in = !in;
    return in;
}
inline float distToPoly(const std::vector<glm::vec2>& p, glm::vec2 q) {
    float best = 1e9f;
    for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++) {
        glm::vec2 a = p[j], b = p[i], ab = b - a;
        float t = glm::clamp(glm::dot(q - a, ab) / std::max(glm::dot(ab, ab), 1e-6f), 0.f, 1.f);
        best = std::min(best, glm::length(q - (a + ab * t)));
    }
    return best;
}

inline Canvas poster() {
    const int W = 512, H = 368;
    Canvas c(W, H);
    std::vector<uint8_t> img; int iw = 0, ih = 0;
    if (loadImageRGBA(cfg::assetPath(cfg::POSTER_IMAGE), img, iw, ih)) {
        c.drawCover(img, iw, ih, 0, 0, W, H);
    } else {                                       // بورتريه موناليزا بديل مع خلفية ريفية
        c.fill(rgb(0x263c35));
        for (int y=0; y<H; ++y) {
            float t=(float)y/H;
            c.rect(0,y,W,1, {0.12f+0.16f*t,0.20f+0.12f*t,0.18f+0.06f*t,1});
        }
        c.ellipse(105,235,78,115,rgb(0x526d57)); c.ellipse(408,220,92,125,rgb(0x526d57));
        c.line({0,260},{180,210},4,rgb(0x8ba078)); c.line({330,220},{512,270},4,rgb(0x8ba078));
        c.ellipse(256,166,61,86,rgb(0xc99a70));
        c.ellipse(256,166,13,8,rgb(0x30251c));
        c.line({231,154},{245,151},4,rgb(0x36251d)); c.line({267,151},{282,154},4,rgb(0x36251d));
        c.line({239,198},{273,198},3,rgb(0x784338));
        c.triangle({165,368},{205,226},{307,226},rgb(0x302a25));
        c.triangle({347,368},{307,226},{205,226},rgb(0x302a25));
        c.ellipse(256,337,114,48,rgb(0x302a25));
        c.line({198,300},{224,355},5,rgb(0xd4bc91)); c.line({224,355},{246,345},5,rgb(0xd4bc91));
        c.strokeRect(12, 12, W - 24, H - 24, 3, rgb(0xd6c69b));
    }

    // مضلع التمزق: حافة يمنى مشرشرة + زاوية سفلية يمنى مقطوعة قطرياً
    Rng r(99);
    std::vector<glm::vec2> poly = { {0, 0}, {(float)W, 0} };
    for (float y = 0; y <= H * 0.38f; y += 20) poly.push_back({ W - r.next() * 16.f, y });
    glm::vec2 s(W - 6.f, H * 0.38f), e(W * 0.62f, (float)H);
    for (int i = 1; i <= 14; i++) {
        float t = i / 14.f, jag = (i % 2 ? 1.f : -1.f) * (4.f + r.next() * 12.f);
        poly.push_back(glm::mix(s, e, t) + glm::vec2(jag, jag * 0.5f));
    }
    for (float x = W * 0.35f; x > 0; x -= 26) poly.push_back({ x, H - r.next() * 6.f });
    poly.push_back({0, (float)H});

    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        uint8_t* p = &c.px[((size_t)y * W + x) * 4];
        glm::vec2 q(x + .5f, y + .5f);
        if (!pointInPoly(poly, q.x, q.y)) { p[3] = 0; continue; }             // مفقود (شفاف)
        if (distToPoly(poly, q) < 4.5f) { p[0] = 250; p[1] = 246; p[2] = 236; } // حافة ورقية بيضاء
    }
    return c;
}

} // namespace tex
