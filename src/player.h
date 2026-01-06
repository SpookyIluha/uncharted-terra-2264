#ifndef PLAYER_H
#define PLAYER_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "camera.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    fm_vec3_t position;
    fm_vec3_t velocity;
    float maxspeed, speedaccel;
    float     charheight;
    float     charwidth;
    float     heightdiff;

    joypad_port_t port;
    camera_t  camera;

} player_t;

extern player_t player;

void player_init();
void player_update();
void player_draw(T3DViewport *viewport);
void player_draw_ui();

void player_fldisable();
void player_flenable();
void player_flswitch();

#ifdef __cplusplus
}
#endif


#endif