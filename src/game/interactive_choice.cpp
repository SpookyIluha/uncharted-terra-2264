 #include "interactive_choice.h"
 #include "engine_filesystem.h"
 #include "audioutils.h"
 #include "engine_command.h"
 #include "player.h"
 #include "subtitles.h"
 #include <libdragon.h>
 #include "effects.h"
 
 InteractiveChoice::InteractiveChoice(const std::string& name, int id)
     : Entity(name, INTERACTIVE_CHOICE_TYPE_NAME, id),
       forced_choice(false), range(2.5f) {}
 
 void InteractiveChoice::load_from_ini(const tortellini::ini::section& section){
     title = section["title"] | "";
     choice1_text = section["choice1_text"] | "";
     choice2_text = section["choice2_text"] | "";
     choice1_command = section["choice1_command"] | "";
     choice2_command = section["choice2_command"] | "";
     forced_choice = section["forced_choice"] | false;
     range = section["range"] | 2.5f;
 }
 
void InteractiveChoice::load_from_eeprom(uint16_t data){
    used = (data & INTERACTIVE_CHOICE_USED_BIT) != 0;
}

uint16_t InteractiveChoice::save_to_eeprom() const{
    return used ? INTERACTIVE_CHOICE_USED_BIT : 0;
}

 
void InteractiveChoice::init(){
    used = false;
}
 
 void InteractiveChoice::update(){
     if(!enabled) return;
     float dist = fm_vec3_distance(&transform.position, &player.position);
     if(dist < range && !used){
         subtitles_add(dictstr("interact"), 1.0f, 'A');
         if(player.joypad.pressed.a){
             sound_play("select", false);
             effects_add_rumble(player.joypad.port, 0.1f);
             open_menu();
         }
     }
 }
 
 void InteractiveChoice::draw(){
 }
 
 Entity* InteractiveChoice::create(const std::string& name, int id){
     return new InteractiveChoice(name, id);
 }
 
 void InteractiveChoice::open_menu(){
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
     int selected = 0;
     bool running = true;
     while(running){
         joypad_poll();
         audioutils_mixer_update();
         effects_update();
         rdpq_attach(display_get(), NULL);
         if(display_interlace_rdp_field() >= 0) 
             rdpq_enable_interlaced(display_interlace_rdp_field());
         else rdpq_disable_interlaced();
         temporal_dither(FRAME_NUMBER);
         rspq_block_run(block);
 
         rdpq_textparms_t titleparms; titleparms.align = ALIGN_CENTER; titleparms.width = display_get_width(); titleparms.style_id = gamestatus.fonts.titlefontstyle;
         rdpq_textparms_t textparms; textparms.align = ALIGN_CENTER; textparms.width = display_get_width(); textparms.style_id = gamestatus.fonts.subtitlefontstyle;
         rdpq_text_printf(&titleparms, gamestatus.fonts.titlefont, 0, 160, dictstr(title.c_str()));
 
         int y1 = 240;
         int y2 = 280;
         rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 0, y1 + 16, dictstr(choice1_text.c_str()));
         rdpq_text_printf(&textparms, gamestatus.fonts.subtitlefont, 0, y2 + 16, dictstr(choice2_text.c_str()));
 
         rdpq_set_mode_standard();
         rdpq_set_prim_color(RGBA32(255,255,255,255));
         rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(0,0,0,TEX0)));
         rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
         int selector_y = selected == 0 ? y1 : y2;
         rdpq_blitparms_t blitparms;
         blitparms.cx = selector->width/2;
         blitparms.scale_x = 4;
         rdpq_sprite_blit(selector, display_get_width()/2, selector_y, &blitparms);
 
         rdpq_mode_combiner(RDPQ_COMBINER_TEX);
         rdpq_mode_alphacompare(10);
         rdpq_sprite_blit(button_a, 640 - 120 - 40, 30, NULL);
         if(!forced_choice){
             rdpq_sprite_blit(button_b, 640 - 320 - 40, 30, NULL);
         }
         rdpq_set_mode_standard();
         rdpq_textparms_t t2; t2.align = ALIGN_LEFT; t2.width = display_get_width(); t2.style_id = gamestatus.fonts.subtitlefontstyle;
         rdpq_text_printf(&t2, gamestatus.fonts.subtitlefont, 640 - 120, 50, dictstr("choice_select"));
         if(!forced_choice){
             rdpq_text_printf(&t2, gamestatus.fonts.subtitlefont, 640 - 320, 50, dictstr("choice_back"));
         }
 
         rdpq_detach_show();
 
         joypad_buttons_t btn  = joypad_get_buttons_pressed(JOYPAD_PORT_1);
         int axis_stick_y = joypad_get_axis_pressed(JOYPAD_PORT_1, JOYPAD_AXIS_STICK_Y);
         if(btn.d_up || axis_stick_y > 0) {selected = 0; sound_play("select3", false);}
         if(btn.d_down || axis_stick_y < 0) {selected = 1; sound_play("select3", false);}
         if(btn.a){
             if(selected == 0){
                 if(!choice1_command.empty()) execute_console_command(choice1_command);
             } else {
                 if(!choice2_command.empty()) execute_console_command(choice2_command);
             }
             sound_play("select2", false);
             effects_add_rumble(JOYPAD_PORT_1, 0.1f);
             running = false;
             used = true;
         }
         if(btn.b && !forced_choice){
             sound_play("select", false);
             effects_add_rumble(JOYPAD_PORT_1, 0.1f);
             running = false;
         }
     }
     rspq_wait();
     if(block) {rspq_block_free(block);} block = NULL;
     sprite_free(selector);
     sprite_free(bg);
     sprite_free(bg_b);
     sprite_free(button_a);
     sprite_free(button_b);
 }
