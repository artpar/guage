/* Test helper for FFI struct-by-value passing tests.
 * Compile: cc -shared -o test_ffi_structs_helper.dylib test_ffi_structs_helper.c
 * (or .so on Linux)
 */

#include <math.h>

typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z, w; } Vec4;
typedef struct { float x, y, width, height; } Rect;
typedef struct { unsigned char r, g, b, a; } Color;

/* Pass struct by value, return scalar */
float vec2_sum(Vec2 v) {
    return v.x + v.y;
}

/* Return struct by value */
Vec2 vec2_make(float x, float y) {
    return (Vec2){x, y};
}

/* Two struct args */
float vec2_dist(Vec2 a, Vec2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx*dx + dy*dy);
}

/* Vec2 + scalar args mixed */
float vec2_dot_scaled(Vec2 a, Vec2 b, float scale) {
    return (a.x * b.x + a.y * b.y) * scale;
}

/* Struct with 4 float fields (HFA) */
float rect_area(Rect r) {
    return r.width * r.height;
}

Rect rect_make(float x, float y, float w, float h) {
    return (Rect){x, y, w, h};
}

/* Struct with uint8 fields (Color — 4 bytes, GPR class) */
int color_sum(Color c) {
    return (int)c.r + (int)c.g + (int)c.b + (int)c.a;
}

Color color_make(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (Color){r, g, b, a};
}

/* Vec2 return + Vec2 arg */
Vec2 vec2_negate(Vec2 v) {
    return (Vec2){-v.x, -v.y};
}

/* Scalar + struct mixed */
Vec2 vec2_scale(float s, Vec2 v) {
    return (Vec2){v.x * s, v.y * s};
}
