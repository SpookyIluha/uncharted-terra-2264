#include <libdragon.h>
#include <time.h>
#include <unistd.h>
#include <display.h>
#include "utils.h"
#include "engine_gamestatus.h"
#include <fmath.h>

color_t get_rainbow_color(float s) {
  float r = fm_sinf(s + 0.0f) * 127.0f + 128.0f;
  float g = fm_sinf(s + 2.0f) * 127.0f + 128.0f;
  float b = fm_sinf(s + 4.0f) * 127.0f + 128.0f;
  return RGBA32(r, g, b, 255);
}

int randm(int max){
  return (rand() % max);
}

int randr(int min, int max){
  int range = min - max;
  return (rand() % range) + min;
}

void rdpq_sprite_blit_anchor(sprite_t* sprite, rdpq_align_t horizontal, rdpq_valign_t vertical, float x, float y, rdpq_blitparms_t* parms){
    assert(sprite);
    int width = sprite->width;
    int height = sprite->height;
    switch(horizontal){
        case ALIGN_RIGHT:
            x -= width; break;
        case ALIGN_CENTER:
            x -= width / 2; break;
        default: break;
    }
    switch(vertical){
        case VALIGN_BOTTOM:
            y -= height; break;
        case VALIGN_CENTER:
            y -= height / 2; break;
        default: break;
    }
    rdpq_sprite_blit(sprite, x,y,parms);
}

void rdpq_tex_blit_anchor(const surface_t* surface, rdpq_align_t horizontal, rdpq_valign_t vertical, float x, float y, rdpq_blitparms_t* parms){
    assert(surface);
    int width = surface->width;
    int height = surface->height;
    switch(horizontal){
        case ALIGN_RIGHT:
            x -= width; break;
        case ALIGN_CENTER:
            x -= width / 2; break;
        default: break;
    }
    switch(vertical){
        case VALIGN_BOTTOM:
            y -= height; break;
        case VALIGN_CENTER:
            y -= height / 2; break;
        default: break;
    }
    rdpq_tex_blit(surface, x,y,parms);
}

void engine_display_init_default(){
    resolution_t res;
    res.width = 640;
    res.height = is_memory_expanded()? 480 : 360;
    res.interlaced = gamestatus.fastgraphics? INTERLACE_RDP : INTERLACE_HALF;
    res.aspect_ratio = (float)res.width / (float)res.height;
    display_init(res,
        DEPTH_16_BPP,
        NUM_BUFFERS, GAMMA_NONE,
        FILTERS_RESAMPLE_ANTIALIAS_DEDITHER
    );
    if(get_tv_type() == TV_PAL && gamestatus.fastgraphics) {
        int offset = is_memory_expanded()? 0 : 60;
        vi_set_borders((vi_borders_t){.up = 48 + offset, .down = 48 + offset});
        vi_set_yscale_factor(2.0f);
    }
}


// string -> vec3
fm_vec3_t string_to_vec(const char* input)
{
    const char* str = input;
    char* end;

    fm_vec3_t result;
    result.x = strtof(str, &end);
    result.y = strtof(end, &end);
    result.z = strtof(end, &end);

    return result;
}

// string -> quat (vec4)
fm_vec4_t string_to_quat(const char* input)
{
    const char* str = input;
    char* end;

    fm_vec4_t result;
    result.w = strtof(str, &end);   
    result.x = strtof(end, &end);
    result.y = strtof(end, &end);
    result.z = strtof(end, &end);

    return result;
}

fm_vec3_t vec_upz_to_upy(fm_vec3_t* in)
{
    fm_vec3_t out;
    out.x = in->x;
    out.y = in->z;
    out.z = -in->y;
    return out;
}

fm_vec3_t vec_upz_to_upy_scl(fm_vec3_t* in)
{
    fm_vec3_t out;
    out.x = in->x;
    out.y = in->z;
    out.z = in->y;
    return out;
}

fm_vec3_t vec_upy_to_upz(fm_vec3_t* in)
{
    fm_vec3_t out;
    out.x = in->x;
    out.y = -in->z;
    out.z = in->y;
    return out;
}

fm_vec3_t vec_upy_to_upz_scl(fm_vec3_t* in)
{
    fm_vec3_t out;
    out.x = in->x;
    out.y = in->z;
    out.z = in->y;
    return out;
}


void transform_upz_to_upy(fm_vec3_t* pos, fm_vec3_t* scale, fm_vec3_t* outpos, fm_vec3_t* outscale){
    *outpos = vec_upz_to_upy(pos);
    *outscale = vec_upz_to_upy_scl(scale);
}


// aabb vs point collision with expanded bounds
bool collideAABB(
    fm_vec3_t* position,
    float expanddistance,
    fm_vec3_t* aabbposition,
    fm_vec3_t* aabbscale, // half-extents
    fm_vec3_t* out
)
{
    // Expanded half-extents
    float hx = aabbscale->x + expanddistance;
    float hy = aabbscale->y + expanddistance;
    float hz = aabbscale->z + expanddistance;

    // Vector from AABB center to point
    float dx = position->x - aabbposition->x;
    float dy = position->y - aabbposition->y;
    float dz = position->z - aabbposition->z;

    // Penetration depths
    float px = hx - fabsf(dx);
    float py = hy - fabsf(dy);
    float pz = hz - fabsf(dz);

    if(out)
        *out = *position;

    // No collision if point lies outside on any axis
    if (px <= 0.0f || py <= 0.0f || pz <= 0.0f)
    {
        return false;
    }

    // Collision occurred - resolve along minimum penetration axis
    if(out){
        if (px < py && px < pz)
        {
            out->x += (dx < 0.0f) ? -px : px;
        }
        else if (py < pz)
        {
            out->y += (dy < 0.0f) ? -py : py;
        }
        else
        {
            out->z += (dz < 0.0f) ? -pz : pz;
        }
    }

    return true;
}

void temporal_dither(int frameidx){
    // temporal dithering to restore as much as possible of True Color
    switch(frameidx % 4){
        case 0:
            rdpq_mode_dithering(DITHER_SQUARE_INVSQUARE);
            break;
        case 1:
            rdpq_mode_dithering(DITHER_NOISE_NOISE);
            break;
        case 2:
            rdpq_mode_dithering(DITHER_NOISE_NOISE);
            break;
        case 3:
            rdpq_mode_dithering(DITHER_BAYER_BAYER);
            break;
    }
}
