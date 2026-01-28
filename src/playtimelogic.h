#ifndef PLAYTIMELOGIC_H
#define PLAYTIMELOGIC_H

#ifdef __cplusplus
extern "C"{
#endif

/// @brief Main gameplay loop
extern void playtimelogic();

extern void playtimelogic_savegame();
extern void playtimelogic_loadgame();
extern void playtimelogic_gotomainmenu();
extern void playtimelogic_console_commands_init();

#ifdef __cplusplus
}
#endif

#endif
