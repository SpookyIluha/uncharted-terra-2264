#ifndef ENGINE_GFX_H
#define ENGINE_GFX_H

#include <libdragon.h>
#include <time.h>
#include <display.h>


#ifdef __cplusplus
extern "C"{
#endif

/*
 * Sometimes the dump screen for Libdragon has an issue
 * where it doesn't show what function an assertion happened
 * inside of, so I just decided to bake it into the message.
 */
#define assertff(expr, msg, ...) \
        assertf((expr), "%s(): " msg, __func__, ##__VA_ARGS__)

#define SFX_CHANNEL_MUSIC 0
#define SFX_CHANNEL_SOUND 2
#define SFX_MAX_CHANNELS  8

#define T3D_TOUNITS(x) (64.0f*(x))
#define T3D_FROMUNITS(x) ((x)*(1.0/64.0f))
#define T3D_TOUNITSCALE (64.0f)

#define NUM_BUFFERS (is_memory_expanded()? 3 : 3)

/// @brief rdpq_sprite_blit but with anchor support
/// @param sprite 
/// @param horizontal horizontal anchor
/// @param vertical  vertical anchor
/// @param x 
/// @param y 
/// @param parms 
void rdpq_sprite_blit_anchor(sprite_t* sprite, rdpq_align_t horizontal, rdpq_valign_t vertical, float x, float y, rdpq_blitparms_t* parms);

/// @brief rdpq_tex_blit but with anchor support
/// @param surface 
/// @param horizontal horizontal anchor
/// @param vertical  vertical anchor
/// @param x 
/// @param y 
/// @param parms 
void rdpq_tex_blit_anchor(const surface_t* surface, rdpq_align_t horizontal, rdpq_valign_t vertical, float x, float y, rdpq_blitparms_t* parms);

/// @brief Initialize the game's default display configuration
void engine_display_init_default();

/// @brief Returns whether a point is within the screen
/// @param x 
/// @param y 
/// @return true if it is
inline bool gfx_pos_within_viewport(float x, float y){
    return x > 0 && x < display_get_width() && y > 0 && y < display_get_height();
}

/// @brief Returns whether a point is within the rectangle
/// @param x 
/// @param y 
/// @return true if it is
inline bool gfx_pos_within_rect(float x, float y, float xa, float ya, float xb, float yb){
    return x > xa && x < xb && y > ya && y < yb;
}

/// @brief Generic linear interp function
/// @param a 
/// @param b 
/// @param t 
/// @return 
inline float lerp(float a, float b, float t)
{
    if(fabs(a - b) < FM_EPSILON) return b;
    return a + (b - a) * t;
}


inline float fclampr(float x, float min, float max){
    if (x > max) return max;
    if (x < min) return min;
    return x;
}

inline float fwrap(float x, float min, float max) {
    if (min > max) {
        return fwrap(x, max, min);
    }
    return (x >= 0 ? min : max) + fmod(x, max - min);
}


inline float frandr( float min, float max )
{
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}

inline int maxi(int a, int b) {
    return (a > b) ? a : b;
}

color_t get_rainbow_color(float s);

/// @brief Random int [0-max)
/// @param max 
/// @return 
int randm(int max);

/// @brief Random int [min-max)
/// @param max 
/// @return 
int randr(int min, int max);

// string to vec
fm_vec3_t string_to_vec(const char* input);

// string to quat
fm_vec4_t string_to_quat(const char* input);

extern fm_vec3_t vec_upz_to_upy(fm_vec3_t* in);

extern fm_vec3_t vec_upz_to_upy_scl(fm_vec3_t* in);

extern fm_vec3_t vec_upy_to_upz(fm_vec3_t* in);

extern fm_vec3_t vec_upy_to_upz_scl(fm_vec3_t* in);

extern void transform_upz_to_upy(fm_vec3_t* pos, fm_vec3_t* scale, fm_vec3_t* outpos, fm_vec3_t* outscale);

// aabb vs point collision with expanded bounds
extern bool collideAABB(
    fm_vec3_t* position,
    float expanddistance,
    fm_vec3_t* aabbposition,
    fm_vec3_t* aabbscale, // half-extents
    fm_vec3_t* out
);

void temporal_dither(int frameidx);

#ifdef __cplusplus
}

#endif



#endif
