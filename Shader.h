// ============================================================
//  Shader: برنامج الإضاءة (Blinn-Phong + ظل Directional + نقاط ضوء) وبرنامج عمق الظل
// ============================================================
#pragma once
#include <glad/gl.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    GLuint id = 0;

    void build(const char* vs, const char* fs) {
        GLuint v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
        id = glCreateProgram();
        glAttachShader(id, v); glAttachShader(id, f);
        glLinkProgram(id);
        GLint ok = 0; glGetProgramiv(id, GL_LINK_STATUS, &ok);
        if (!ok) { char log[1024]; glGetProgramInfoLog(id, 1024, nullptr, log); std::fprintf(stderr, "Link error:\n%s\n", log); std::exit(1); }
        glDeleteShader(v); glDeleteShader(f);
    }
    void use() const { glUseProgram(id); }
    GLint loc(const char* n) const { return glGetUniformLocation(id, n); }
    void setInt(const char* n, int v) const { glUniform1i(loc(n), v); }
    void setFloat(const char* n, float v) const { glUniform1f(loc(n), v); }
    void setVec2(const char* n, const glm::vec2& v) const { glUniform2fv(loc(n), 1, glm::value_ptr(v)); }
    void setVec3(const char* n, const glm::vec3& v) const { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }
    void setMat4(const char* n, const glm::mat4& m) const { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }

private:
    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[2048]; glGetShaderInfoLog(s, 2048, nullptr, log); std::fprintf(stderr, "Shader compile error:\n%s\n", log); std::exit(1); }
        return s;
    }
};

namespace shaders {

inline const char* MAIN_VS = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uModel, uView, uProj, uLightSpace;
uniform vec2 uUVScale;
out vec3 vPos; out vec3 vNormal; out vec2 vUV; out vec4 vLightPos;
void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vPos = wp.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vUV = aUV * uUVScale;
    vLightPos = uLightSpace * wp;
    gl_Position = uProj * uView * wp;
}
)GLSL";

inline const char* MAIN_FS = R"GLSL(
#version 330 core
in vec3 vPos; in vec3 vNormal; in vec2 vUV; in vec4 vLightPos;
out vec4 FragColor;
uniform sampler2D uTex;
uniform sampler2D uShadow;
uniform vec3  uTint;
uniform float uEmissive;
uniform float uSpec;
uniform float uShine;
uniform vec3  uViewPos;
uniform vec3  uAmbient;
uniform vec3  uSunDir;      // اتجاه نحو الشمس
uniform vec3  uSunColor;
uniform int   uNPoint;
uniform vec3  uPPos[3];
uniform vec3  uPCol[3];

float shadowAt(vec4 lp, float ndl) {
    vec3 p = lp.xyz / lp.w * 0.5 + 0.5;
    if (p.z > 1.0 || p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0) return 0.0;
    float bias = max(0.003 * (1.0 - ndl), 0.0006);
    vec2 ts = 1.0 / vec2(textureSize(uShadow, 0));
    float s = 0.0;
    for (int x = -1; x <= 1; x++) for (int y = -1; y <= 1; y++) {
        float d = texture(uShadow, p.xy + vec2(x, y) * ts).r;
        s += (p.z - bias > d) ? 1.0 : 0.0;
    }
    return s / 9.0;
}

void main() {
    vec4 t = texture(uTex, vUV);
    if (t.a < 0.5) discard;                       // الأجزاء الممزقة من الملصق
    vec3 albedo = pow(t.rgb, vec3(2.2)) * pow(uTint, vec3(2.2));
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;
    vec3 V = normalize(uViewPos - vPos);

    vec3 L = normalize(uSunDir);
    float ndl = max(dot(N, L), 0.0);
    float sh = shadowAt(vLightPos, ndl);
    float sp = pow(max(dot(N, normalize(L + V)), 0.0), uShine) * uSpec;
    vec3 col = albedo * uAmbient + (albedo * ndl + vec3(sp)) * uSunColor * (1.0 - sh);

    for (int i = 0; i < uNPoint; i++) {
        vec3 d = uPPos[i] - vPos;
        float dist = length(d);
        vec3 Lp = d / dist;
        float att = 1.0 / (1.0 + 0.03 * dist + 0.006 * dist * dist);
        float nl = max(dot(N, Lp), 0.0);
        float spp = pow(max(dot(N, normalize(Lp + V)), 0.0), uShine) * uSpec;
        col += (albedo * nl + vec3(spp)) * uPCol[i] * att;
    }
    col += albedo * uEmissive;
    FragColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);
}
)GLSL";

inline const char* DEPTH_VS = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uModel, uLightSpace;
void main() { gl_Position = uLightSpace * uModel * vec4(aPos, 1.0); }
)GLSL";

inline const char* DEPTH_FS = R"GLSL(
#version 330 core
void main() {}
)GLSL";

} // namespace shaders
