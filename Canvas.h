// ============================================================
//  Canvas: صورة RGBA على الـ CPU مع أدوات رسم بسيطة
//  تُستخدم لتوليد التكستشرات برمجياً ثم رفعها إلى OpenGL
// ============================================================
#pragma once
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <glm/glm.hpp>

// stb_image اختياري: إن وُجد نحمّل صور المستخدم، وإلا نستخدم الرسم البديل
#if defined(__has_include)
#  if __has_include(<stb_image.h>)
#    define STB_IMAGE_IMPLEMENTATION
#    include <stb_image.h>
#    define HAVE_STB_IMAGE 1
#  endif
#endif

struct Color { float r, g, b, a; };
inline Color rgb(int hex, float a = 1.f) {
    return { ((hex >> 16) & 255) / 255.f, ((hex >> 8) & 255) / 255.f, (hex & 255) / 255.f, a };
}

// مولد أرقام عشوائية ثابت (Seed) كي يبقى الشكل نفسه في كل تشغيل
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed) {}
    float next() { s = s * 1664525u + 1013904223u; return (s >> 8) / 16777216.f; }
};

class Canvas {
public:
    int w, h;
    std::vector<uint8_t> px;   // RGBA

    Canvas(int w_, int h_) : w(w_), h(h_), px((size_t)w_ * h_ * 4, 255) {}

    void blend(int x, int y, const Color& c) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        uint8_t* p = &px[((size_t)y * w + x) * 4];
        float a = c.a;
        p[0] = (uint8_t)(p[0] * (1 - a) + c.r * 255 * a);
        p[1] = (uint8_t)(p[1] * (1 - a) + c.g * 255 * a);
        p[2] = (uint8_t)(p[2] * (1 - a) + c.b * 255 * a);
        p[3] = 255;
    }
    void set(int x, int y, const Color& c) {
        if (x < 0 || y < 0 || x >= w || y >= h) return;
        uint8_t* p = &px[((size_t)y * w + x) * 4];
        p[0] = (uint8_t)(c.r * 255); p[1] = (uint8_t)(c.g * 255);
        p[2] = (uint8_t)(c.b * 255); p[3] = (uint8_t)(c.a * 255);
    }

    void fill(const Color& c) { for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) set(x, y, c); }
    void rect(int x0, int y0, int rw, int rh, const Color& c) {
        for (int y = y0; y < y0 + rh; y++) for (int x = x0; x < x0 + rw; x++) blend(x, y, c);
    }
    void strokeRect(int x0, int y0, int rw, int rh, int t, const Color& c) {
        rect(x0, y0, rw, t, c); rect(x0, y0 + rh - t, rw, t, c);
        rect(x0, y0, t, rh, c); rect(x0 + rw - t, y0, t, rh, c);
    }
    void dashedRect(int x0, int y0, int rw, int rh, int t, int dash, const Color& c) {
        for (int x = x0; x < x0 + rw; x += dash * 2) {
            rect(x, y0, std::min(dash, x0 + rw - x), t, c);
            rect(x, y0 + rh - t, std::min(dash, x0 + rw - x), t, c);
        }
        for (int y = y0; y < y0 + rh; y += dash * 2) {
            rect(x0, y, t, std::min(dash, y0 + rh - y), c);
            rect(x0 + rw - t, y, t, std::min(dash, y0 + rh - y), c);
        }
    }
    void ellipse(float cx, float cy, float rx, float ry, const Color& c) {
        for (int y = (int)(cy - ry); y <= (int)(cy + ry); y++)
            for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) {
                float dx = (x - cx) / rx, dy = (y - cy) / ry;
                if (dx * dx + dy * dy <= 1.f) blend(x, y, c);
            }
    }
    void circle(float cx, float cy, float r, const Color& c) { ellipse(cx, cy, r, r, c); }
    void triangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, const Color& col) {
        int minx = (int)std::floor(std::min({a.x, b.x, c.x})), maxx = (int)std::ceil(std::max({a.x, b.x, c.x}));
        int miny = (int)std::floor(std::min({a.y, b.y, c.y})), maxy = (int)std::ceil(std::max({a.y, b.y, c.y}));
        auto edge = [](glm::vec2 p, glm::vec2 q, glm::vec2 r) { return (r.x - p.x) * (q.y - p.y) - (r.y - p.y) * (q.x - p.x); };
        for (int y = miny; y <= maxy; y++) for (int x = minx; x <= maxx; x++) {
            glm::vec2 p(x + .5f, y + .5f);
            float e1 = edge(a, b, p), e2 = edge(b, c, p), e3 = edge(c, a, p);
            if ((e1 >= 0 && e2 >= 0 && e3 >= 0) || (e1 <= 0 && e2 <= 0 && e3 <= 0)) blend(x, y, col);
        }
    }
    void line(glm::vec2 a, glm::vec2 b, float thick, const Color& c) {
        float len = glm::length(b - a); int n = std::max(1, (int)(len * 2));
        for (int i = 0; i <= n; i++) circle(a.x + (b.x - a.x) * i / n, a.y + (b.y - a.y) * i / n, thick / 2, c);
    }

    // ---- خط نقطي 5x7 بسيط للكتابة على الطاولة (أحرف إنجليزية كبيرة) ----
    void text(const std::string& s, int x, int y, int scale, const Color& c) {
        for (char ch : s) {
            const char* const* g = glyph(ch);
            if (g) for (int r = 0; r < 7; r++) for (int col = 0; col < 5; col++)
                if (g[r][col] == '1') rect(x + col * scale, y + r * scale, scale, scale, c);
            x += 6 * scale;
        }
    }
    static int textWidth(const std::string& s, int scale) { return (int)s.size() * 6 * scale - scale; }

    // ---- رسم صورة مصدر بنمط cover داخل مستطيل (للصور المحمّلة من القرص) ----
    void drawCover(const std::vector<uint8_t>& src, int sw, int sh, int x0, int y0, int rw, int rh) {
        float s = std::max((float)rw / sw, (float)rh / sh);
        float cropW = rw / s, cropH = rh / s;
        float ox = (sw - cropW) / 2, oy = (sh - cropH) / 2;
        for (int y = 0; y < rh; y++) for (int x = 0; x < rw; x++) {
            int sx = std::min(sw - 1, (int)(ox + (x + .5f) / s)), sy = std::min(sh - 1, (int)(oy + (y + .5f) / s));
            const uint8_t* p = &src[((size_t)sy * sw + sx) * 4];
            set(x0 + x, y0 + y, { p[0] / 255.f, p[1] / 255.f, p[2] / 255.f, 1.f });
        }
    }

private:
    static const char* const* glyph(char c) {
        static const char* D[] = {"11110","10001","10001","10001","10001","10001","11110"};
        static const char* E[] = {"11111","10000","10000","11110","10000","10000","11111"};
        static const char* C[] = {"01110","10001","10000","10000","10000","10001","01110"};
        static const char* K[] = {"10001","10010","10100","11000","10100","10010","10001"};
        static const char* P[] = {"11110","10001","10001","11110","10000","10000","10000"};
        static const char* L[] = {"10000","10000","10000","10000","10000","10000","11111"};
        static const char* A[] = {"01110","10001","10001","11111","10001","10001","10001"};
        static const char* Y[] = {"10001","10001","01010","00100","00100","00100","00100"};
        static const char* O[] = {"01110","10001","10001","10001","10001","10001","01110"};
        static const char* N[] = {"10001","11001","10101","10011","10001","10001","10001"};
        static const char* T[] = {"11111","00100","00100","00100","00100","00100","00100"};
        switch (c) {
            case 'D': return D; case 'E': return E; case 'C': return C; case 'K': return K;
            case 'P': return P; case 'L': return L; case 'A': return A; case 'Y': return Y;
            case 'O': return O; case 'N': return N; case 'T': return T; default: return nullptr;
        }
    }
};

// تحميل صورة من القرص إلى RGBA (يرجع false إن لم يوجد stb أو الملف)
inline bool loadImageRGBA(const std::string& path, std::vector<uint8_t>& out, int& w, int& h) {
#ifdef HAVE_STB_IMAGE
    int n = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, 4);
    if (!data) return false;
    out.assign(data, data + (size_t)w * h * 4);
    stbi_image_free(data);
    return true;
#else
    (void)path; (void)out; (void)w; (void)h;
    return false;
#endif
}
