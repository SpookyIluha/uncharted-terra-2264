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
 #include "intro.h"
 
 bool cont = false;
 
 
 camera_t maincamera;
 T3DMWModel model;
 
 T3DViewport viewport;
 
 void new_game();
 
 void game_start(){
     engine_display_init_default();
 
     while(true){
         game_menu();
         if(!cont)
             new_game();
         else
             playtimelogic();
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
    effects_rumble_stop();
    sprite_t* bg = sprite_load(filesystem_getfn(DIR_IMAGE, "menu/storytime").c_str() );
    surface_t bgsurf = sprite_get_pixels(bg);
    bool pressed_start = false;
    float time = 0;

    std::ifstream t(filesystem_getfn(DIR_SCRIPT_LANG, "storytime.txt").c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();

    rdpq_textparms_t parms; 
    parms.align = ALIGN_LEFT; 
    parms.width = display_get_width() - 80; 
    parms.style_id = gamestatus.fonts.titlefontstyle; 
    parms.wrap = gamestatus.fonts.wrappingmode;
    
    int nbytes = strlen(buffer.str().c_str());
    rdpq_paragraph_t* paragraph = rdpq_paragraph_build(&parms, gamestatus.fonts.titlefont, buffer.str().c_str(), &nbytes);
    float text_height = paragraph->bbox.y1 - paragraph->bbox.y0;
    
    float rate = 10.0f;
    float fade_time = 5.0f;
    float duration = (text_height + 260.0f) / rate;

    sound_play("ambience_intro", false);
    bgm_stop(2.0f);

    while(!pressed_start && time < duration){
        timesys_update();
        joypad_poll();
        time += display_get_delta_time();
        audioutils_mixer_update();

        float alpha = 1.0f;
        if(time < 1.0f) alpha = time;
        float fade_start = duration - fade_time;
        if(time > fade_start) alpha = 1.0f - (time - fade_start) / fade_time;
        if(alpha > 1.0f) alpha = 1.0f;
        if(alpha < 0.0f) alpha = 0.0f;

        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        pressed.raw |= joypad_get_buttons_pressed(JOYPAD_PORT_2).raw;
        if(pressed.start || pressed.a) pressed_start = true;


        rdpq_attach(display_get(), NULL);
        if(display_interlace_rdp_field() >= 0) 
            rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();
        rdpq_clear(RGBA32(0,0,0,0));
        int scissor_y = display_get_height() / 2;
        rdpq_set_scissor(0,scissor_y - 120,640, scissor_y + 120);

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
        rdpq_paragraph_render(paragraph, 40, (int)(scissor_y + 140 - time * rate));

        rdpq_sync_tile();
        rdpq_detach_show();
    }
    rspq_wait();
    sprite_free(bg);
    rdpq_paragraph_free(paragraph);
    sound_stop();
    display_close();
    {
        std::string moviefn = filesystem_getfn(DIR_MOVIES, "newgame");
        movie_play(const_cast<char*>(moviefn.c_str()), "newgame", 15);
    }
    sound_stop();
    bgm_stop(0);
    audioutils_mixer_update();
    rspq_wait();
    wait_ms(2000);
    engine_display_init_default();
    rspq_wait();
    memset(&gamestatus.state.game, 0, sizeof(gamestatus.state.game));
    playtimelogic();
}
 
 void game_draw(){
   uint8_t colorAmbient[4] = {255, 255, 255, 0xFF};
   float rotAngle = 0.0f;
   int frameIdx = 0;
 
 
   {
     // ======== Update ======== //
     // cycle through FP matrices to avoid overwriting what the RSP may still need to load
     frameIdx = (frameIdx + 1) % 6;
 
     float modelScale = 1.0f;
 
     t3d_vec3_lerp(&maincamera.camTarget_off, &maincamera.camTarget_off, &maincamera.camTarget, 0.1f);
     t3d_vec3_lerp(&maincamera.camPos_off, &maincamera.camPos_off, &maincamera.camPos, 0.1f);
     t3d_viewport_set_projection(&viewport, T3D_DEG_TO_RAD(85.0f), 16.0f, 2300.0f);  
     T3DVec3 camtarg_shake = (T3DVec3){0, 0, 0};
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
 
     rdpq_set_fog_color(RGBA32(109,94,90,255));
     temporal_dither(FRAME_NUMBER);
     model.draw();
 
   }
 }
 
 void game_menu(){
     effects_rumble_stop();
     sound_stop();
     bgm_stop(0);
     bgm_play("dead_windmills", true, 0);
     sprite_t* bg = sprite_load(filesystem_getfn(DIR_IMAGE, "menu/gamelogo.ia8").c_str() );
     surface_t bgsurf = sprite_get_pixels(bg);
     sprite_t* button_a = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/button_a.rgba32").c_str());
 
     viewport = t3d_viewport_create_buffered(6);
     model = T3DMWModel();
     model.load(filesystem_getfn(DIR_MODEL, gamecomplete? "mainmenu/model2" : "mainmenu/model").c_str());
 
     t3d_light_set_exposure(1);
     cont = false;
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
         audioutils_mixer_update();
         camlocationtimer += display_get_delta_time() / 12;
         if(camlocationtimer >= 1) {camidx++; camlocationtimer = 0;}
 
         joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
         pressed.raw |= joypad_get_buttons_pressed(JOYPAD_PORT_2).raw;
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
         rdpq_text_printf(&parms, gamestatus.fonts.titlefont, 0, display_get_height() - 45, dictstr("press_start"));
         rdpq_sync_tile();
         rdpq_detach_show();
     }
     sound_play("select3", false);
 
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
         float height = display_get_height();
         {
             posoffset = lerp(posoffset, 0, 0.1f);
             int posoffseti = (int)posoffset;
                 rdpq_sprite_blit(button_a, 15 + selection*150, height - 60 + posoffseti, NULL);
             if(gamestatus.state_persistent.lastsavetype != SAVE_NONE){
                 parmstext.style_id = gamestatus.fonts.titlefontstyle;
                 rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 50, height - 40 + posoffseti, dictstr("MM_continue"));
             } else
             {
                 parmstext.style_id = gamestatus.fonts.unavailablefontstyle;
                 rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 50, height - 40 + posoffseti, dictstr("MM_continue"));
             }
             parmstext.style_id = gamestatus.fonts.titlefontstyle;
             rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 200, height - 40 + posoffseti, dictstr("MM_newgame"));
             rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 350, height - 40 + posoffseti, music_volume_get() > 0? dictstr("MM_music_on") : dictstr("MM_music_off"));
             rdpq_text_printf(&parmstext, gamestatus.fonts.titlefont, 500, height - 40 + posoffseti, sound_volume_get() > 0? dictstr("MM_sounds_on") :dictstr ("MM_sounds_off"));
             joypad_poll();
 
             if(gamestatus.state.scripts.debug){
                 rdpq_text_printf(NULL, 1, 20, 40, "FPS: %.2f", display_get_fps());
                 heap_stats_t stats; sys_get_heap_stats(&stats);
                 rdpq_text_printf(NULL, 1, 20, 60, "MEM: %i total, %i used", stats.total, stats.used);
             }
 
             joypad_buttons_t btn  = joypad_get_buttons_pressed(JOYPAD_PORT_1);
             btn.raw |= joypad_get_buttons_pressed(JOYPAD_PORT_2).raw;
             int axis_stick_x = joypad_get_axis_pressed(JOYPAD_PORT_1, JOYPAD_AXIS_STICK_X);
 
             if(btn.d_left || axis_stick_x < 0) {selection--; sound_play("select3", false);}
             if(btn.d_right || axis_stick_x > 0) {selection++; sound_play("select3", false);}
             if(selection < 0) selection = 3;
             if(selection > 3) selection = 0;
             if(gamestatus.state_persistent.lastsavetype == SAVE_NONE && selection == 0) selection = 3;
             if(btn.a) 
                 switch(selection){
                     case 0:{
                        bgm_stop(0); 
                        if(playtimelogic_loadgame()){
                            sound_play("select2", false); gamestart = true; cont = true;
                        } else {
                            // If eeprom is disabled or load failed, show message
                            // However, in main menu we don't have a subtitle system active like in player.cpp
                            // But the user specifically asked for dictstr("MM_savenfound")
                            // We can just stay in the menu for now as playtimelogic_loadgame will return false
                            sound_play("screw", false); 
                        }
                    } break;
                     case 1:{
                         sound_play("select2", false); 
                         gamestart = true;
                     } break;
                     case 2:{
                         {music_volume(1 - music_volume_get()); sound_play("select2", false);}
                     } break;
                     case 3:{
                         {sound_volume(1 - sound_volume_get()); sound_play("select2", false);}
                     } break;
                 }
         } 
         rdpq_detach_show();
         time += display_get_delta_time(); frame++;
         audioutils_mixer_update();
     }
 
     rspq_wait();
     t3d_viewport_destroy(&viewport);
     sprite_free(bg);
     sprite_free(button_a);
     model.free();
 }
