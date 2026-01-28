#include <libdragon.h>
#include <string>
#include <fstream>
#include <sstream>
#include "engine_filesystem.h"
#include "engine_t3dm_wrapper.h"
#include "engine_eeprom.h"
#include "audioutils.h"
#include "utils.h"
#include "types.h"
#include "effects.h"
#include "level.h"
#include "player.h"
#include "playtimelogic.h"
#include "subtitles.h"
#include "game/entity.h"
#include "engine_command.h"
#include "intro.h"

extern "C" {
  void* sbrk_top(int incr);
}

bool must_save_game_on_next_frame = false;
bool must_load_game_on_next_frame = false;
bool must_goto_main_menu = false;
static int must_play_ending = 0;

void playtimelogic_savegame(){
  must_save_game_on_next_frame = true;
}

void playtimelogic_loadgame(){
  must_load_game_on_next_frame = true;  
}
void playtimelogic_gotomainmenu(){
  must_goto_main_menu = true;
}

void playtimelogic_console_commands_init(){
  register_game_console_command("game_ending_a", [](std::vector<std::string> args){
    must_play_ending = 1;
    must_goto_main_menu = true;
  });
  register_game_console_command("game_ending_b", [](std::vector<std::string> args){
    must_play_ending = 2;
    must_goto_main_menu = true;
  });
}

void savegame(){
  currentlevel.save_eeprom();
  engine_eeprom_save_manual();
  engine_eeprom_save_persistent();
}

void loadgame(){
  engine_eeprom_load_persistent();
  engine_eeprom_load_manual();
  currentlevel.load(gamestatus.state.game.levelname);
  player.camera.far_plane = T3D_TOUNITS(currentlevel.drawdistance);
}

void playtimelogic(){
  must_goto_main_menu = false;
  must_play_ending = 0;
  T3DViewport viewport = t3d_viewport_create_buffered(12);
  
  player_init();
  surface_t surf_zbuf;
  /* Try to allocate the Z-Buffer from the top of the heap (near the stack).
      This basically puts it in the last memory bank, hopefully separating it
      from framebuffers, which provides a nice speed gain. */
  int width, height;
  bool zbuf_sbrk_top = false;
  width = display_get_width();
  height = display_get_height();
  void *buf = sbrk_top(width * height * 2);
  if (buf != (void*)-1) {
      data_cache_hit_invalidate(buf, width * height * 2);
      surf_zbuf = surface_make(UncachedAddr(buf), FMT_RGBA16, width, height, width*2);
      zbuf_sbrk_top = true;
  } else {
      surf_zbuf = surface_alloc(FMT_RGBA16, width, height);
      zbuf_sbrk_top = false;
  }

  if(!must_load_game_on_next_frame){
    currentlevel.load(gamestatus.startlevelname);
  }
  
  while(!must_goto_main_menu){
    if(must_save_game_on_next_frame){
      savegame();
      must_save_game_on_next_frame = false;
    }
    if(must_load_game_on_next_frame){
      loadgame();
      player_init();
      must_load_game_on_next_frame = false;
    }
    player.camera.far_plane = T3D_TOUNITS(currentlevel.drawdistance);
    // ======== Update ======== //
    timesys_update();
    joypad_poll();
    audioutils_mixer_update();
    player_update();
    subtitles_update();
    traversal_update(); // Check for level transitions
    traversal_fade_update(); // Update fade from black
    EntitySystem::update_all(); // Update all entities

    // ======== Draw ======== //
    rdpq_attach(display_get(), &surf_zbuf);

    if(display_interlace_rdp_field() >= 0) 
        rdpq_enable_interlaced(display_interlace_rdp_field());
    else rdpq_disable_interlaced();

    rdpq_clear_z(ZBUF_MAX);
    mixer_try_play();

    t3d_frame_start();
    t3d_viewport_attach(&viewport);

    player_draw(&viewport);

    rdpq_set_mode_standard();
    temporal_dither(FRAME_NUMBER);
    rdpq_mode_persp(true);
    t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);
    mixer_try_play();
    rdpq_mode_antialias(AA_REDUCED);
    rdpq_mode_zmode(ZMODE_INTERPENETRATING);
    currentlevel.draw();
    rdpq_sync_pipe();
    rdpq_sync_tile();
    rdpq_mode_persp(true);
    EntitySystem::draw_all(); // Draw all entities
    rdpq_sync_pipe();
    rdpq_sync_tile();
    player_draw_ui();
    subtitles_draw();
    traversal_fade_draw(); // Draw fade from black overlay
    mixer_try_play();
    rdpq_sync_pipe();

    if(gamestatus.state.scripts.debug){
      rdpq_text_printf(NULL, 1, 20, 40, "FPS: %.2f", display_get_fps());
      heap_stats_t stats; sys_get_heap_stats(&stats);
      rdpq_text_printf(NULL, 1, 20, 60, "MEM: %i total, %i used", stats.total, stats.used);
    }
    
    rdpq_detach_show();
    mixer_try_play();
  }
  rspq_wait();
  t3d_viewport_destroy(&viewport);
  currentlevel.free();

  if ( surf_zbuf.buffer )
  {
    surface_free(&surf_zbuf);
    if (zbuf_sbrk_top) {
      sbrk_top(-width * height * 2);
      zbuf_sbrk_top = false;
    }
  }

  if(must_play_ending){
    display_close();
    sound_stop();
    bgm_play("a_different_existenz", true, 0);
    std::string moviename = must_play_ending == 1? "movie_a" : "movie_b";
    std::string moviefn = filesystem_getfn(DIR_MOVIES, moviename.c_str());
    movie_play(const_cast<char*>(moviefn.c_str()), must_play_ending == 1? "ambience1" : "ambience_intro", 15);

    display_init((resolution_t){
        .width = 640, .height = 240,
        .interlaced = INTERLACE_HALF,
        .aspect_ratio = (float)640 / 240,
      },
      DEPTH_16_BPP,
      NUM_BUFFERS, GAMMA_NONE,
      FILTERS_DEDITHER
    );

    auto play_scroll = [](const char* text, const char* image){
      sprite_t* bg = sprite_load(filesystem_getfn(DIR_IMAGE, image).c_str() );
      surface_t bgsurf = sprite_get_pixels(bg);
      float time = 0;
      std::ifstream t(filesystem_getfn(DIR_SCRIPT_LANG, text).c_str());
      std::stringstream buffer;
      buffer << t.rdbuf();
      rdpq_textparms_t textparms; textparms.align = ALIGN_CENTER; textparms.width = display_get_width() - 80; textparms.style_id = gamestatus.fonts.titlefontstyle; textparms.wrap = WRAP_WORD;
      int nbytes = strlen(buffer.str().c_str());
      rdpq_paragraph_t* paragraph = rdpq_paragraph_build(&textparms, gamestatus.fonts.titlefont, buffer.str().c_str(), &nbytes);
      float text_height = paragraph->bbox.y1 - paragraph->bbox.y0;
      float rate = 10.0f;
      float fade_time = 5.0f;
      float duration = (text_height + 260.0f) / rate;
      bool skip = false;
      while(!skip && time < duration){
        timesys_update();
        joypad_poll();
        time += display_get_delta_time();
        audioutils_mixer_update();
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        pressed.raw |= joypad_get_buttons_pressed(JOYPAD_PORT_2).raw;
        if(pressed.a) skip = true;
        float alpha = 1.0f;
        if(time < 1.0f) alpha = time;
        float fade_start = duration - fade_time;
        if(time > fade_start) alpha = 1.0f - (time - fade_start) / fade_time;
        if(alpha > 1.0f) alpha = 1.0f;
        if(alpha < 0.0f) alpha = 0.0f;
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
        temporal_dither(FRAME_NUMBER);
        rdpq_sprite_blit(bg, display_get_width() / 2, display_get_height() / 2, &blitparms);
        rdpq_mode_filter(FILTER_BILINEAR);
        rdpq_paragraph_render(paragraph, 40, (int)(scissor_y + 140 - time * rate));
        rdpq_mode_combiner(RDPQ_COMBINER_TEX);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_sync_tile();
        rdpq_detach_show();
      }
      rspq_wait();
      sprite_free(bg);
      rdpq_paragraph_free(paragraph);
      sound_stop();
    };

    play_scroll(must_play_ending == 1? "end_a.txt" : "end_b.txt", "menu/storytime");
    play_scroll("credits.txt", "menu/storytime2");
    display_close();
    bgm_stop(1.0f);
    game_logo();
    sound_stop();
    audioutils_mixer_update();
    rspq_wait();

    engine_display_init_default();
  }
}
