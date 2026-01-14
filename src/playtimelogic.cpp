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

bool must_save_game_on_next_frame = false;
bool must_load_game_on_next_frame = false;
bool must_goto_main_menu = false;

void playtimelogic_savegame(){
  must_save_game_on_next_frame = true;
}

void playtimelogic_loadgame(){
  must_load_game_on_next_frame = true;  
}
void playtimelogic_gotomainmenu(){
  must_goto_main_menu = true;
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
  const T3DVec3 camPos = {{0,0.0f,0.0f}};
  const T3DVec3 camTarget = {{0,0,0}};
  const T3DVec3 camUp = {{0,1,0}};

  uint8_t colorAmbient[4] = {255, 255, 255, 0xFF};
  uint8_t colorDir[4]     = {0xEE, 0xAA, 0xAA, 0xFF};

  T3DVec3 lightDirVec = {{-1.0f, 1.0f, 1.0f}};
  T3DViewport viewport = t3d_viewport_create_buffered(12);
  int frameIdx = 0;
  
  player_init();

  if(!must_load_game_on_next_frame){
    currentlevel.load(gamestatus.startlevelname);
    player.camera.far_plane = T3D_TOUNITS(currentlevel.drawdistance);
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
    rdpq_attach(display_get(), display_get_zbuf());
    rdpq_clear_z(ZBUF_MAX);

    t3d_frame_start();
    t3d_viewport_attach(&viewport);

    if(display_interlace_rdp_field() >= 0) 
        rdpq_enable_interlaced(display_interlace_rdp_field());
    else rdpq_disable_interlaced();

    player_draw(&viewport);

    rdpq_set_mode_standard();
    temporal_dither(FRAME_NUMBER);
    rdpq_mode_persp(true);
    t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);

    rdpq_mode_antialias(AA_REDUCED);
    rdpq_mode_zmode(ZMODE_INTERPENETRATING);
    currentlevel.draw();
    rdpq_mode_persp(true);
    EntitySystem::draw_all(); // Draw all entities
    player_draw_ui();
    subtitles_draw();
    traversal_fade_draw(); // Draw fade from black overlay
    rdpq_detach_show();
  }
  rspq_wait();
}