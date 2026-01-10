#ifndef SUBTITLES_H
#define SUBTITLES_H

#include <functional>
#include <vector>
#include <string>

#define SUBTITLES_MAX_LENGTH 256
#define SUBTITLES_HEIGHT 100
#define SUBTITLES_START_Y 380
#define CHARACTERS_PER_SEC_DISPLAY 20

void subtitles_init();

void subtitles_update();

void subtitles_add(const char* text, float duration, char buttonsprite);

void subtitles_draw();

void subtitles_free();

#endif