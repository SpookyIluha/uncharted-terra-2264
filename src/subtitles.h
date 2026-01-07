#ifndef SUBTITLES_H
#define SUBTITLES_H

#include <functional>
#include <vector>
#include <string>

#define SUBTITLES_MAX_LENGTH 256
#define SUBTITLES_HEIGHT 100
#define SUBTITLES_START_Y 380

void subtitles_init();

void subtitles_update();

void subtitles_add(char* text, float duration);

void subtitles_draw();

#endif