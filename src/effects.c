#include <libdragon.h>
#include <time.h>
#include <display.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "engine_gamestatus.h"
#include "audioutils.h"
#include "utils.h"
#include "effects.h"

effectdata_t effects;

void effects_init(){
    for(int i = 0; i < MAXPLAYERS; i++){
        effects.rumbletime[i] = 0;
        effects.rumblestate[i] = false;
    }
}
void effects_update(){

    for(int i = 0; i < MAXPLAYERS; i++){
        if(effects.rumbletime[i] > 0){
            effects.rumbletime[i] -= display_get_delta_time();
            if(!effects.rumblestate[i]) {joypad_set_rumble_active(i, true); effects.rumblestate[i] = true;}
        } else if(effects.rumblestate[i]) {joypad_set_rumble_active(i, false); effects.rumblestate[i] = false;}
    }

}

void effects_rumble_stop(){
    for(int i = 0; i < MAXPLAYERS; i++){
        joypad_set_rumble_active(i, false);
        effects.rumblestate[i] = false;
    }
}

void effects_add_rumble(joypad_port_t port, float time){
    if(port < 0) return;
    //if(!gamestatus.state.game.settings.vibration) return;
    effects.rumbletime[port] += time;
}



void effects_draw(){

}

void effects_close(){

    for(int i = 0; i < MAXPLAYERS; i++)
        joypad_set_rumble_active(i, false);

}