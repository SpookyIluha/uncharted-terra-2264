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

    struct{
        joypad_inputs_t input;
        joypad_buttons_t pressed;
        joypad_buttons_t held;
        joypad_port_t port;
    } joypad;
    camera_t  camera;

} player_t;

extern player_t player;

void player_inventory_additem(uint8_t item_id, uint8_t item_count);
void player_inventory_removeitem_by_item_id(uint8_t item_id, uint8_t item_count);
void player_inventory_removeitem_by_slot(uint8_t slot, uint8_t item_count);
void player_inventory_clear();
int player_inventory_count_occupied_item_slots();
uint8_t player_inventory_get_ith_occupied_item_slot(int i);

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