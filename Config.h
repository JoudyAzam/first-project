// ============================================================
//  إعدادات المشروع (أبعاد، شخصيات، مدة الحركات)
//  عدّل هنا بسهولة: أسماء ملفات صور الشخصيات والملصق
// ============================================================
#pragma once
#include <string>
#include <glm/glm.hpp>

#ifndef ASSET_DIR
#define ASSET_DIR "assets"   // يحدده CMake تلقائياً بالمسار الكامل
#endif

namespace cfg {

// الطاولة: سطحها العلوي عند y = 0 ، مركزها عند (0,0,0)
constexpr float TABLE_W = 16.f, TABLE_D = 10.f, TABLE_T = 0.4f, TABLE_H = 3.f;
// الطاولة الدائرية (شكل بصري فقط): نصف القطر يغطي زوايا الأقسام الثلاثة
constexpr float TABLE_R = 7.8f;
constexpr float RUG_R = 9.2f;

// البطاقة
constexpr float CARD_W = 1.2f, CARD_H = 1.7f, CARD_T = 0.03f, CARD_SPACING = 1.32f;
constexpr int   DECK_SIZE = 5;          // 5 بطاقات = أيضاً الحد الأقصى لضغطات G

// الغرفة
constexpr float ROOM_X_MIN = -15.f, ROOM_X_MAX = 15.f;
constexpr float ROOM_Z_MIN = -11.f, ROOM_Z_MAX = 14.5f;
constexpr float ROOM_HEIGHT = 16.f;
constexpr float FLOOR_Y = -TABLE_H;

// مدة الحركات (ثانية)
constexpr float ANIM_CAMERA = 1.0f, ANIM_FLIP = 0.7f, ANIM_DEAL = 0.9f, ANIM_STAGGER = 0.07f;

// أقسام الطاولة (مركز + أبعاد)
struct Zone { float cx, cz, w, d; };
constexpr Zone ZONE_DECK     { -3.4f,  1.2f,  6.9f, 2.7f };
constexpr Zone ZONE_PLAYED   {  3.4f,  1.2f,  6.9f, 2.7f };
constexpr Zone ZONE_OPPONENT {  0.0f, -2.4f,  6.9f, 2.7f };

// ------------------------------------------------------------
// البطاقات بالترتيب المطلوب؛ يمكن استبدالها بصور JPG في assets/cards/.
// عند غياب الصور تُرسم لوحات بديلة مولدة برمجياً تلقائياً.
// ------------------------------------------------------------
struct CardDef { const char* image; int fallback; };
inline const CardDef CARD_DEFS[DECK_SIZE] = {
    { "cards/card1.png", 0 },   // Girl with a Pearl Earring
    { "cards/card2.jpg", 1 },   // The Scream
    { "cards/card3.jpg", 2 },   // The Starry Night
    { "cards/card4.jpg", 3 },   // The Son of Man
    { "cards/card5.jpg", 4 },   // The Birth of Venus
};

// الملصق (العمل الفني) - يُمزَّق برمجياً
inline const char* POSTER_IMAGE = "poster/poster.jpg";
constexpr float POSTER_W = 5.f, POSTER_H = 3.6f;

inline std::string assetPath(const std::string& rel) { return std::string(ASSET_DIR) + "/" + rel; }

} // namespace cfg
