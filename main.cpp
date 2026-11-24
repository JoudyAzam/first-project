// ============================================================
//  لعبة بطاقات - C++17 / OpenGL 3.3 (GLFW + GLAD + GLM)
//
//   1 : كاميرا من فوق الطاولة تماماً (البطاقات مسطحة)
//   2 : منظور اللاعب (البطاقات واقفة)
//   G : سحب بطاقة عشوائية من المخزون إلى منطقة اللعب (حتى 5 مرات)
//   Esc : خروج
// ============================================================
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Config.h"
#include "Shader.h"
#include "Scene.h"
#include "CardGame.h"
#include "CameraRig.h"

// ---------- حالة التطبيق (تُستخدم من callbacks) ----------
struct App {
    int fbW = 1280, fbH = 720;
    bool keyView1 = false, keyView2 = false, keyDeal = false;
    bool aspectChanged = false;
} g_app;

static void onKey(GLFWwindow* win, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;                       // نتجاهل التكرار (Repeat)
    switch (key) {
        case GLFW_KEY_1: case GLFW_KEY_KP_1: g_app.keyView1 = true; break;
        case GLFW_KEY_2: case GLFW_KEY_KP_2: g_app.keyView2 = true; break;
        case GLFW_KEY_G:                     g_app.keyDeal  = true; break;
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, GLFW_TRUE); break;
        default: break;
    }
}
static void onResize(GLFWwindow*, int w, int h) { if (w > 0 && h > 0) { g_app.fbW = w; g_app.fbH = h; g_app.aspectChanged = true; } }

// ---------- خريطة الظل ----------
struct ShadowMap {
    GLuint fbo = 0, depth = 0; int size = 2048;
    void init() {
        glGenTextures(1, &depth);
        glBindTexture(GL_TEXTURE_2D, depth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[] = {1, 1, 1, 1};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
        glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

#ifdef CARDGAME_SELFTEST
static void saveScreenshot(const char* path, int w, int h) {
    std::vector<unsigned char> px((size_t)w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    FILE* f = std::fopen(path, "wb");
    std::fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; y--) std::fwrite(&px[(size_t)y * w * 3], 1, (size_t)w * 3, f);
    std::fclose(f);
}
#endif

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "GLFW init failed\n"); return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Card Game", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "Window creation failed (OpenGL 3.3 required)\n"); glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) { std::fprintf(stderr, "GLAD load failed\n"); return 1; }
    glfwSetKeyCallback(window, onKey);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwGetFramebufferSize(window, &g_app.fbW, &g_app.fbH);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // ---------- الموارد ----------
    Shader mainSh, depthSh;
    mainSh.build(shaders::MAIN_VS, shaders::MAIN_FS);
    depthSh.build(shaders::DEPTH_VS, shaders::DEPTH_FS);

    ShadowMap shadow; shadow.init();
    auto scene = std::make_unique<Scene>();
    scene->build();
    CardGame game; game.init(*scene);
    CameraRig rig((float)g_app.fbW / std::max(1, g_app.fbH));

    // مصفوفة ضوء الشمس (إسقاط متعامد) للظل
    glm::mat4 lightSpace = glm::ortho(-17.f, 17.f, -17.f, 17.f, 1.f, 70.f) *
                           glm::lookAt(scene->sunDir() * 30.f, glm::vec3(0), glm::vec3(0, 1, 0));

    std::printf("Controls:  1 = top view | 2 = player view | G = draw card | Esc = quit\n");

    auto updateTitle = [&]() {
        char t[160];
        std::snprintf(t, sizeof t, "Card Game  |  View: %s  |  Deck: %zu  |  Played: %zu/%d",
                      rig.mode == ViewMode::Top ? "1 (top)" : "2 (player)", game.deck.size(), game.played.size(), cfg::DECK_SIZE);
        glfwSetWindowTitle(window, t);
    };
    updateTitle();

    double last = glfwGetTime();
#ifdef CARDGAME_SELFTEST
    int frame = 0;
#endif

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = std::min((float)(now - last), 0.05f);
        last = now;
#ifdef CARDGAME_SELFTEST
        dt = 1.f / 30.f;
        struct Step { int f; const char* what; };
        static const Step steps[] = {
            {60, "shot:/tmp/shots/t1_top.ppm"}, {61, "g"}, {62, "g"}, {160, "shot:/tmp/shots/t2_top_g2.ppm"},
            {161, "2"}, {300, "shot:/tmp/shots/t3_player.ppm"},
            {301, "g"}, {302, "g"}, {303, "g"}, {304, "g"}, {305, "g"}, {306, "g"}, {307, "g"},
            {450, "shot:/tmp/shots/t4_player_all.ppm"}, {451, "1"}, {600, "shot:/tmp/shots/t5_top_all.ppm"}, {601, "quit"},
        };
        for (const Step& s : steps) if (s.f == frame) {
            std::string w = s.what;
            if (w == "g") g_app.keyDeal = true;
            else if (w == "1") g_app.keyView1 = true;
            else if (w == "2") g_app.keyView2 = true;
            else if (w == "quit") glfwSetWindowShouldClose(window, GLFW_TRUE);
            else if (w.rfind("shot:", 0) == 0) saveScreenshot(w.c_str() + 5, g_app.fbW, g_app.fbH);
        }
        frame++;
#endif
        glfwPollEvents();

        // ---------- الإدخال ----------
        if (g_app.keyView1) { rig.setMode(ViewMode::Top);    game.setMode(ViewMode::Top);    updateTitle(); }
        if (g_app.keyView2) { rig.setMode(ViewMode::Player); game.setMode(ViewMode::Player); updateTitle(); }
        if (g_app.keyDeal)  { game.deal(); updateTitle(); }
        g_app.keyView1 = g_app.keyView2 = g_app.keyDeal = false;
        if (g_app.aspectChanged) { rig.setAspect((float)g_app.fbW / std::max(1, g_app.fbH)); g_app.aspectChanged = false; }

        // ---------- التحديث ----------
        rig.update(dt);
        game.update(dt);

        std::vector<Renderable> cardItems;
        game.appendRenderables(cardItems, scene->whiteTex);

        // ---------- 1) تمرير الظل ----------
        glViewport(0, 0, shadow.size, shadow.size);
        glBindFramebuffer(GL_FRAMEBUFFER, shadow.fbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthSh.use();
        depthSh.setMat4("uLightSpace", lightSpace);
        for (const auto& r : scene->items) if (r.castShadow) drawRenderable(depthSh, r, true);
        for (const auto& r : cardItems)    if (r.castShadow) drawRenderable(depthSh, r, true);

        // ---------- 2) التمرير الرئيسي ----------
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, g_app.fbW, g_app.fbH);
        glClearColor(0.08f, 0.07f, 0.06f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainSh.use();
        mainSh.setMat4("uView", rig.view());
        mainSh.setMat4("uProj", rig.proj());
        mainSh.setMat4("uLightSpace", lightSpace);
        mainSh.setVec3("uViewPos", rig.position());
        mainSh.setVec3("uAmbient", glm::vec3(0.30f, 0.28f, 0.26f));
        mainSh.setVec3("uSunDir", scene->sunDir());
        mainSh.setVec3("uSunColor", glm::vec3(0.95f, 0.88f, 0.75f));
        int np = (int)std::min<size_t>(3, scene->lights.size());
        mainSh.setInt("uNPoint", np);
        for (int i = 0; i < np; i++) {
            std::string idx = std::to_string(i);
            mainSh.setVec3(("uPPos[" + idx + "]").c_str(), scene->lights[i].pos);
            mainSh.setVec3(("uPCol[" + idx + "]").c_str(), scene->lights[i].color);
        }
        mainSh.setInt("uTex", 0);
        mainSh.setInt("uShadow", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadow.depth);
        glActiveTexture(GL_TEXTURE0);

        for (const auto& r : scene->items) drawRenderable(mainSh, r, false);
        for (const auto& r : cardItems)    drawRenderable(mainSh, r, false);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
