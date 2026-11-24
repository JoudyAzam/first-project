// ============================================================
//  Mesh: مجسمات أولية (صندوق، مستوي، أسطوانة، كرة) بصيغة pos|normal|uv
// ============================================================
#pragma once
#include <glad/gl.h>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>

struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei count = 0;

    void build(const std::vector<float>& v, const std::vector<unsigned>& idx) {
        count = (GLsizei)idx.size();
        glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
        const GLsizei stride = 8 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);                   glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }
    void draw() const {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    }
};

namespace prim {

struct Data { std::vector<float> v; std::vector<unsigned> i; };

inline void vert(Data& d, glm::vec3 p, glm::vec3 n, glm::vec2 uv) {
    d.v.insert(d.v.end(), { p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y });
}
inline void quad(Data& d, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 e, glm::vec3 n) {
    unsigned base = (unsigned)(d.v.size() / 8);
    vert(d, a, n, {0, 0}); vert(d, b, n, {1, 0}); vert(d, c, n, {1, 1}); vert(d, e, n, {0, 1});
    d.i.insert(d.i.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
}

// مستوي في XY ينظر نحو +Z ، مركزه (0,0,0)
inline Mesh plane(float w, float h) {
    Data d; float x = w / 2, y = h / 2;
    quad(d, {-x, -y, 0}, {x, -y, 0}, {x, y, 0}, {-x, y, 0}, {0, 0, 1});
    Mesh m; m.build(d.v, d.i); return m;
}

// صندوق مركزه (0,0,0)
inline Mesh box(float w, float h, float d_) {
    Data d; float x = w / 2, y = h / 2, z = d_ / 2;
    quad(d, {-x, -y,  z}, { x, -y,  z}, { x,  y,  z}, {-x,  y,  z}, {0, 0, 1});
    quad(d, { x, -y, -z}, {-x, -y, -z}, {-x,  y, -z}, { x,  y, -z}, {0, 0, -1});
    quad(d, { x, -y,  z}, { x, -y, -z}, { x,  y, -z}, { x,  y,  z}, {1, 0, 0});
    quad(d, {-x, -y, -z}, {-x, -y,  z}, {-x,  y,  z}, {-x,  y, -z}, {-1, 0, 0});
    quad(d, {-x,  y,  z}, { x,  y,  z}, { x,  y, -z}, {-x,  y, -z}, {0, 1, 0});
    quad(d, {-x, -y, -z}, { x, -y, -z}, { x, -y,  z}, {-x, -y,  z}, {0, -1, 0});
    Mesh m; m.build(d.v, d.i); return m;
}

// أسطوانة/مخروط مقطوع على المحور Y ، مركزها (0,0,0)
inline Mesh cylinder(float rTop, float rBot, float h, int seg, bool caps) {
    Data d; const float pi2 = 6.2831853f;
    float slope = (rBot - rTop) / h;
    for (int s = 0; s <= seg; s++) {
        float a = pi2 * s / seg, c = std::cos(a), sn = std::sin(a);
        glm::vec3 n = glm::normalize(glm::vec3(c, slope, sn));
        vert(d, {c * rBot, -h / 2, sn * rBot}, n, {(float)s / seg, 0});
        vert(d, {c * rTop,  h / 2, sn * rTop}, n, {(float)s / seg, 1});
    }
    for (int s = 0; s < seg; s++) {
        unsigned a = s * 2;
        d.i.insert(d.i.end(), { a, a + 2, a + 1, a + 1, a + 2, a + 3 });
    }
    if (caps) {
        for (int side = 0; side < 2; side++) {
            float y = side ? h / 2 : -h / 2, r = side ? rTop : rBot;
            glm::vec3 n(0, side ? 1.f : -1.f, 0);
            unsigned center = (unsigned)(d.v.size() / 8);
            vert(d, {0, y, 0}, n, {.5f, .5f});
            for (int s = 0; s <= seg; s++) {
                float a = pi2 * s / seg;
                vert(d, {std::cos(a) * r, y, std::sin(a) * r}, n, {.5f + .5f * std::cos(a), .5f + .5f * std::sin(a)});
            }
            for (int s = 0; s < seg; s++) {
                if (side) d.i.insert(d.i.end(), { center, center + 1 + s + 1, center + 1 + s });
                else      d.i.insert(d.i.end(), { center, center + 1 + s, center + 1 + s + 1 });
            }
        }
    }
    Mesh m; m.build(d.v, d.i); return m;
}

// كرة نصف قطرها r
inline Mesh sphere(float r, int seg, int rings) {
    Data d; const float pi = 3.14159265f;
    for (int y = 0; y <= rings; y++) for (int x = 0; x <= seg; x++) {
        float u = (float)x / seg, v = (float)y / rings;
        glm::vec3 n(std::cos(u * 2 * pi) * std::sin(v * pi), std::cos(v * pi), std::sin(u * 2 * pi) * std::sin(v * pi));
        vert(d, n * r, n, {u, v});
    }
    for (int y = 0; y < rings; y++) for (int x = 0; x < seg; x++) {
        unsigned a = y * (seg + 1) + x, b = a + seg + 1;
        d.i.insert(d.i.end(), { a, b, a + 1, a + 1, b, b + 1 });
    }
    Mesh m; m.build(d.v, d.i); return m;
}

} // namespace prim
