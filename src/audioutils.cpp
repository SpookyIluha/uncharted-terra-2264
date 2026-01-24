#include <libdragon.h>
#include <xm64.h>
#include <string>
#include <string.h>
#include "engine_filesystem.h"
#include "engine_command.h"
#include "audioutils.h"
#include "engine_gamestatus.h"

wav64_t* bgmusic;
xm64player_t* bgmusic_xm;
bool bgmusic_is_xm;

typedef struct {
    wav64_t* wav;
    bool in_use;
    bool looping;
    int channel;
    char name[SHORTSTR_LENGTH];
} sound_slot_t;

static sound_slot_t sound_slots[SOUND_SLOT_COUNT];
static int sound_next_slot;


static wav64_t prewarm_music_wav[8];
static xm64player_t prewarm_music_xm[8];
static wav64_t prewarm_sfx[64];
static char prewarm_music_wav_names[8][SHORTSTR_LENGTH];
static char prewarm_music_xm_names[8][SHORTSTR_LENGTH];
static char prewarm_sfx_names[64][SHORTSTR_LENGTH];
static int prewarm_music_wav_count = 0;
static int prewarm_music_xm_count = 0;
static int prewarm_sfx_count = 0;

int audio_prewarm_all_sounds_callback(const char *fn, dir_t *dir, void *data){
    if(prewarm_sfx_count >= 64) return DIR_WALK_CONTINUE;
    char nameonly[128] = {0};
    char nameres[32] = {0};
    strcpy(nameonly, strrchr(fn, '/') + 1);
    *strrchr(nameonly, '.') = '\0';
    strcpy(nameres, nameonly);
    snprintf(prewarm_sfx_names[prewarm_sfx_count], SHORTSTR_LENGTH, "%s", nameres);
    wav64_open(&prewarm_sfx[prewarm_sfx_count], fn);
    debugf("Prewarm sound: %s\n", prewarm_sfx_names[prewarm_sfx_count]);
    prewarm_sfx_count++;
    return DIR_WALK_CONTINUE;
}

int audio_prewarm_all_music_callback(const char *fn, dir_t *dir, void *data){
    if(prewarm_music_wav_count >= 8) return DIR_WALK_CONTINUE;
    char nameonly[128] = {0};
    char nameres[32] = {0};
    strcpy(nameonly, strrchr(fn, '/') + 1);
    *strrchr(nameonly, '.') = '\0';
    strcpy(nameres, nameonly);
    snprintf(prewarm_music_wav_names[prewarm_music_wav_count], SHORTSTR_LENGTH, "%s", nameres);
    wav64_open(&prewarm_music_wav[prewarm_music_wav_count], fn);
    debugf("Prewarm music: %s\n", prewarm_music_wav_names[prewarm_music_wav_count]);
    prewarm_music_wav_count++;
    return DIR_WALK_CONTINUE;
}

int audio_prewarm_all_xmmusic_callback(const char *fn, dir_t *dir, void *data){
    if(prewarm_music_xm_count >= 8) return DIR_WALK_CONTINUE;
    char nameonly[128] = {0};
    char nameres[32] = {0};
    strcpy(nameonly, strrchr(fn, '/') + 1);
    *strrchr(nameonly, '.') = '\0';
    strcpy(nameres, nameonly);
    snprintf(prewarm_music_xm_names[prewarm_music_xm_count], SHORTSTR_LENGTH, "%s", nameres);
    xm64player_open(&prewarm_music_xm[prewarm_music_xm_count], fn);
    debugf("Prewarm XM music: %s\n", prewarm_music_xm_names[prewarm_music_xm_count]);
    prewarm_music_xm_count++;
    return DIR_WALK_CONTINUE;
}


void bgm_play_console_command(std::vector<std::string> args) {
    if(args.size() < 1) {
        debugf("Usage: sound_play <soundname> <looping>\n");
        return;
    }
    bool looping = false;
    if(args.size() > 1)
        looping = args[1] == "loop";
    bgm_play(args[0].c_str(), looping, 2.0f);
}

void bgm_stop_console_command(std::vector<std::string> args) {
    bgm_stop(2.0f);
}

void sound_play_console_command(std::vector<std::string> args) {
    if(args.size() < 1) {
        debugf("Usage: sound_play <soundname> <looping>\n");
        return;
    }
    bool looping = false;
    if(args.size() > 1)
        looping = args[1] == "loop";
    sound_play(args[0].c_str(), looping);
}

void sound_stop_console_command(std::vector<std::string> args) {
    if(args.size() < 1) {
        debugf("Usage: sound_stop <soundname>\n");
        return;
    }
    sound_stop_looping(args[0].c_str());
}

void audio_console_commands_init(){
    register_game_console_command("sound_play", sound_play_console_command);
    register_game_console_command("sound_stop", sound_stop_console_command);
    register_game_console_command("bgm_play", bgm_play_console_command);
    register_game_console_command("bgm_stop", bgm_stop_console_command);
}



void audio_prewarm_all(){
    dir_glob("**/*.wav64", (std::string("rom:/") + gamestatus.data.bgmfolder + "/").c_str(), audio_prewarm_all_music_callback, NULL);
    dir_glob("**/*.xm64",  (std::string("rom:/") + gamestatus.data.bgmfolder + "/").c_str(), audio_prewarm_all_xmmusic_callback, NULL);
    dir_glob("**/*.wav64", (std::string("rom:/") + gamestatus.data.sfxfolder + "/").c_str(), audio_prewarm_all_sounds_callback, NULL);
    for(int i = 0; i < AUDIO_MAX_CHANNELS ; i+=2){
        wav64_play(&prewarm_sfx[0], i);
        mixer_try_play();
        rspq_wait();
        mixer_ch_stop(i);
        mixer_ch_stop(i+1);
    }
}

int audio_find_sound(const char* name){
    int index = 0;
    while(index < prewarm_sfx_count && strcmp(prewarm_sfx_names[index], name)) index++;
    if(index >= prewarm_sfx_count) assertf(0, "Sound not found %s", name);
    return index;
}

int audio_find_music(const char* name){
    int index = 0;
    while(index < prewarm_music_wav_count && strcmp(prewarm_music_wav_names[index], name)) index++;
    if(index >= prewarm_music_wav_count) {
        index = 0;
        while(index < prewarm_music_xm_count && strcmp(prewarm_music_xm_names[index], name)) index++;
        if(index >= prewarm_music_xm_count) assertf(0, "Music not found %s", name);
    }
    return index;
}



static void bgm_close_current(){   
    if(bgmusic_is_xm){
        xm64player_stop(bgmusic_xm);
        xm64player_seek(bgmusic_xm, 0, 0, 0);
    } else {
        mixer_ch_stop(AUDIO_CHANNEL_MUSIC);
        mixer_ch_stop(AUDIO_CHANNEL_MUSIC + 1);
    }
    for(int i = AUDIO_CHANNEL_MUSIC; i < AUDIO_CHANNEL_MUSIC + AUDIO_CHANNEL_MUSIC_CHANNELS; i++){
        mixer_ch_stop(i);
    }
    rspq_wait();
    gamestatus.state.audio.bgmusic_playing = false;
}

static void bgm_open_new(const char* name, bool loop){
    int xm_idx = 0;
    while(xm_idx < prewarm_music_xm_count && strcmp(prewarm_music_xm_names[xm_idx], name)) xm_idx++;
    if(xm_idx < prewarm_music_xm_count){
        bgmusic_xm = &prewarm_music_xm[xm_idx];
        debugf("Playing XM music %s on channel %d\n", name, AUDIO_CHANNEL_MUSIC);
        debugf("XM channel count: %d\n", xm64player_num_channels(bgmusic_xm));
        xm64player_set_loop(bgmusic_xm, loop);
        xm64player_play(bgmusic_xm, AUDIO_CHANNEL_MUSIC);
        bgmusic_is_xm = true;
        gamestatus.state.audio.bgmusic_playing = true;
    } else {
        int wav_idx = 0;
        while(wav_idx < prewarm_music_wav_count && strcmp(prewarm_music_wav_names[wav_idx], name)) wav_idx++;
        assertf(wav_idx < prewarm_music_wav_count, "Music not prewarmed: %s", name);
        bgmusic = &prewarm_music_wav[wav_idx];
        debugf("Playing music %s on channel %d\n", name, AUDIO_CHANNEL_MUSIC);
        wav64_set_loop(bgmusic, loop);
        wav64_play(bgmusic, AUDIO_CHANNEL_MUSIC);
        bgmusic_is_xm = false;
        gamestatus.state.audio.bgmusic_playing = true;
    }
}

void audioutils_mixer_update(){
    float volume = gamestatus.state.audio.bgmusic_vol;
    mixer_try_play();
    if(gamestatus.state.audio.transitionstate == 1 && gamestatus.state.audio.transitiontime > 0){
        gamestatus.state.audio.transitiontime -= display_get_delta_time();
        if(gamestatus.state.audio.transitiontime <= 0) {
            gamestatus.state.audio.transitionstate = 2;
            if(gamestatus.state.audio.bgmusic_playing){
                bgm_close_current();
            }
            if(gamestatus.state.audio.bgmusic_name[0]){
                bgm_open_new(gamestatus.state.audio.bgmusic_name, gamestatus.state.audio.loopingmusic);
            }
        }
        volume *= (gamestatus.state.audio.transitiontime / gamestatus.state.audio.transitiontimemax);
    }
    if(gamestatus.state.audio.transitionstate == 2 && gamestatus.state.audio.transitiontime < gamestatus.state.audio.transitiontimemax){
        gamestatus.state.audio.transitiontime += display_get_delta_time();
        volume *= (gamestatus.state.audio.transitiontime / gamestatus.state.audio.transitiontimemax);
    }
    if(bgmusic_is_xm && gamestatus.state.audio.bgmusic_playing){
        xm64player_set_vol(bgmusic_xm, volume);
    } else {
        mixer_ch_set_vol(AUDIO_CHANNEL_MUSIC, volume, volume);
    }
}

void bgm_hardplay(const char* name, bool loop, float transition){
    bgm_close_current();
    gamestatus.state.audio.loopingmusic = loop;
    gamestatus.state.audio.transitionstate = 0;
    gamestatus.state.audio.transitiontime = 0;
    gamestatus.state.audio.transitiontimemax = 1;
    bgm_open_new(name, gamestatus.state.audio.loopingmusic);
    strcpy(gamestatus.state.audio.bgmusic_name, name);
}

void bgm_play(const char* name, bool loop, float transition){
    if(transition == 0) { bgm_hardplay(name, loop, transition); return;}
    bgm_stop(0);
    gamestatus.state.audio.loopingmusic = loop;
    gamestatus.state.audio.transitionstate = 1;
    gamestatus.state.audio.transitiontime = transition;
    gamestatus.state.audio.transitiontimemax = transition;
    strcpy(gamestatus.state.audio.bgmusic_name, name);
}

void bgm_hardstop(){
    gamestatus.state.audio.bgmusic_playing = false;
    gamestatus.state.audio.transitionstate = 0;
    gamestatus.state.audio.transitiontime = 0.1;
    gamestatus.state.audio.transitiontimemax = 0.1;
    gamestatus.state.audio.bgmusic_name[0] = 0;
    bgm_close_current();
}

void bgm_stop(float transition){
    if(transition == 0) { bgm_hardstop(); return;}
    gamestatus.state.audio.transitionstate = 1;
    gamestatus.state.audio.transitiontime = 1;
    gamestatus.state.audio.transitiontimemax = 1;
    gamestatus.state.audio.bgmusic_name[0] = 0;
}

static int sound_slot_find_for_play(bool loop){
    int start = sound_next_slot;
    for(int i = 0; i < SOUND_SLOT_COUNT; i++){
        int idx = (start + i) % SOUND_SLOT_COUNT;
        if(sound_slots[idx].looping) continue;
        return idx;
    }
    return -1;
}

void sound_play(const char* name, bool loop){
    int slot = sound_slot_find_for_play(loop);
    if(slot < 0) return;
    sound_slot_t* s = &sound_slots[slot];
    if(s->in_use){
        mixer_ch_stop(s->channel);
    }
    int base_channel = AUDIO_CHANNEL_SOUND_BASE + (slot * 2);
    int sfx_idx = 0;
    while(sfx_idx < prewarm_sfx_count && strcmp(prewarm_sfx_names[sfx_idx], name)) sfx_idx++;
    assertf(sfx_idx < prewarm_sfx_count, "Sound not prewarmed: %s", name);
    s->wav = &prewarm_sfx[sfx_idx];
    debugf("Playing sound %s on channel %d\n", name, base_channel);
    wav64_set_loop(s->wav, loop);
    wav64_play(s->wav, base_channel);
    s->in_use = true;
    s->looping = loop;
    s->channel = base_channel;
    strncpy(s->name, name, SHORTSTR_LENGTH - 1);
    s->name[SHORTSTR_LENGTH - 1] = 0;
    sound_next_slot = (slot + 1) % SOUND_SLOT_COUNT;
    gamestatus.state.audio.sound_playing = true;
    strcpy(gamestatus.state.audio.sound_name, name);
}

void sound_stop(){
    for(int i = 0; i < SOUND_SLOT_COUNT; i++){
        sound_slot_t* s = &sound_slots[i];
        if(!s->in_use) continue;
        mixer_ch_stop(s->channel);
        s->in_use = false;
        s->looping = false;
        s->name[0] = 0;
    }
    gamestatus.state.audio.sound_playing = false;
    gamestatus.state.audio.sound_name[0] = 0;
}

void sound_stop_looping(const char* name){
    for(int i = 0; i < SOUND_SLOT_COUNT; i++){
        sound_slot_t* s = &sound_slots[i];
        if(!s->in_use) continue;
        if(!s->looping) continue;
        if(strcmp(s->name, name) != 0) continue;
        mixer_ch_stop(s->channel);
        s->in_use = false;
        s->looping = false;
        s->name[0] = 0;
    }
}

void music_volume(float vol){
    gamestatus.state.audio.bgmusic_vol = vol;
}

void sound_volume(float vol){
    for(int i = 0; i < SOUND_SLOT_COUNT; i++){
        int ch = AUDIO_CHANNEL_SOUND_BASE + (i * 2);
        mixer_ch_set_vol(ch, vol, vol);
    }
    gamestatus.state.audio.sound_vol = vol;
}

float music_volume_get() {return gamestatus.state.audio.bgmusic_vol;};

float sound_volume_get() {return gamestatus.state.audio.sound_vol;};
