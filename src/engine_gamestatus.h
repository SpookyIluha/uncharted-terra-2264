#ifndef ENGINE_GAMESTATUS_H
#define ENGINE_GAMESTATUS_H
/// made by SpookyIluha
/// The global gamestatus variables that can be used at runtime and EEPROM saving

#include "types.h"

#ifdef __cplusplus
extern "C"{
#endif

#define MAX_FONTS 16
#define MAX_STYLES 16
#define MAX_ENTITIES 512
#define SHORTSTR_LENGTH 32
#define LONGSTR_LENGTH 64
#define MAX_INVENTORY_SLOTS 64  

#define STATE_MAGIC_NUMBER 0xAEEA
#define STATE_PERSISTENT_MAGIC_NUMBER 0xABBC

typedef enum {
    ONE_MINUTE = 1,
    TWO_MINUTES = 2,
    THREE_MINUTES = 3,
    FOUR_MINUTES = 4,
    FIVE_MINUTES = 5
} matchduration_t;

typedef enum {
    FASTEST = 0,
    DEFAULT = 1,
    NICEST = 2,
} graphicssetting_t;

typedef struct gamestate_s{
    unsigned short magicnumber;
    struct{
        bool bgmusic_playing;
        bool sound_playing;
        char bgmusic_name[SHORTSTR_LENGTH];
        char sound_name[SHORTSTR_LENGTH];
        float bgmusic_vol;
        float sound_vol;

        int transitionstate;
        float transitiontime;
        float transitiontimemax;
        bool loopingmusic;
    } audio;

    struct{
        struct {
            uint16_t flags;
        } entities[MAX_ENTITIES];

        char   playername[SHORTSTR_LENGTH];
        char   levelname[LONGSTR_LENGTH];
        fm_vec3_t playerpos;
        fm_vec2_t playerrot;
        
        struct{
            matchduration_t duration;
            graphicssetting_t graphics;
            int vibration;
            float deadzone;
        } settings;

        struct{
            uint8_t item_id;
            uint8_t item_count;
        } inventory[MAX_INVENTORY_SLOTS];

        uint32_t journalscollectedbitflag;
    } game;

    struct{
        bool debug;
    } scripts;

} gamestate_t;
static_assert(sizeof(gamestate_t) <= 2048, "gamestate exceeds EEPROM size");

typedef enum{
    SAVE_NONE = 0,
    SAVE_MANUALSAVE
} saveenum_t;

typedef struct{
    unsigned short        magicnumber;
    uint64_t     global_game_state; // game incomplete, complete, broken etc.
    saveenum_t   lastsavetype;
    bool         manualsaved;
    char         curlang[SHORTSTR_LENGTH]; // "en" - english, etc.
} gamestate_persistent_t;

typedef struct gamestatus_s{
    double currenttime;
    double realtime;
    double deltatime;
    double deltarealtime;
    double fixeddeltatime;
    double fixedtime;
    double fixedframerate;
    uint64_t framenumber;

    double gamespeed;
    bool   paused;
    bool   fastgraphics;

    char   startlevelname[LONGSTR_LENGTH];

    struct {
        char imagesfolder[SHORTSTR_LENGTH];
        char fontfolder[SHORTSTR_LENGTH];
        char bgmfolder[SHORTSTR_LENGTH];
        char sfxfolder[SHORTSTR_LENGTH];
        char scriptsfolder[SHORTSTR_LENGTH];
        char moviesfolder[SHORTSTR_LENGTH];
        char modelsfolder[SHORTSTR_LENGTH];
    } data;

    struct{
        int mainfont;
        int mainfontstyle;
        int mainfontselected;
        int titlefont;
        int titlefontstyle;
        int subtitlefont;
        int subtitlefontstyle;

        struct{
            rdpq_font_t* font;
            rdpq_fontstyle_t styles[MAX_STYLES];
        } fonts[MAX_FONTS];

        int fontcount;
    } fonts;

    gamestate_t state;
    gamestate_persistent_t state_persistent;

    float statetime;
} gamestatus_t;

/// @brief Global game state, includes a persistent state, a state for the game, state of the engine's datapoints
extern gamestatus_t gamestatus;

#define CURRENT_TIME            gamestatus.currenttime
#define CURRENT_TIME_REAL       gamestatus.realtime

#define GAMESPEED               gamestatus.gamespeed
#define GAME_PAUSED             gamestatus.paused

#define DELTA_TIME              gamestatus.deltatime
#define DELTA_TIME_REAL         gamestatus.deltarealtime

#define DELTA_TIME_FIXED        gamestatus.fixeddeltatime
#define CURRENT_TIME_FIXED      gamestatus.fixedtime

#define FRAME_NUMBER            gamestatus.framenumber

/// Init the global game state to 0
void timesys_init();


void timesys_update();

#ifdef __cplusplus
}
#endif

#endif