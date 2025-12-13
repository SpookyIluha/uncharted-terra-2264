#ifndef TYPES_H
#define TYPES_H

#include <unistd.h>
#include <time.h>
#include <libdragon.h>

// Various misc types

#ifdef __cplusplus
extern "C"{
#endif

typedef enum buffers_s{
    DOUBLE_BUFFERED = 2,
    TRIPLE_BUFFERED = 3,
    QUAD_BUFFERED = 4,
} buffers_t;

typedef struct{
    float x,y;
} pos2d;

#ifdef __cplusplus
}
#endif

#endif