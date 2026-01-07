#include <libdragon.h>
#include <string>
#include <fstream>
#include <sstream>
#include "engine_filesystem.h"
#include "engine_t3dm_wrapper.h"
#include "audioutils.h"
#include "utils.h"
#include "camera.h"
#include "effects.h"
#include "playtimelogic.h"
#include "mainmenu.h"

#define NUM_BUFFERS (is_memory_expanded()? 3 : 3)

bool cont = false;

camera_t maincamera;
T3DMWModel model;

T3DViewport viewport;

void new_game();

void game_start(){
    resolution_t res;
        res.width = 640;
        res.height = 480;
        res.interlaced = INTERLACE_RDP;
        res.aspect_ratio = (float)640 / 480;

    display_init(res,
		DEPTH_16_BPP,
		3, GAMMA_NONE,
		FILTERS_RESAMPLE_ANTIALIAS_DEDITHER
	);

    while(true){
        if(!cont){
            game_menu();
            if(!cont)
                new_game();
        }
    }
}

float camlocations[5][4][3] = {
  {{7,5,24},{-7,5,24},{7,5,-24},{-7,5,-24}},
  {{16,8,15},{-16,8,15},{20,4,5},{-20,4,5}},
  {{18,12,15},{18,5,15},{6,4,5},{6,4,5}},
  {{0,5,17},{0,5,17},{6,4,5},{-6,4,5}},
  {{0,5,24},{0,5,12},{0,0,0},{0,0,0}}
};


void new_game(){
    sprite_t* bg = sprite_load(filesystem_getfn(DIR_IMAGE, "menu/storytime").c_str() );
    surface_t bgsurf = sprite_get_pixels(bg);
    bool pressed_start = false;
    float time = 0;

    std::ifstream t(filesystem_getfn(DIR_SCRIPT_LANG, "storytime.txt").c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();

    while(!pressed_start && time < 70.0f){
        timesys_update();
        joypad_poll();
        time += display_get_delta_time();

        float alpha = 1.0f;
        if(time < 1.0f) alpha = time;
        if(time > 60.0f) alpha = 1.0f - (time - 60.0f) / 5.0f;
        if(alpha > 1.0f) alpha = 1.0f;
        if(alpha < 0.0f) alpha = 0.0f;

        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if(pressed.start || pressed.a) pressed_start = true;


        rdpq_attach(display_get(), NULL);
        rdpq_clear(RGBA32(0,0,0,0));
        rdpq_disable_interlaced();
        rdpq_set_scissor(0,240 - 120,640, 240 + 120);
        rdpq_mode_zbuf(false,false);
        rdpq_sync_pipe();
        rdpq_sync_tile();

        rdpq_sync_pipe();
        rdpq_sync_tile();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
        rdpq_mode_dithering(DITHER_SQUARE_INVSQUARE);
        rdpq_set_prim_color(RGBA32(alpha*255,alpha*255,alpha*255,alpha*255));
        rdpq_blitparms_t blitparms;
        blitparms.cx = bgsurf.width / 2;
        blitparms.cy = bgsurf.height / 2;
        rdpq_sprite_blit(bg, display_get_width() / 2, display_get_height() / 2, &blitparms);

        rdpq_mode_filter(FILTER_BILINEAR);
        rdpq_textparms_t parms; parms.align = ALIGN_LEFT; parms.width = display_get_width() - 80; parms.style_id = gamestatus.fonts.titlefontstyle; parms.wrap = WRAP_WORD;
        rdpq_text_printf(&parms, gamestatus.fonts.titlefont, 40, (int)(400 - time * 10), buffer.str().c_str());

        rdpq_sync_tile();
        rdpq_detach_show();
    }
    rspq_wait();
    sprite_free(bg);

    playtimelogic();
}

void game_draw(){

  // Also create a buffered viewport to have a distinct matrix for each frame, avoiding corruptions if the CPU is too fast
  // In an actual game make sure to free this viewport via 't3d_viewport_destroy' if no longer needed.


  const T3DVec3 camPos = {{0,10.0f,40.0f}};
  const T3DVec3 camTarget = {{0,0,0}};
  const T3DVec3 camUp = {{0,1,0}};

  uint8_t colorAmbient[4] = {255, 255, 255, 0xFF};
  uint8_t colorDir[4]     = {0xEE, 0xAA, 0xAA, 0xFF};

  T3DVec3 lightDirVec = {{-1.0f, 1.0f, 1.0f}};
  //t3d_vec3_norm(&lightDirVec);

  float rotAngle = 0.0f;
  int frameIdx = 0;


  {
    // ======== Update ======== //
    // cycle through FP matrices to avoid overwriting what the RSP may still need to load
    frameIdx = (frameIdx + 1) % 3;

    float modelScale = 1.0f;

    t3d_vec3_lerp(&maincamera.camTarget_off, &maincamera.camTarget_off, &maincamera.camTarget, 0.1f);
    t3d_vec3_lerp(&maincamera.camPos_off, &maincamera.camPos_off, &maincamera.camPos, 0.1f);
    t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(85.0f), 16.0f, 2300.0f);  
    T3DVec3 camtarg_shake = (T3DVec3){.x = frandr(-1,1) * effects.screenshaketime, .y = frandr(-1,1) * effects.screenshaketime, .z = frandr(-1,1) * effects.screenshaketime};
    t3d_vec3_add(&camtarg_shake, &maincamera.camTarget_off, &camtarg_shake);
    T3DVec3 upvec; upvec.v[0] = 0; upvec.v[1] = 1; upvec.v[2] = 0;
    t3d_viewport_look_at(&viewport, &maincamera.camPos_off, &camtarg_shake, &upvec);

    // slowly rotate model, for more information on matrices and how to draw objects
    // see the example: "03_objects"
    t3d_mat4fp_from_srt_euler(model.get_transform_fp(),
      (float[3]){modelScale, modelScale, modelScale},
      (float[3]){0.0f, rotAngle*0.2f, rotAngle},
      (float[3]){0,0,0}
    );

    // ======== Draw ======== //
    t3d_frame_start();
    t3d_viewport_attach(&viewport);
    if(display_interlace_rdp_field() >= 0) 
        rdpq_enable_interlaced(display_interlace_rdp_field());
    else rdpq_disable_interlaced();

    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_count(0);

    rdpq_set_fog_color(RGBA32(255,255,255,255));
    temporal_dither(FRAME_NUMBER);
    model.draw();

  }
}

void game_menu(){
    sprite_t* bg = sprite_load(filesystem_getfn(DIR_IMAGE, "menu/gamelogo.ia8").c_str() );
    surface_t bgsurf = sprite_get_pixels(bg);
    sprite_t* button_a = sprite_load(filesystem_getfn(DIR_IMAGE, "menu/button_a.rgba32").c_str());

    viewport = t3d_viewport_create_buffered(3);
    model = T3DMWModel();
    model.load(filesystem_getfn(DIR_MODEL, "mainmenu/model").c_str());

    viewport = t3d_viewport_create_buffered(3);

    rdpq_textparms_s parmstext; parmstext.style_id = gamestatus.fonts.titlefontstyle; parmstext.disable_aa_fix = true;

    float time = 0;
    int frame = 0;
    bool gamestart = false;
    float posoffset = 450;
    int selection = 0;
    if(gamestatus.state_persistent.lastsavetype == SAVE_NONE) selection = 1;

    bool pressed_start = false;
    float camlocationtimer = 0;
    int camidx = 0;
    while(!pressed_start){
        timesys_update();
        joypad_poll();
        camlocationtimer += display_get_delta_time() / 12;
        if(camlocationtimer >= 1) {camidx++; camlocationtimer = 0;}

        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if(pressed.start || pressed.a) pressed_start = true;

        camidx = camidx % 5;
        T3DVec3 campos; t3d_vec3_lerp(&campos, (T3DVec3*)&camlocations[camidx][0], (T3DVec3*)&camlocations[camidx][1], camlocationtimer);
        T3DVec3 camrot; t3d_vec3_lerp(&camrot, (T3DVec3*)&camlocations[camidx][2], (T3DVec3*)&camlocations[camidx][3], camlocationtimer);

        t3d_vec3_scale(&campos, &campos, 8);
        t3d_vec3_scale(&camrot, &camrot, 8);

        maincamera.camPos = campos;
        maincamera.camTarget = camrot;

        maincamera.camPos_off = campos;
        maincamera.camTarget_off = camrot;

        rdpq_attach(display_get(), NULL);
        if(display_interlace_rdp_field() >= 0) 
            rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();

        rdpq_mode_zbuf(false,false);
        rdpq_sync_pipe();
        rdpq_sync_tile();

        game_draw();

        rdpq_sync_pipe();
        rdpq_sync_tile();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_blitparms_t blitparms;
        blitparms.cx = bgsurf.width / 2;
        blitparms.cy = bgsurf.height / 2;
        rdpq_sprite_blit(bg, display_get_width() / 2, display_get_height() / 4, &blitparms);
        
        rdpq_textparms_t parms; parms.align = ALIGN_CENTER; parms.width = display_get_width(); parms.style_id = gamestatus.fonts.titlefontstyle;
        rdpq_text_printf(&parms, gamestatus.fonts.titlefont, 0, display_get_height() - 45, "Press Start");
        rdpq_sync_tile();
        rdpq_detach_show();
    }

    while(!gamestart){
        timesys_update();
        joypad_poll();
        camlocationtimer += display_get_delta_time() / 12;
        if(camlocationtimer >= 1) {camidx++; camlocationtimer = 0;}

        camidx = camidx % 5;
        T3DVec3 campos; t3d_vec3_lerp(&campos, (T3DVec3*)&camlocations[camidx][0], (T3DVec3*)&camlocations[camidx][1], camlocationtimer);
        T3DVec3 camrot; t3d_vec3_lerp(&camrot, (T3DVec3*)&camlocations[camidx][2], (T3DVec3*)&camlocations[camidx][3], camlocationtimer);

        t3d_vec3_scale(&campos, &campos, 8);
        t3d_vec3_scale(&camrot, &camrot, 8);

        maincamera.camPos = campos;
        maincamera.camTarget = camrot;

        maincamera.camPos_off = campos;
        maincamera.camTarget_off = camrot;

        rdpq_attach(display_get(), NULL);
        if(display_interlace_rdp_field() >= 0) 
            rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();

        rdpq_mode_zbuf(false,false);
        rdpq_sync_pipe();
        rdpq_sync_tile();

        game_draw();

        rdpq_sync_pipe();
        rdpq_sync_tile();
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_blitparms_t blitparms;
        blitparms.cx = bgsurf.width / 2;
        blitparms.cy = bgsurf.height / 2;
        rdpq_sprite_blit(bg, display_get_width() / 2, display_get_height() / 4, &blitparms);
        
        {
            posoffset = lerp(posoffset, 0, 0.1f);
                rdpq_sprite_blit(button_a, 15 + selection*150, 420 + posoffset, NULL);
            if(gamestatus.state_persistent.lastsavetype != SAVE_NONE){
                parmstext.style_id = gamestatus.fonts.titlefontstyle;
                rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 50, 440 + posoffset, dictstr("MM_continue"));
            } else
            {
                parmstext.style_id = 2;
                rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 50, 440 + posoffset, dictstr("MM_continue"));
            }
            parmstext.style_id = gamestatus.fonts.titlefontstyle;
            rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 200, 440 + posoffset, dictstr("MM_newgame"));
            rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 350, 440 + posoffset, music_volume_get() > 0? dictstr("MM_music_on") : dictstr("MM_music_off"));
            rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 500, 440 + posoffset, sound_volume_get() > 0? dictstr("MM_sounds_on") :dictstr ("MM_sounds_off"));
            joypad_poll();

            joypad_buttons_t btn  = joypad_get_buttons_pressed(JOYPAD_PORT_1);
            int axis_stick_x = joypad_get_axis_pressed(JOYPAD_PORT_1, JOYPAD_AXIS_STICK_X);

            if(btn.d_left || axis_stick_x < 0) {selection--; sound_play("menu/select", false);}
            if(btn.d_right || axis_stick_x > 0) {selection++; sound_play("menu/select", false);}
            if(selection < 0) selection = 3;
            if(selection > 3) selection = 0;
            if(gamestatus.state_persistent.lastsavetype == SAVE_NONE && selection == 0) selection = 3;
            if(btn.a) 
                switch(selection){
                    case 0:{
                        bgm_stop(0); 
                        engine_eeprom_load_manual();
                        sound_play("menu/select", false); gamestart = true; cont = true;
                    } break;
                    case 1:{
                        sound_play("menu/select", false); 
                        if(gamestatus.state_persistent.global_game_state == 1) {
                            bgm_stop(0); 
                            sound_stop();
                            timesys_init();
                            cont = true;
                        }
                        else{
                            //overlays_set_player_name(dictstr("MM_entername")); 
                            //overlays_message(dictstr("MM_tutorial")); 
                            cont = false;
                        }
                        gamestart = true;
                    } break;
                    case 2:{
                        {music_volume(1 - music_volume_get()); sound_play("menu/select", false);}
                    } break;
                    case 3:{
                        {sound_volume(1 - sound_volume_get()); sound_play("menu/select", false);}
                    } break;
                }
        } 
        rdpq_detach_show();
        time += display_get_delta_time(); frame++;
        audioutils_mixer_update();
    }

    rspq_wait();

    sprite_free(bg);
    sprite_free(button_a);
    model.free();
}