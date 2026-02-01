#ifndef PLAYTIMELOGIC_H
#define PLAYTIMELOGIC_H

#ifdef __cplusplus
extern "C"{
#endif

extern bool gamecomplete;

/// @brief Main gameplay loop
extern void playtimelogic();

extern bool playtimelogic_savegame();
extern bool playtimelogic_loadgame();
extern void playtimelogic_gotomainmenu();
extern void playtimelogic_console_commands_init();

#ifdef __cplusplus
}
#endif

#endif
