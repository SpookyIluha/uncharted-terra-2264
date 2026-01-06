#include <libdragon.h>
#include <string>
#include <fstream>
#include <sstream>
#include "engine_filesystem.h"
#include "engine_t3dm_wrapper.h"
#include "audioutils.h"
#include "utils.h"
#include "types.h"
#include "effects.h"
#include "level.h"
#include "player.h"
#include "playtimelogic.h"



void playtimelogic(){

  const T3DVec3 camPos = {{0,0.0f,0.0f}};
  const T3DVec3 camTarget = {{0,0,0}};
  const T3DVec3 camUp = {{0,1,0}};

  uint8_t colorAmbient[4] = {255, 255, 255, 0xFF};
  uint8_t colorDir[4]     = {0xEE, 0xAA, 0xAA, 0xFF};

  T3DVec3 lightDirVec = {{-1.0f, 1.0f, 1.0f}};
  T3DViewport viewport = t3d_viewport_create_buffered(3);
  int frameIdx = 0;

  currentlevel.load("start_ship");

  player_init();
  player.camera.far_plane = T3D_TOUNITS(currentlevel.drawdistance);

  while(true){
    timesys_update();
    joypad_poll();
    audioutils_mixer_update();
    player_update();
    traversal_update(); // Check for level transitions
    traversal_fade_update(); // Update fade from black
    // ======== Update ======== //
    // cycle through FP matrices to avoid overwriting what the RSP may still need to load
    frameIdx = (frameIdx + 1) % 3;


    // ======== Draw ======== //
    rdpq_attach(display_get(), display_get_zbuf());
    rdpq_clear_z(ZBUF_MAX);

    t3d_frame_start();
    t3d_viewport_attach(&viewport);

    if(display_interlace_rdp_field() >= 0) 
        rdpq_enable_interlaced(display_interlace_rdp_field());
    else rdpq_disable_interlaced();

    player_draw(&viewport);
    temporal_dither(frameIdx);

    rdpq_mode_antialias(AA_REDUCED);
    rdpq_mode_zmode(ZMODE_INTERPENETRATING);
    currentlevel.draw();
    player_draw_ui();
    traversal_fade_draw(); // Draw fade from black overlay
    rdpq_detach_show();
  }
}