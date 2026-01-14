#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "utils.h"
#include "camera.h"
#include "engine_gamestatus.h"
#include "level.h"
#include "item_instance.h"
#include "subtitles.h"
#include "playtimelogic.h"
#include "journal_entry.h"
#include "utils.h"
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


void player_save_to_eeprom(){
    memcpy(&gamestatus.state.game.playerpos, &player.position, sizeof(fm_vec3_t));
    memcpy(&gamestatus.state.game.playerrot, &player.camera.rotation, sizeof(fm_vec3_t));
}

void player_load_from_eeprom(){
    memcpy(&player.position, &gamestatus.state.game.playerpos, sizeof(fm_vec3_t));
    memcpy(&player.camera.rotation, &gamestatus.state.game.playerrot, sizeof(fm_vec3_t));
    fm_vec3_dir_from_euler(&player.camera.camTarget_off, &player.camera.rotation);
}

void player_init(){
    player.joypad.port = JOYPAD_PORT_1;

    player.maxspeed = (1.5f);
    player.speedaccel = (8.0f);
    player.charheight = 1.7f;
    player.charwidth = 0.5f;
    player.position = (fm_vec3_t){{0,0,0}};
    player.camera.aspect_ratio = (float)display_get_width() / (float)display_get_height();
    player.camera.FOV = 60;
    player.camera.near_plane = T3D_TOUNITS(0.45f);
    player.camera.far_plane = T3D_TOUNITS(6.0f);
    player.camera.rotation = (fm_vec3_t){{0, 0, 0}};

    player_load_from_eeprom();

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

Entity* player_potential_user = NULL;

void player_use_item(uint8_t item_slot){
    if(!player_potential_user) {
        subtitles_add(dictstr("PAUSE_nothing_to_use_on"), 4.0f, '\0');
        return;
    }

    debugf("INFO: tried to use item with item_id %d\n", gamestatus.state.game.inventory[item_slot].item_id);

    if(gamestatus.state.game.inventory[item_slot].item_id == 0){
        return;
    }
    const std::string item_name = items_names[gamestatus.state.game.inventory[item_slot].item_id];
    std::string command = "entity " + player_potential_user->get_name() + " use_item " + item_name;

    if(!execute_console_command("entity " + player_potential_user->get_name() + " use_item " + item_name)){
        subtitles_add("PAUSE_cant_use_item_on_this", 4.0f, '\0');
    }
}

int player_check_has_item(const std::string& item_name){
    for(int i = 0; i < MAX_INVENTORY_SLOTS; i++){
        if(gamestatus.state.game.inventory[i].item_id == 0){
            continue;
        }
        if(items_names[gamestatus.state.game.inventory[i].item_id] == item_name){
            return i;
        }
    }
    return -1;
}

void player_inventory_additem(uint8_t item_id, uint8_t item_count){
    for(int i = 0; i < MAX_INVENTORY_SLOTS; i++){
        if(gamestatus.state.game.inventory[i].item_id == item_id){
            gamestatus.state.game.inventory[i].item_count += item_count;
            return;
        }
    }
    for(int i = 0; i < MAX_INVENTORY_SLOTS; i++){
        if(gamestatus.state.game.inventory[i].item_id == 0){
            gamestatus.state.game.inventory[i].item_id = item_id;
            gamestatus.state.game.inventory[i].item_count = item_count;
            return;
        }
    }
}

void player_inventory_removeitem_by_item_id(uint8_t item_id, uint8_t item_count){
    for(int i = 0; i < MAX_INVENTORY_SLOTS; i++){
        if(gamestatus.state.game.inventory[i].item_id == item_id){
            gamestatus.state.game.inventory[i].item_count -= item_count;
            if(gamestatus.state.game.inventory[i].item_count <= 0){
                gamestatus.state.game.inventory[i].item_id = 0;
                gamestatus.state.game.inventory[i].item_count = 0;
            }
            return;
        }
    }
}

void player_inventory_removeitem_by_slot(uint8_t slot, uint8_t item_count){
    if(gamestatus.state.game.inventory[slot].item_id == 0){
        return;
    }
    player_inventory_removeitem_by_item_id(gamestatus.state.game.inventory[slot].item_id, item_count);
}

void player_inventory_clear(){
    for(int i = 0; i < MAX_INVENTORY_SLOTS; i++){
        gamestatus.state.game.inventory[i].item_id = 0;
        gamestatus.state.game.inventory[i].item_count = 0;
    }
}

int player_inventory_count_occupied_item_slots(){
    int count = 0;
    for(int i = 0; i < MAX_INVENTORY_SLOTS; i++){
        if(gamestatus.state.game.inventory[i].item_id != 0){
            count++;
        }
    }
    return count;
}

// this is some pretty dumb code
uint8_t player_inventory_get_ith_occupied_item_slot_item_id(int i){
    i+=1;
    int count = 0;
    for(int j = 0; j < MAX_INVENTORY_SLOTS; j++){
        if(gamestatus.state.game.inventory[j].item_id != 0){
            count++;
            if(count == i){
                return gamestatus.state.game.inventory[j].item_id;
            }
        }
    }
    return 0;
}

uint8_t player_inventory_get_ith_occupied_item_slot(int i){
    i+=1;
    int count = 0;
    for(int j = 0; j < MAX_INVENTORY_SLOTS; j++){
        if(gamestatus.state.game.inventory[j].item_id != 0){
            count++;
            if(count == i){
                return j;
            }
        }
    }
    return 0;
}

void player_inventory_combine_items(uint8_t slot1, uint8_t slot2){
    int item_id = items_check_combine_recepies(gamestatus.state.game.inventory[slot1].item_id, gamestatus.state.game.inventory[slot2].item_id);
    if(item_id == -1){
        return;
    }

    uint8_t item_count = 1;
    player_inventory_removeitem_by_slot(slot1, item_count);
    player_inventory_removeitem_by_slot(slot2, item_count);
    player_inventory_additem(item_id, item_count);
}

uint8_t player_inventory_get_item_count(uint8_t slot){
    return gamestatus.state.game.inventory[slot].item_count;
}

void player_pause_menu(){
    sprite_t* selector = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/selector.i8").c_str());
    sprite_t* bg = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/pause_bg.ci4").c_str());
    surface_t bgsurf = sprite_get_pixels(bg);
    sprite_t* bg_b = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/pause_bg_b.i4").c_str());
    sprite_t* button_a = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/button_a.rgba32").c_str());
    sprite_t* button_b = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/button_b.rgba32").c_str());
    sprite_t* gradient = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/pause_gradient.i8").c_str());
    surface_t gradientsurf = sprite_get_pixels(gradient);
    float displayheight = display_get_height();

    rspq_block_t* block = nullptr;
    rspq_wait();
    rdpq_attach(display_get(), NULL);
    rdpq_detach_show();
    rspq_wait();
    if(!block){
        rspq_block_begin();
        rdpq_set_mode_standard();
        rdpq_texparms_t texparms;
        texparms.s.repeats = REPEAT_INFINITE;
        texparms.t.repeats = REPEAT_INFINITE;
        rdpq_tex_upload(TILE0, &bgsurf, &texparms);
        rdpq_texture_rectangle(TILE0, 0,0, display_get_width(), display_get_height(), 0,0);
        rdpq_set_mode_standard();
        rdpq_set_prim_color(RGBA32(255,255,255,255));
        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(0,0,0,TEX0)));
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_sprite_blit(bg_b, 0,0, NULL);
        block = rspq_block_end();
    }

    auto player_pause_menu_show_journal_entry = [&](int collected_entry_id){
        int journal_entry_id = get_collected_journal_entry_id_by_collected_index(collected_entry_id);
        std::string entry_name = (journals[std::to_string(journal_entry_id)]["name"] | "JOURNAL_NAME_MISSING");
        std::string entry_timestamp = (journals[std::to_string(journal_entry_id)]["timestamp"] | "JOURNAL_TIMESTAMP_MISSING");
        std::string entry_text = (journals[std::to_string(journal_entry_id)]["entry"] | "JOURNAL_TEXT_MISSING");

        rdpq_textparms_t textparms;
        textparms.width = 500;
        textparms.align = ALIGN_LEFT; 
        textparms.valign = VALIGN_TOP;
        textparms.wrap = WRAP_WORD;

        std::string combined = (entry_name + "\n" + entry_timestamp + "\n\n" + entry_text);

        int nbytes = strlen(combined.c_str());
        rdpq_paragraph_t* paragraph = rdpq_paragraph_build(&textparms, gamestatus.fonts.subtitlefont, combined.c_str(), &nbytes);
        float height = paragraph->bbox.y1 - paragraph->bbox.y0 - 300;
        if(height < 0 ) height = 0;

        bool pressed_a = false;
        float offset = 0;
        // todo draw
        while(!pressed_a){
            joypad_poll();
            audioutils_mixer_update();
            subtitles_update();
            timesys_update();
            player.joypad.pressed = joypad_get_buttons_pressed(player.joypad.port);
            if(player.joypad.pressed.a) {pressed_a = true; continue;}
            player.joypad.held = joypad_get_buttons_held(JOYPAD_PORT_1);
            if(player.joypad.held.d_down) offset += 3;
            if(player.joypad.held.d_up)   offset -= 3;
            if(offset < 0)  offset  = 0;
            if(offset > height) offset = height;
            
            rdpq_attach(display_get(), NULL);
            
            if(display_interlace_rdp_field() >= 0) 
                rdpq_enable_interlaced(display_interlace_rdp_field());
            else rdpq_disable_interlaced();

            temporal_dither(FRAME_NUMBER);

            rspq_block_run(block);
            rdpq_set_mode_standard();
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            rdpq_sprite_blit(button_a, 550, 400, NULL);
            rdpq_paragraph_render(paragraph, 80, 50 - offset);
            rdpq_detach_show();
        }
        rspq_wait();
        sound_play("menu/select", false);
        rdpq_paragraph_free(paragraph); paragraph = NULL;
    };


    int columnselector = 1; // 0 - items, 1 - main, 2 - journals
    int rowselector = 0;
    gamestatus.paused = true;
    
    int journal_list_draw_offset = 0;
    int item_list_draw_offset = 0;

    int item_selected_first = -1;
    int item_selected_second = -1;
    
    int collectedentries = journals_collected_count();
    debugf("collectedentries: %d\n", collectedentries);
    int collecteditems = player_inventory_count_occupied_item_slots();
    debugf("collecteditems: %d\n", collecteditems);

    while(gamestatus.paused){
        joypad_poll();
        audioutils_mixer_update();
        subtitles_update();
        timesys_update();
        
        collecteditems = player_inventory_count_occupied_item_slots();

        player.joypad.input = joypad_get_inputs(player.joypad.port);
        player.joypad.pressed = joypad_get_buttons_pressed(player.joypad.port);
        player.joypad.held = joypad_get_buttons_held(player.joypad.port);

        joypad_buttons_t btn  = player.joypad.pressed;
        int axis_stick_x = joypad_get_axis_pressed(player.joypad.port, JOYPAD_AXIS_STICK_X);
        int axis_stick_y = joypad_get_axis_pressed(player.joypad.port, JOYPAD_AXIS_STICK_Y);

        if(btn.d_left || axis_stick_x < 0) {columnselector--; rowselector = 0; sound_play("menu/select", false);}
        if(btn.d_right || axis_stick_x > 0) {columnselector++; rowselector = 0; sound_play("menu/select", false);}
        if(btn.d_up || axis_stick_y > 0) {rowselector--; sound_play("menu/select", false);}
        if(btn.d_down || axis_stick_y < 0) {rowselector++; sound_play("menu/select", false);}

        if(btn.start) {
            gamestatus.paused = false;
            continue;
        }

        if(columnselector < 0) columnselector = 0;
        if(columnselector > 2) columnselector = 2;

        if(columnselector == 0 && collecteditems == 0) columnselector = 1;
        if(columnselector == 2 && collectedentries == 0) columnselector = 1;

        float selectorx, selectory;
        float itemselectedselectorx = 0, itemselectedselectory = -100;

        switch(columnselector){
            case 0:{
                if(rowselector < 0) rowselector = collecteditems - 1;
                if(rowselector > collecteditems - 1) rowselector = 0;
                selectorx = 120 - selector->width / 2;
                item_list_draw_offset = maxi(0,rowselector - 3);
                selectory = 100 + (rowselector-item_list_draw_offset)*40;
                itemselectedselectorx = 120 - selector->width / 2;
                if(item_selected_first != -1 && (rowselector-item_list_draw_offset >= 0 && rowselector-item_list_draw_offset <= 5)){
                    itemselectedselectory = 100 + (item_selected_first-item_list_draw_offset)*40;
                } else itemselectedselectory = -100;
                if(btn.a && item_selected_first != -1){
                    player_use_item(player_inventory_get_ith_occupied_item_slot(item_selected_first));
                    gamestatus.paused = false;
                    continue;
                }
                else {
                    if(btn.a && item_selected_first == -1){
                        sound_play("menu/select", false);
                        item_selected_first = rowselector;
                    }
                }
                if(btn.b && item_selected_first != -1){
                    sound_play("menu/select", false);
                    item_selected_second = rowselector;
                    int left_item_slot = player_inventory_get_ith_occupied_item_slot(item_selected_first);
                    int right_item_slot = player_inventory_get_ith_occupied_item_slot(item_selected_second);
                    player_inventory_combine_items(left_item_slot, right_item_slot);
                    item_selected_first = -1;
                    item_selected_second = -1;
                    rowselector = 0;
                }
            } break;
            case 1:{
                if(rowselector < 0) rowselector = 5;
                if(rowselector > 5) rowselector = 0;
                selectorx = 320 - selector->width / 2;
                selectory = 100 + (rowselector)*40;
                if(btn.a)
                    switch(rowselector){
                        case 0:{
                            gamestatus.paused = false;
                            sound_play("menu/select", false);
                            continue;
                        } break;
                        case 1:{
                            playtimelogic_savegame();
                            gamestatus.paused = false;
                            sound_play("menu/select", false);
                            subtitles_add("gamesaved", 3.0f, '\0');
                        } break;
                        case 2:{
                            playtimelogic_loadgame();
                            gamestatus.paused = false;
                            sound_play("menu/select", false);
                        } break;
                        case 3:{
                            {music_volume(1 - music_volume_get()); sound_play("menu/select", false);}
                        } break;
                        case 4:{
                            {sound_volume(1 - sound_volume_get()); sound_play("menu/select", false);}
                        } break;
                        case 5:{
                            playtimelogic_gotomainmenu();
                            gamestatus.paused = false;
                            sound_play("menu/select", false);
                        } break;
                    }
            } break;
            case 2:{
                if(rowselector < 0) rowselector = collectedentries - 1;
                if(rowselector > collectedentries - 1) rowselector = 0;
                journal_list_draw_offset = maxi(0, rowselector - 3);
                selectorx = 520 - selector->width / 2;
                selectory = 100 + (rowselector-journal_list_draw_offset)*40;
                if(btn.a){
                    player_pause_menu_show_journal_entry((rowselector + journal_list_draw_offset));
                }
            } break;
            default: break;
        }

        rdpq_attach(display_get(), NULL);

        if(display_interlace_rdp_field() >= 0) 
            rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();
        
        temporal_dither(FRAME_NUMBER);

        rspq_block_run(block);

        rdpq_textparms_t textparms;
        textparms.width = 200;
        textparms.align = ALIGN_CENTER;
        textparms.wrap = WRAP_ELLIPSES;

        rdpq_text_printf(&textparms, gamestatus.fonts.mainfont, 320 - 100, 80, dictstr("PAUSE_paused"));
        rdpq_text_printf(&textparms, gamestatus.fonts.mainfont, 120 - 100, 80, dictstr("PAUSE_items"));
        rdpq_text_printf(&textparms, gamestatus.fonts.mainfont, 520 - 100, 80, dictstr("PAUSE_journals"));

        rdpq_set_mode_standard();
        rdpq_set_prim_color(RGBA32(255,255,255,255));
        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(0,0,0,TEX0)));
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_sprite_blit(selector, selectorx, selectory, NULL);
        if(item_selected_first != -1){
            rdpq_set_prim_color(RGBA32(100,100,100,255));
            rdpq_sprite_blit(selector, itemselectedselectorx, itemselectedselectory, NULL);
        }

        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,TEX0),(0,0,0,TEX0)));
        rdpq_sprite_blit(button_a, 640 - 120 - 40, 30, NULL);
        textparms.align = ALIGN_LEFT;
        if(columnselector == 0 && item_selected_first != -1){
            rdpq_sprite_blit(button_b, 640 - 320 - 40, 30, NULL);
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 320, 50, dictstr("PAUSE_combine_items")); 
        }
        if(columnselector == 0 && item_selected_first == -1){
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 120, 50, dictstr("PAUSE_select")); 
        } else if (columnselector == 0 && item_selected_first != -1){
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 120, 50, dictstr("PAUSE_use_item")); 
        }
        if(columnselector == 1){
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 120, 50, dictstr("PAUSE_select")); 
        }
        if(columnselector == 2){
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 120, 50, dictstr("PAUSE_read_journal")); 
        }
        textparms.align = ALIGN_CENTER;
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, 120, dictstr("PAUSE_continue")); 
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, 160, dictstr("PAUSE_savegame"));
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, 200, dictstr("PAUSE_loadgame"));
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, 240, dictstr("PAUSE_togglemusic"));
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, 280, dictstr("PAUSE_togglesounds"));
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, 320, dictstr("PAUSE_exittomenu"));

        textparms.width = 170;
        textparms.align = ALIGN_LEFT;

        for(int i = item_list_draw_offset, drawnitems = 0; i < collecteditems && drawnitems < 6; i++, drawnitems++){
            int item_id = player_inventory_get_ith_occupied_item_slot_item_id(i);
            std::string itemname = (itemsdict[items_names[item_id]]["fullname"] | "UNKNOWN_ITEM_NAME");
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 50, 120 + i*40, 
                "%i: %s x %d", i+1, itemname.c_str(), 
                player_inventory_get_item_count(item_id));
            if(i - item_list_draw_offset == rowselector && columnselector == 0){
                rdpq_textparms_t descparms;
                descparms.width = display_get_width() - 60;
                descparms.align = ALIGN_LEFT;
                descparms.wrap = WRAP_WORD;
                rdpq_text_printf(&descparms, gamestatus.fonts.subtitlefont, 30, 420, 
                "%s x %d. %s", itemname.c_str(), 
                player_inventory_get_item_count(item_id), (itemsdict[items_names[item_id]]["description"] | "UNKNOWN_ITEM_DESCRIPTION").c_str());
            }

        }
        
        for(int i = journal_list_draw_offset, drawnentries = 0; i < collectedentries && drawnentries < 6; i++, drawnentries++){
            int journal_entry_id = get_collected_journal_entry_id_by_collected_index(i);
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 450, 120 + i*40, 
                "%i: %s", i + 1, (journals[std::to_string(journal_entry_id)]["name"] | "UNKNOWN_TITLE").c_str());
        }

        rdpq_detach_show();
    }

    rspq_wait();

    if(block) {rspq_block_free(block);} block = NULL;

    sprite_free(selector);
    sprite_free(bg);
    sprite_free(bg_b);
    sprite_free(button_a);
    sprite_free(button_b);
    sprite_free(gradient);
}

void player_update(){
    player.joypad.input = joypad_get_inputs(player.joypad.port);
    player.joypad.pressed = joypad_get_buttons_pressed(player.joypad.port);
    player.joypad.held = joypad_get_buttons_held(player.joypad.port);

    if(player.joypad.pressed.start){
        player_pause_menu();
    }

    player_potential_user = nullptr;

    fm_vec3_t forward = {0}, right = {0}, lookat = {0}, wishdir = {0};
    fm_vec3_t eulerright = player.camera.rotation; eulerright.y += T3D_DEG_TO_RAD(90);
    float maxspeed = player.maxspeed;
    if((player.joypad.held.l || player.joypad.held.r) && !player.joypad.held.z && (player.joypad.held.d_up || player.joypad.held.c_up)) maxspeed *= 2.0f;

    fm_vec3_dir_from_euler(&forward, &player.camera.rotation);
    fm_vec3_dir_from_euler(&right, &eulerright);
    forward.y = 0;
    right.y = 0;
    fm_vec3_norm(&forward, &forward);
    fm_vec3_norm(&right, &right);
    fm_vec3_scale(&forward, &forward, maxspeed);
    fm_vec3_scale(&right, &right, maxspeed);

    if(player.joypad.held.d_left || player.joypad.held.c_left)  fm_vec3_add(&wishdir, &wishdir, &right);
    if(player.joypad.held.d_right || player.joypad.held.c_right) fm_vec3_sub(&wishdir, &wishdir, &right);
    if(player.joypad.held.d_up || player.joypad.held.c_up)    fm_vec3_add(&wishdir, &wishdir, &forward);
    if(player.joypad.held.d_down || player.joypad.held.c_down)  fm_vec3_sub(&wishdir, &wishdir, &forward);
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

    if(player.position.y > 0) player.position.y -= 0.05f;
    else player.position.y = 0;

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

    player.camera.rotation.y   -= T3D_DEG_TO_RAD(1.0f * DELTA_TIME * player.joypad.input.stick_x);
    player.camera.rotation.x   += T3D_DEG_TO_RAD(0.7f * DELTA_TIME * player.joypad.input.stick_y);
    player.camera.rotation.x = fclampr(player.camera.rotation.x, T3D_DEG_TO_RAD(-70), T3D_DEG_TO_RAD(70));

    player.camera.rotation.x = t3d_lerp(player.camera.rotation.x, player.camera.rotation.x, 0.3f);
    player.camera.rotation.y = t3d_lerp(player.camera.rotation.y, player.camera.rotation.y, 0.3f);

    player.camera.position = (fm_vec3_t){{(player.position.x), ((player.position.y + player.charheight)), (player.position.z)}};
    float velocitylen = fm_vec3_len2(&player.velocity);
    player.camera.position.y += sinf(CURRENT_TIME * 12.0f) * velocitylen * T3D_FROMUNITS(0.5f);
    player.camera.rotation.x += cosf(CURRENT_TIME * 6.0f) * velocitylen * 0.0008f;

    if(player.joypad.held.z) player.heightdiff = lerp(player.heightdiff, (player.charheight / 3), 0.2f );
    else player.heightdiff = lerp(player.heightdiff, 0, 0.2f );
    player.camera.position.y -= (player.heightdiff);

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
    //rdpq_text_printf(NULL, 1, 20, 20, "Pos: %.2f, %.2f, %.2f", (player.position.x), (player.position.y), (player.position.z));
    rdpq_text_printf(NULL, 1, 20, 40, "FPS: %.2f", display_get_fps());
   // heap_stats_t stats; sys_get_heap_stats(&stats);
    //rdpq_text_printf(NULL, 1, 20, 60, "MEM: %i total, %i used", stats.total, stats.used);
}
