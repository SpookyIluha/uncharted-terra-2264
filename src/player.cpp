#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "utils.h"
#include "camera.h"
#include "engine_gamestatus.h"
#include "level.h"
#include "player.h"

player_t player;
int frame = 0;
sprite_t* ui_play;
sprite_t* ui_play2;
sprite_t* ui_play3;

wav64_t walksound;
float   walksoundvolume;
wav64_t fldisablesound;
wav64_t flenablesound;
wav64_t fllockedsound;
surface_t prevbuffer = {0};



bool intersectInner(fm_vec3_t* in, float width, float height, fm_vec3_t* out){
    float dx = in->x;
    float px = (width/2.0f) - abs(dx);
    if (px <= 0) {
      return false;
    }

    float dy = in->z;
    float py = (height/2.0f) - abs(dy);
    if (py <= 0) {
      return false;
    }

    fm_vec3_t delta = {0};
    fm_vec3_t normal = {0};
    if (px < py) {
      float sx = copysignf(1.0, dx);
      delta.x = px * sx;
      normal.x = sx;
      out->x = ((width/2.0f) * sx);
      out->z = in->z;
    } else {
      float sy = copysignf(1.0, dy);
      delta.z = py * sy;
      normal.z = sy;
      out->x = in->x;
      out->z = ((height/2.0f) * sy);
    }
    return true;
  }

void player_init(){
    player.port = JOYPAD_PORT_1;

    player.maxspeed = (1.5f);
    player.speedaccel = (8.0f);
    player.charheight = 1.7f;
    player.charwidth = 0.5f;
    player.position = (fm_vec3_t){{0,0,0}};
    player.camera.aspect_ratio = (float)4 / (float)3;
    player.camera.FOV = 60;
    player.camera.near_plane = T3D_TOUNITS(0.45f);
    player.camera.far_plane = T3D_TOUNITS(6.0f);
    player.camera.rotation = (fm_vec3_t){{0, 0, 0}};

    /*ui_play = sprite_load("rom:/ui_play.ia8.sprite");
    ui_play2 = sprite_load("rom:/ui_play2.ia8.sprite");
    ui_play3 = sprite_load("rom:/ui_play3.ia8.sprite");

    wav64_open(&walksound, "rom:/sfx/walksounds.wav64");
    wav64_open(&fldisablesound, "rom:/sfx/flashlight_click1.wav64");
    wav64_open(&flenablesound, "rom:/sfx/flashlight_click2.wav64");
    wav64_open(&fllockedsound, "rom:/sfx/flashlight_locked.wav64");
    wav64_set_loop(&walksound, true);
    wav64_play(&walksound, SFX_CHANNEL_WALKSOUND);
    mixer_ch_set_vol(4, 0.0f, 0.0f);*/
}

/*void player_fldisable(){
    player.light.flashlightenabled = false;
    wav64_play(&fldisablesound, SFX_CHANNEL_FLASHLIGHT);
}

void player_flenable(){
    player.light.flashlightenabled = true;
    wav64_play(&flenablesound, SFX_CHANNEL_FLASHLIGHT);
}

void player_flswitch(){
    if(player.light.flashlightenabled) player_fldisable();
    else player_flenable();
}*/

void player_update(){
    joypad_inputs_t input = joypad_get_inputs(player.port);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(player.port);
    joypad_buttons_t held = joypad_get_buttons_held(player.port);
    fm_vec3_t forward = {0}, right = {0}, lookat = {0}, wishdir = {0};
    fm_vec3_t eulerright = player.camera.rotation; eulerright.y += T3D_DEG_TO_RAD(90);
    float maxspeed = player.maxspeed;
    if((held.l || held.r) && !held.z && (held.d_up || held.c_up)) maxspeed *= 2.0f;

    fm_vec3_dir_from_euler(&forward, &player.camera.rotation);
    fm_vec3_dir_from_euler(&right, &eulerright);
    forward.y = 0;
    right.y = 0;
    fm_vec3_norm(&forward, &forward);
    fm_vec3_norm(&right, &right);
    fm_vec3_scale(&forward, &forward, maxspeed);
    fm_vec3_scale(&right, &right, maxspeed);

    if(held.d_left || held.c_left)  fm_vec3_add(&wishdir, &wishdir, &right);
    if(held.d_right || held.c_right) fm_vec3_sub(&wishdir, &wishdir, &right);
    if(held.d_up || held.c_up)    fm_vec3_add(&wishdir, &wishdir, &forward);
    if(held.d_down || held.c_down)  fm_vec3_sub(&wishdir, &wishdir, &forward);
    float wishdirlen = fm_vec3_len(&wishdir);

    //walksoundvolume = lerp(walksoundvolume, wishdirlen > player.maxspeed / 5.0f? 1.0f : 0.0f, 0.1f);
    //mixer_ch_set_vol(SFX_CHANNEL_WALKSOUND, walksoundvolume, walksoundvolume);

    if(wishdirlen >= maxspeed - FM_EPSILON) wishdirlen = maxspeed / wishdirlen;
    fm_vec3_scale(&wishdir, &wishdir, wishdirlen);

    fm_vec3_t towardswishdir = {0};
    fm_vec3_sub(&towardswishdir, &wishdir, &player.velocity);
    float wishlen = fm_vec3_len(&towardswishdir);
    if(wishlen < player.speedaccel * DELTA_TIME * 2){
        player.velocity = wishdir;
    }
    else{
        fm_vec3_norm(&towardswishdir, &towardswishdir);
        fm_vec3_scale(&towardswishdir, &towardswishdir, player.speedaccel * DELTA_TIME);
        fm_vec3_add(&player.velocity, &towardswishdir, &player.velocity); 
    }

    fm_vec3_t velocitydelta = {0};
    fm_vec3_scale(&velocitydelta, &player.velocity, DELTA_TIME);
    fm_vec3_add(&player.position, &player.position, &velocitydelta);

    for(int i = 0; i < MAX_AABB_COLLISIONS; i++) {
        if(currentlevel.aabb_collisions[i].enabled) {
            fm_vec3_t resolvedpos = {0};
            if(collideAABB(&player.position, player.charwidth, &currentlevel.aabb_collisions[i].center, &currentlevel.aabb_collisions[i].half_extents, &resolvedpos)) {
                player.position = resolvedpos;
            }
        }
    }

    // outer collision
    /*if(player.position.x < T3D_TOUNITS(-5.3f)) player.position.x = T3D_TOUNITS(-5.3f);
    if(player.position.x > T3D_TOUNITS(5.3f)) player.position.x = T3D_TOUNITS(5.3f);
    if(player.position.z > T3D_TOUNITS(2.6f)) player.position.z = T3D_TOUNITS(2.6f);
    if(player.position.z < T3D_TOUNITS(-2.8f)) player.position.z = T3D_TOUNITS(-2.8f);

    // floor 0 collision
    if(player.floor == 0 && player.position.z > 0 && player.position.x > T3D_TOUNITS(-3.75f)) player.position.x = T3D_TOUNITS(-3.75f);
    
    // inner collision
    fm_vec3_t innerhit = {0};
    if(intersectInner(&player.position, T3D_TOUNITS(7.5f), T3D_TOUNITS(2.2f), &innerhit)) {
        player.position.x = innerhit.x;
        player.position.z = innerhit.z;
    }

    // handle stair slopes
    if(player.position.x > T3D_TOUNITS(-3.0f) && player.position.x < T3D_TOUNITS(3.0f)){
        float floorheight = 0;
        float floor = (float)player.floor;
        // upstairs
        if(player.position.z > 0){
            if(player.floor % 2 == 1) floor += 1.0f;
            float t = (player.position.x + T3D_TOUNITS(3.0f)) / T3D_TOUNITS(6.0f); //fmap(player.position.x, T3D_TOUNITS(3.0f),T3D_TOUNITS(-3.0f),0.0f,1.0f);
            float highfloor = -((float)floor - 1.0f) * T3D_TOUNITS(4.0f);
            float lowfloor =  -((float)floor) * T3D_TOUNITS(4.0f);
            floorheight = lerp(lowfloor, highfloor, t);
        } else { // downstairs 
            if(player.floor % 2 == 1) floor -= 1.0f;
            float t = (player.position.x + T3D_TOUNITS(3.0f)) / T3D_TOUNITS(6.0f);
            float highfloor = -((float)floor) * T3D_TOUNITS(4.0f);
            float lowfloor =  -((float)floor + 1.0f) * T3D_TOUNITS(4.0f);
            floorheight = lerp(highfloor, lowfloor, t);
        }

        player.position.y = floorheight;
    } // if not on stairs
    else{
        player.floor = floorf(T3D_FROMUNITS(-player.position.y / 4.0f) + 0.9f);
        player.position.y = -(player.floor) * T3D_TOUNITS(4.0f); 
    }*/

    player.camera.rotation.y   -= T3D_DEG_TO_RAD(1.0f * DELTA_TIME * input.stick_x);
    player.camera.rotation.x   += T3D_DEG_TO_RAD(0.7f * DELTA_TIME * input.stick_y);
    player.camera.rotation.x = fclampr(player.camera.rotation.x, T3D_DEG_TO_RAD(-70), T3D_DEG_TO_RAD(70));

    player.camera.rotation.x = t3d_lerp(player.camera.rotation.x, player.camera.rotation.x, 0.3f);
    player.camera.rotation.y = t3d_lerp(player.camera.rotation.y, player.camera.rotation.y, 0.3f);

    player.camera.position = (fm_vec3_t){{T3D_TOUNITS(player.position.x), T3D_TOUNITS((player.position.y + player.charheight)), T3D_TOUNITS(player.position.z)}};
    float velocitylen = fm_vec3_len2(&player.velocity);
    player.camera.position.y += sinf(CURRENT_TIME * 12.0f) * velocitylen * 0.5f;
    player.camera.rotation.x += cosf(CURRENT_TIME * 6.0f) * velocitylen * 0.0008f;

    if(held.z) player.heightdiff = lerp(player.heightdiff, (player.charheight / 3), 0.2f );
    else player.heightdiff = lerp(player.heightdiff, 0, 0.2f );
    player.camera.position.y -= T3D_TOUNITS(player.heightdiff);

    /*
    if(pressed.a) {
        if(!player.light.flashlightlocked){
            player_flswitch();
            if(player.light.onflashlightpress) 
                player.light.onflashlightpress();
        }
        else wav64_play(&fllockedsound, SFX_CHANNEL_EFFECTS2);
    }*/

    frame++;
}

void player_draw(T3DViewport *viewport){
    //int noise = (funstuff.noiselevel / 20) + 2;
    //viewport->offset[0] = rand() % noise;
    camera_transform(&player.camera, viewport);
    //t3d_light_set_point(0, (uint8_t*)&player.light.color, (T3DVec3*)&player.light.position, 0.5f, false);
}

void player_draw_ui(){
    /*int noise = funstuff.noiselevel / 5 + 1;
    int whitenoise = funstuff.noiselevel / 5 + 5;
    int offset = funstuff.noiselevel / 5;
    T3DObject* obj = t3d_model_get_object_by_index(player.light.flashlightmodel, 0);
    surface_t surf = sprite_get_pixels(player.light.flashlighttextures[player.light.flashlighttexindex]);
    T3DModelState state = t3d_model_state_create();
            state.drawConf = &(T3DModelDrawConf){
                .userData = NULL,
                .tileCb = NULL,
                .filterCb = NULL
            };
    t3d_matrix_push(player.light.modelMatFP[frame % 6]);
    //if(!player.light.modelblock) {
        //rspq_block_begin();
        rdpq_sync_pipe(); // Hardware crashes otherwise
        rdpq_sync_tile();
        t3d_model_draw_material(obj->material, &state);
        rdpq_mode_mipmap(MIPMAP_NONE, 0);
        if(player.light.flashlighttexindex == 1)
             rdpq_tex_upload(TILE0, &surf, &(rdpq_texparms_t){.s.mirror = true, .t.mirror = true, .s.repeats = 2, .t.repeats = 1, .s.translate = frandr(-10,10), .t.translate = 32});
        else rdpq_tex_upload(TILE0, &surf, &(rdpq_texparms_t){.s.mirror = true, .t.mirror = true, .s.repeats = 2, .t.repeats = 2});
        rdpq_mode_combiner(RDPQ_COMBINER2(
            (TEX0,0,PRIM,0), (TEX0,0,PRIM,0), 
            (NOISE,0,ENV,COMBINED), (0,0,0,COMBINED)
            ));
        rdpq_mode_zbuf(false,false);
        rdpq_mode_blender(RDPQ_BLENDER((MEMORY_RGB, IN_ALPHA, IN_RGB, ONE)));
        if(player.light.flashlightenabled && !player.light.flashlightstrobe) rdpq_set_prim_color(RGBA32(funstuff.noiselevel > 100? 75 : 15,15,15, 255));
        else  rdpq_set_prim_color(RGBA32(0,0,0, 0));
        rdpq_set_env_color(RGBA32(whitenoise,whitenoise,whitenoise,255));
        rdpq_mode_dithering(DITHER_NOISE_NOISE);
        t3d_model_draw_object(obj, NULL);
        t3d_matrix_pop(1);
        //player.light.modelblock = rspq_block_end();
    //} rspq_block_run(player.light.modelblock);
   rdpq_sync_pipe(); // Hardware crashes otherwise
    rdpq_set_mode_standard();
    rdpq_mode_alphacompare(30);
    rdpq_set_prim_color(RGBA32(70,70,70,255));
    rdpq_mode_combiner(RDPQ_COMBINER2((NOISE,0,TEX0,0), (TEX0,0,PRIM,0), (COMBINED,0,PRIM,0),(0,0,0,COMBINED)));
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_dithering(DITHER_NOISE_NOISE);
    int offsetrand = rand() % noise;
    rdpq_sync_pipe(); // Hardware crashes otherwise
    rdpq_sync_load();
    rdpq_sync_tile();
    if(frame % 64 > 32) rdpq_sprite_blit(ui_play, 30 + offsetrand,20, NULL);
    rdpq_sync_pipe(); // Hardware crashes otherwise
    rdpq_sprite_blit(ui_play2, display_get_width() - 90 + offsetrand, display_get_height() - 50 + offset, NULL);
    rdpq_sync_pipe(); // Hardware crashes otherwise
    rdpq_sprite_blit(ui_play3, 20 + offsetrand,20, NULL);

;
    //rdpq_text_printf(NULL, 1, 20, 20, "Indices: %i\nVerts: %i", stairs.stairsmodel->totalVertCount);
    //rdpq_text_printf(NULL, 1, 20, 20, "floor: %i\n pos: %.2f, %.2f, %.2f", player.floor, T3D_FROMUNITS(player.position.x), T3D_FROMUNITS(player.position.y), T3D_FROMUNITS(player.position.z));
    if(prevbuffer.buffer && player.look.motionblur > 0){
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY_CONST);
        rdpq_set_fog_color(RGBA32(0, 0, 0, player.look.motionblur * 255.0f));
        rdpq_mode_zbuf(false,false); 
        rdpq_tex_blit(&prevbuffer, 0,0, NULL);
    }
    prevbuffer = display_get_current_framebuffer();*/
    rdpq_sync_pipe();
    rdpq_text_printf(NULL, 1, 20, 20, "Pos: %.2f, %.2f, %.2f", (player.position.x), (player.position.y), (player.position.z));
    rdpq_text_printf(NULL, 1, 20, 40, "FPS: %.2f", display_get_fps());
    heap_stats_t stats; sys_get_heap_stats(&stats);
    rdpq_text_printf(NULL, 1, 20, 60, "MEM: %i total, %i used", stats.total, stats.used);
}
