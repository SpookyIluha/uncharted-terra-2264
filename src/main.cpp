#include <libdragon.h>
#include <t3d/t3d.h>
#include "intro.h"
#include "engine_gamestatus.h"
#include "engine_eeprom.h"
#include "engine_filesystem.h"
#include "engine_setup.h"
#include "mainmenu.h"
#include "level.h"
#include "subtitles.h"
#include "game/entity_register.h"

void systems_init(){
    debug_init_isviewer();
    debug_init_usblog();

    dfs_init(DFS_DEFAULT_LOCATION);
    //asset_init_compression(3);
    //wav64_init_compression(3);

    audio_init(28000, 4);
    mixer_init(24);

    rdpq_init();
    timer_init();
    joypad_init();
    timesys_init();
    srand(getentropy32());
    register_VI_handler((void(*)())rand);

    T3DInitParams t3dparms = {};
    t3d_init(t3dparms);

    engine_eeprom_init();
    engine_eeprom_load_persistent();
    filesystem_init();
    check_language_config();
    check_memory_expanded();
    engine_level_init();
    entity_register_all();
    subtitles_init();
}

int main(void)
{
    systems_init();
    //libdragon_logo();
    //movie_play("rom:/movies/intrologo.m1v", NULL, 15);
    //game_logo();
    game_start();
}