#ifndef EFFECTS_H
#define EFFECTS_H

#include <libdragon.h>
#include <time.h>
#include <display.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

#ifdef __cplusplus
extern "C"{
#endif

#define MAX_EFFECTS 4
#define MAXPLAYERS 4

typedef struct effectdata_s{
    float rumbletime[MAXPLAYERS];
    bool  rumblestate[MAXPLAYERS];
} effectdata_t;

extern effectdata_t effects;

void effects_init();
void effects_update();
void effects_draw();
void effects_close();

void effects_add_rumble(joypad_port_t port, float time);

void effects_rumble_stop();


#ifdef __cplusplus
}
#endif

#endif