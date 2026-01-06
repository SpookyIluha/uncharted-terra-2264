#ifndef INTRO_H
#define INTRO_H

#ifdef __cplusplus
extern "C"{
#endif

/// Libragon intro entrypoint
extern void libdragon_logo();
extern void movie_play(char* moviefilename, char* audiofilename, float movie_fps);
extern void game_logo();
#ifdef __cplusplus
}
#endif

#endif