#ifndef CAMERA_H
#define CAMERA_H

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    fm_vec3_t position;
    fm_vec3_t rotation;
    fm_vec3_t upvector;
    float FOV;
    float near_plane;
    float far_plane;
    float aspect_ratio;

    T3DVec3 camPos;
    T3DVec3 camTarget;

    T3DVec3 camPos_off;
    T3DVec3 camTarget_off;
} camera_t;

extern void fm_vec3_dir_from_euler(fm_vec3_t *out, const fm_vec3_t *euler);

extern void camera_transform(camera_t *camera, T3DViewport* viewport);

#ifdef __cplusplus
}
#endif


#endif
