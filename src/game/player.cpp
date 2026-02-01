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
#include "effects.h"

player_t player;
int frame = 0;
float pltime = 0;
sprite_t* ui_play;
sprite_t* ui_play2;
sprite_t* ui_play3;

wav64_t walksound;
float   walksoundvolume;
wav64_t fldisablesound;
wav64_t flenablesound;
wav64_t fllockedsound;
surface_t prevbuffer = {0};

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
    player.joypad.mouseport = JOYPAD_PORT_2;

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
        sound_play("screw", false);
        return;
    }
    sound_play("itemuse", false);

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
        textparms.wrap = gamestatus.fonts.wrappingmode;

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
            effects_update();
            subtitles_update();
            timesys_update();
            player.joypad.pressed = joypad_get_buttons_pressed(player.joypad.port);
            if(player.joypad.pressed.a) {effects_add_rumble(player.joypad.port, 0.1f); pressed_a = true; continue;}
            player.joypad.held = joypad_get_buttons_held(JOYPAD_PORT_1);
            if(player.joypad.held.d_down) {offset += 3;}
            if(player.joypad.held.d_up)   {offset -= 3;}
            if(player.joypad.held.c_down) {offset += 3;}
            if(player.joypad.held.c_up)   {offset -= 3;}
            if(player.joypad.input.stick_y > 30) {offset -= 3;}
            if(player.joypad.input.stick_y < -30) {offset += 3;}

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
            rdpq_sprite_blit(button_a, 550, 40, NULL);
            rdpq_paragraph_render(paragraph, 80, 50 - offset);
            rdpq_detach_show();
        }
        rspq_wait();
        sound_play("select2", false);
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

    sound_play("pause_start", false);

    while(gamestatus.paused){
        joypad_poll();
        audioutils_mixer_update();
        effects_update();
        subtitles_update();
        timesys_update();
        
        collecteditems = player_inventory_count_occupied_item_slots();

        player.joypad.input = joypad_get_inputs(player.joypad.port);
        player.joypad.pressed = joypad_get_buttons_pressed(player.joypad.port);
        player.joypad.held = joypad_get_buttons_held(player.joypad.port);

        joypad_buttons_t btn  = player.joypad.pressed;
        btn.raw |= joypad_get_buttons_pressed(player.joypad.mouseport).raw;
        int axis_stick_x = joypad_get_axis_pressed(player.joypad.port, JOYPAD_AXIS_STICK_X);
        int axis_stick_y = joypad_get_axis_pressed(player.joypad.port, JOYPAD_AXIS_STICK_Y);
        int mouse_axis_stick_x = joypad_get_axis_pressed(player.joypad.mouseport, JOYPAD_AXIS_STICK_X);
        if(mouse_axis_stick_x != 0){
            axis_stick_x = mouse_axis_stick_x;
        }
        int mouse_axis_stick_y = joypad_get_axis_pressed(player.joypad.mouseport, JOYPAD_AXIS_STICK_Y);
        if(mouse_axis_stick_y != 0){
            axis_stick_y = mouse_axis_stick_y;
        }

        const char* select_sound = "inventoryselect";

        if(btn.d_left || axis_stick_x < 0) {columnselector--; rowselector = 0; sound_play((columnselector == 1 || columnselector == 2) ? "select3" : select_sound, false); effects_add_rumble(player.joypad.port, 0.1f);}
        if(btn.d_right || axis_stick_x > 0) {columnselector++; rowselector = 0; sound_play((columnselector == 1 || columnselector == 2) ? "select3" : select_sound, false); effects_add_rumble(player.joypad.port, 0.1f);}
        if(btn.d_up || axis_stick_y > 0) {rowselector--; sound_play((columnselector == 1 || columnselector == 2) ? "select3" : select_sound, false); effects_add_rumble(player.joypad.port, 0.1f);}
        if(btn.d_down || axis_stick_y < 0) {rowselector++; sound_play((columnselector == 1 || columnselector == 2) ? "select3" : select_sound, false); effects_add_rumble(player.joypad.port, 0.1f);}

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
        const int start_y = is_memory_expanded()? 0 : 0;

        switch(columnselector){
            case 0:{
                if(rowselector < 0) rowselector = collecteditems - 1;
                if(rowselector > collecteditems - 1) rowselector = 0;
                selectorx = 120 - selector->width / 2;
                item_list_draw_offset = maxi(0,rowselector - 3);
                selectory = 80 + (rowselector-item_list_draw_offset)*30;
                itemselectedselectorx = 120 - selector->width / 2;
                if(item_selected_first != -1 && (rowselector-item_list_draw_offset >= 0 && rowselector-item_list_draw_offset <= 5)){
                    itemselectedselectory = 80 + (item_selected_first-item_list_draw_offset)*30;
                } else itemselectedselectory = -100;
                if(btn.a && item_selected_first != -1){
                    player_use_item(player_inventory_get_ith_occupied_item_slot(item_selected_first));
                    gamestatus.paused = false;
                    effects_add_rumble(player.joypad.port, 0.1f);
                    continue;
                }
                else {
                    if(btn.a && item_selected_first == -1){
                        sound_play("select2", false);
                        item_selected_first = rowselector;
                        effects_add_rumble(player.joypad.port, 0.1f);
                    }
                }
                if(btn.b && item_selected_first != -1){
                    sound_play("select2", false);
                    item_selected_second = rowselector;
                    int left_item_slot = player_inventory_get_ith_occupied_item_slot(item_selected_first);
                    int right_item_slot = player_inventory_get_ith_occupied_item_slot(item_selected_second);
                    player_inventory_combine_items(left_item_slot, right_item_slot);
                    item_selected_first = -1;
                    item_selected_second = -1;
                    rowselector = 0;
                    effects_add_rumble(player.joypad.port, 0.1f);
                }
            } break;
            case 1:{
                if(rowselector < 0) rowselector = 5;
                if(rowselector > 5) rowselector = 0;
                selectorx = 320 - selector->width / 2;
                selectory = 80 + (rowselector)*30;
                if(btn.a)
                    switch(rowselector){
                        case 0:{
                            gamestatus.paused = false;
                            sound_play("select2", false);
                            continue;
                        } break;
                        case 1:{
                            if(playtimelogic_savegame()){
                                gamestatus.paused = false;
                                sound_play("select2", false);
                                effects_add_rumble(player.joypad.port, 0.1f);
                                subtitles_add(dictstr("MM_saved"), 3.0f, '\0');
                            } else {
                                subtitles_add(dictstr("MM_savenfound"), 3.0f, '\0');
                                gamestatus.paused = false;
                                sound_play("screw", false);
                                effects_add_rumble(player.joypad.port, 0.1f);
                            }
                        } break;
                        case 2:{
                            if(playtimelogic_loadgame()){
                                gamestatus.paused = false;
                                sound_play("select2", false);
                                effects_add_rumble(player.joypad.port, 0.1f);
                            } else {
                                subtitles_add(dictstr("MM_savenfound"), 3.0f, '\0');
                                gamestatus.paused = false;
                                sound_play("screw", false);
                                effects_add_rumble(player.joypad.port, 0.1f);
                            }
                        } break;
                        case 3:{
                            {music_volume(1 - music_volume_get()); sound_play("select2", false);}
                        } break;
                        case 4:{
                            {sound_volume(1 - sound_volume_get()); sound_play("select2", false);}
                        } break;
                        case 5:{
                            playtimelogic_gotomainmenu();
                            gamestatus.paused = false;
                            sound_play("select2", false);
                            effects_add_rumble(player.joypad.port, 0.1f);
                        } break;
                    }
            } break;
            case 2:{
                if(rowselector < 0) rowselector = collectedentries - 1;
                if(rowselector > collectedentries - 1) rowselector = 0;
                journal_list_draw_offset = maxi(0, rowselector - 3);
                selectorx = 520 - selector->width / 2;
                selectory = 80 + (rowselector-journal_list_draw_offset)*30;
                if(btn.a){
                    sound_play("journal", false);
                    effects_add_rumble(player.joypad.port, 0.1f);
                    player_pause_menu_show_journal_entry(rowselector);
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

        rdpq_text_printf(&textparms, gamestatus.fonts.mainfont, 320 - 100, start_y + 60, dictstr("PAUSE_paused"));
        rdpq_text_printf(&textparms, gamestatus.fonts.mainfont, 120 - 100, start_y + 60, dictstr("PAUSE_items"));
        rdpq_text_printf(&textparms, gamestatus.fonts.mainfont, 520 - 100, start_y + 60, dictstr("PAUSE_journals"));

        rdpq_set_mode_standard();
        rdpq_set_prim_color(RGBA32(255,255,255,255));
        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(0,0,0,TEX0)));
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_sprite_blit(selector, selectorx, start_y + selectory, NULL);
        if(item_selected_first != -1){
            rdpq_set_prim_color(RGBA32(100,100,100,255));
            rdpq_sprite_blit(selector, itemselectedselectorx, start_y + itemselectedselectory, NULL);
        }

        rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,TEX0),(0,0,0,TEX0)));
        rdpq_sprite_blit(button_a, 640 - 120 - 40, start_y + 10, NULL);
        textparms.align = ALIGN_LEFT;
        if(columnselector == 0 && item_selected_first != -1){
            rdpq_sprite_blit(button_b, 640 - 320 - 40, start_y + 10, NULL);
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 320, start_y + 30, dictstr("PAUSE_combine_items")); 
        }
        if(columnselector == 0 && item_selected_first == -1){
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 120, start_y + 30, dictstr("PAUSE_select")); 
        } else if (columnselector == 0 && item_selected_first != -1){
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 120, start_y + 30, dictstr("PAUSE_use_item")); 
        }
        if(columnselector == 1){
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 120, start_y + 30, dictstr("PAUSE_select")); 
        }
        if(columnselector == 2){
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 640 - 120, start_y + 30, dictstr("PAUSE_read_journal")); 
        }
        textparms.align = ALIGN_CENTER;
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, start_y + 100, dictstr("PAUSE_continue")); 
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, start_y + 130, dictstr("PAUSE_savegame"));
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, start_y + 160, dictstr("PAUSE_loadgame"));
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, start_y + 190, dictstr("PAUSE_togglemusic"));
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, start_y + 220, dictstr("PAUSE_togglesounds"));
        rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 320 - 100, start_y + 250, dictstr("PAUSE_exittomenu"));

        textparms.width = 170;
        textparms.align = ALIGN_LEFT;

        for(int i = item_list_draw_offset, drawnitems = 0; i < collecteditems && drawnitems < 6; i++, drawnitems++){
            int item_slot = player_inventory_get_ith_occupied_item_slot(i);
            int item_id = player_inventory_get_ith_occupied_item_slot_item_id(i);
            std::string itemname = (itemsdict[items_names[item_id]]["fullname"] | "UNKNOWN_ITEM_NAME");
            int y = start_y + 100 + drawnitems*30;
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 50, y, 
                "%i: %s x %d", i+1, itemname.c_str(), 
                player_inventory_get_item_count(item_slot));
            if(i == rowselector && columnselector == 0){
                rdpq_textparms_t descparms;
                descparms.width = display_get_width() - 60;
                descparms.align = ALIGN_LEFT;
                descparms.wrap = gamestatus.fonts.wrappingmode;
                rdpq_text_printf(&descparms, gamestatus.fonts.subtitlefont, 30, display_get_height() - 70, 
                    "%s x %d. %s", itemname.c_str(), 
                    player_inventory_get_item_count(item_slot), (itemsdict[items_names[item_id]]["description"] | "UNKNOWN_ITEM_DESCRIPTION").c_str());
            }
        }
        
        for(int i = journal_list_draw_offset, drawnentries = 0; i < collectedentries && drawnentries < 6; i++, drawnentries++){
            int journal_entry_id = get_collected_journal_entry_id_by_collected_index(i);
            int y = start_y + 100 + drawnentries*30;
            rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 450, y, 
                "%i: %s", i + 1, (journals[std::to_string(journal_entry_id)]["name"] | "UNKNOWN_TITLE").c_str());
        }

        rdpq_detach_show();
    }
    sound_play("pause_end", false);
    rspq_wait();

    if(block) {rspq_block_free(block);} block = NULL;

    sprite_free(selector);
    sprite_free(bg);
    sprite_free(bg_b);
    sprite_free(button_a);
    sprite_free(button_b);
}

void player_update(){
    player.joypad.input = joypad_get_inputs(player.joypad.port);
    player.joypad.pressed = joypad_get_buttons_pressed(player.joypad.port);
    player.joypad.held = joypad_get_buttons_held(player.joypad.port);

    joypad_inputs_t mouseinput = joypad_get_inputs(player.joypad.mouseport);

    if (mouseinput.stick_x > 20 || mouseinput.stick_x < -20)
        player.joypad.input.stick_x = mouseinput.stick_x;
    if (mouseinput.stick_y > 20 || mouseinput.stick_y < -20)
        player.joypad.input.stick_y = mouseinput.stick_y;
    
    player.joypad.pressed.raw |= joypad_get_buttons_pressed(player.joypad.mouseport).raw;
    player.joypad.held.raw |= joypad_get_buttons_held(player.joypad.mouseport).raw;

    if(player.joypad.pressed.start){
        player_pause_menu();
    }

    player_potential_user = nullptr;

    fm_vec3_t forward = {0}, right = {0}, wishdir = {0};
    fm_vec3_t eulerright = player.camera.rotation; eulerright.y += T3D_DEG_TO_RAD(90);
    float maxspeed = player.maxspeed;
    bool running = false;
    if((player.joypad.held.l || player.joypad.held.r) && !player.joypad.held.z && (player.joypad.held.d_up || player.joypad.held.c_up)) running = true;
    if(running) maxspeed *= 2.0f;

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

    float pltimelast = pltime;
    pltime += running? DELTA_TIME * 2 : DELTA_TIME * 2;
    if(wishdirlen > 0.5f)
        if((int)pltime != (int)pltimelast)
            if(!currentlevel.floormaterial.empty()){
                int soundidx = (rand() % 4) + 1;
                sound_play(("footstep_" + currentlevel.floormaterial + std::to_string(soundidx)).c_str(), false);
            }
    
    if(player.joypad.input.stick_x < 20 && player.joypad.input.stick_x > -20) player.joypad.input.stick_x = 0;
    if(player.joypad.input.stick_y < 20 && player.joypad.input.stick_y > -20) player.joypad.input.stick_y = 0;
    player.camera.rotation.y   -= T3D_DEG_TO_RAD(1.0f * DELTA_TIME * player.joypad.input.stick_x);
    player.camera.rotation.x   += T3D_DEG_TO_RAD(0.7f * DELTA_TIME * player.joypad.input.stick_y);
    player.camera.rotation.x = fclampr(player.camera.rotation.x, T3D_DEG_TO_RAD(-70), T3D_DEG_TO_RAD(70));

    player.camera.rotation.x = t3d_lerp(player.camera.rotation.x, player.camera.rotation.x, 0.3f);
    player.camera.rotation.y = t3d_lerp(player.camera.rotation.y, player.camera.rotation.y, 0.3f);

    player.camera.position = (fm_vec3_t){{(player.position.x), ((player.position.y + player.charheight)), (player.position.z)}};
    float velocitylen = fm_vec3_len2(&player.velocity);
    player.camera.position.y += sinf(CURRENT_TIME * 12.0f) * velocitylen * T3D_FROMUNITS(0.5f);
    player.camera.rotation.x += cosf(CURRENT_TIME * 6.0f) * velocitylen * 0.0008f;

    player.camera.rotation.x += cosf(CURRENT_TIME * 2.0f) * 0.0005f;

    if(player.joypad.held.z) player.heightdiff = lerp(player.heightdiff, (player.charheight / 3), 0.2f );
    else player.heightdiff = lerp(player.heightdiff, 0, 0.2f );
    if(player.joypad.pressed.z) sound_play("player_crouch", false);
    player.camera.position.y -= (player.heightdiff);

    frame++;
}

void player_draw(T3DViewport *viewport){
    camera_transform(&player.camera, viewport);
}

void player_draw_ui(){

}
