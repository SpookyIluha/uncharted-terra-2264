#include "interactive_keypad.h"
#include "engine_command.h"
#include "player.h"
#include "audioutils.h"
#include "subtitles.h"
#include "engine_filesystem.h"
#include <libdragon.h>
#include "effects.h"

InteractiveKeypad::InteractiveKeypad(const std::string& name, int id)
    : Entity(name, INTERACTIVE_KEYPAD_TYPE_NAME, id),
      digit_count(4), range(2.5f), unlocked(false) {}

void InteractiveKeypad::load_from_ini(const tortellini::ini::section& section){
    correct_password = section["password"] | "";
    digit_count = (int)correct_password.length();
    if(digit_count < 1) digit_count = 1;
    if(digit_count > 8) digit_count = 8;
    on_success_command = section["on_success_command"] | "";
    success_subtitle = section["on_success_subtitle"] | "";
    range = section["range"] | 2.5f;
}

void InteractiveKeypad::load_from_eeprom(uint16_t data){
    unlocked = (data & INTERACTIVE_KEYPAD_UNLOCKED_BIT) != 0;
}

uint16_t InteractiveKeypad::save_to_eeprom() const{
    return unlocked ? INTERACTIVE_KEYPAD_UNLOCKED_BIT : 0;
}

void InteractiveKeypad::init(){
}

void InteractiveKeypad::update(){
    if(!enabled || unlocked) return;
    float dist = fm_vec3_distance(&transform.position, &player.position);
    if(dist < range){
        subtitles_add(dictstr("keypad_use"), 1.0f, 'A');
        if(player.joypad.pressed.a){
            sound_play("select", false);
            effects_add_rumble(player.joypad.port, 0.1f);
            open_menu();
        }
    }
}

void InteractiveKeypad::draw(){
}

Entity* InteractiveKeypad::create(const std::string& name, int id){
    return new InteractiveKeypad(name, id);
}

void InteractiveKeypad::open_menu(){
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
    int selected = 0;
    int digits[8] = {0};
    bool running = true;
    while(running){
        joypad_poll();
        audioutils_mixer_update();
        effects_update();
        timesys_update();
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        int axis_x = joypad_get_axis_pressed(JOYPAD_PORT_1, JOYPAD_AXIS_STICK_X);
        int axis_y = joypad_get_axis_pressed(JOYPAD_PORT_1, JOYPAD_AXIS_STICK_Y);
        if(pressed.b || pressed.start) {running = false; sound_play("robotnoise", false); effects_add_rumble(JOYPAD_PORT_1, 0.1f); break;}
        if(pressed.a){
            char input[16] = {0};
            for(int i=0;i<digit_count;i++){
                input[i] = (char)('0' + digits[i]);
            }
            input[digit_count] = '\0';
            if(!correct_password.empty() && std::string(input) == correct_password){
                if(!on_success_command.empty()){
                    execute_console_command(on_success_command);
                }
                if(!success_subtitle.empty()){
                    subtitles_add(dictstr(success_subtitle.c_str()), 5.0f, '\0', 10);
                }
                sound_play("consolebeep", false);
                effects_add_rumble(JOYPAD_PORT_1, 0.1f);
                unlocked = true;
                running = false;
                break;
            } else {
                sound_play("invalid2", false);
                effects_add_rumble(JOYPAD_PORT_1, 0.1f);
            }
        }
        if(pressed.d_left || axis_x < 0){ selected--; sound_play("chime1", false); effects_add_rumble(JOYPAD_PORT_1, 0.1f); }
        if(pressed.d_right || axis_x > 0){ selected++; sound_play("chime1", false); effects_add_rumble(JOYPAD_PORT_1, 0.1f); }
        if(selected < 0) selected = digit_count - 1;
        if(selected >= digit_count) selected = 0;
        if(pressed.d_up || axis_y > 0){ digits[selected] = (digits[selected] + 1) % 10; sound_play("chime1", false); effects_add_rumble(JOYPAD_PORT_1, 0.1f); }
        if(pressed.d_down || axis_y < 0){ digits[selected] = (digits[selected] + 9) % 10; sound_play("chime1", false); effects_add_rumble(JOYPAD_PORT_1, 0.1f); }
        rdpq_attach(display_get(), NULL);
        if(display_interlace_rdp_field() >= 0) rdpq_enable_interlaced(display_interlace_rdp_field());
        else rdpq_disable_interlaced();
        temporal_dither(FRAME_NUMBER);
        rspq_block_run(block);
        rdpq_mode_zbuf(false,false);
        rdpq_sync_pipe();
        rdpq_sync_tile();
        rdpq_set_mode_standard();
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_textparms_t text;
        text.align = ALIGN_CENTER;
        text.width = display_get_width();
        text.style_id = gamestatus.fonts.subtitlefontstyle;
        // Title bar
        rdpq_text_printf(&text, gamestatus.fonts.mainfont, 0, 180, dictstr("keypad_enter_password"));
        // Digits and rectangles
        int base_y = 240;
        int spacing = 40;
        int center = display_get_width() / 2;
        rdpq_textparms_t digitparms;
        digitparms.align = ALIGN_LEFT;
        digitparms.width = display_get_width();
        digitparms.style_id = gamestatus.fonts.subtitlefontstyle;
        for(int i=0;i<digit_count;i++){
            int x = center - (digit_count*spacing)/2 + i*spacing;
            char ch[2] = { (char)('0' + digits[i]), 0 };
            rdpq_text_printf(&digitparms, gamestatus.fonts.subtitlefont, x - 6, base_y, "%s", ch);
        }
        for(int i=0;i<digit_count;i++){
            int x = center - (digit_count*spacing)/2 + i*spacing;
            color_t c = RGBA32(i==selected? 255:100, i==selected? 255:100, i==selected? 255:100, 255);
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
            rdpq_set_prim_color(c);
            rdpq_fill_rectangle(x-15, base_y+20, x+15, base_y+30);
        }
        // Buttons similar to pause menu
        rdpq_textparms_t t2; t2.align = ALIGN_LEFT; t2.width = display_get_width(); t2.style_id = gamestatus.fonts.subtitlefontstyle;
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_mode_alphacompare(10);
        rdpq_sprite_blit(button_a, 640 - 120 - 40, 30, NULL);
        rdpq_sprite_blit(button_b, 640 - 320 - 40, 30, NULL);
        rdpq_set_mode_standard();
        rdpq_text_printf(&t2, gamestatus.fonts.subtitlefont, 640 - 120, 50, dictstr("keypad_commit"));
        rdpq_text_printf(&t2, gamestatus.fonts.subtitlefont, 640 - 320, 50, dictstr("keypad_back"));
        rdpq_detach_show();
    }
    rspq_wait();
    if(block) {rspq_block_free(block);} block = NULL;
    sprite_free(bg);
    sprite_free(bg_b);
    sprite_free(button_a);
    sprite_free(button_b);
}
