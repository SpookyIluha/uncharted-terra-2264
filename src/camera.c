#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "camera.h"
#include "utils.h"

void fm_vec3_dir_from_euler(fm_vec3_t *out, const fm_vec3_t *euler) {
    out->z = -cos(euler->y)*cos(euler->x);
    out->x = -sin(euler->y)*cos(euler->x);
    out->y = sin(euler->x);
}

void fm_vec3_euler_from_dir(fm_vec3_t *out, const fm_vec3_t *dir) {
    // Normalize the direction vector first
    fm_vec3_t normalized = *dir;
    float len = fm_vec3_len(&normalized);
    if (len > 0.0001f) {
        fm_vec3_norm(&normalized, &normalized);
    } else {
        // Default direction if vector is too small
        normalized = (fm_vec3_t){{0, 0, -1}};
    }
    
    // Pitch (x rotation): asin(y)
    out->x = asinf(normalized.y);
    
    // Yaw (y rotation): atan2(x, z)
    // From fm_vec3_dir_from_euler: x = -sin(y)*cos(x), z = -cos(y)*cos(x)
    // So tan(y) = x/z, therefore y = atan2(x, z)
    out->y = atan2f(normalized.x, normalized.z);
    
    // Roll (z rotation) is not used in this camera system
    out->z = 0;
}

void camera_transform(camera_t *camera, T3DViewport* viewport)
{
    fm_vec3_t forward; 
    fm_vec3_dir_from_euler(&forward, &camera->rotation);
    fm_vec3_lerp(&camera->camTarget_off, &camera->camTarget_off, &forward, 0.2f);
    fm_vec3_add(&forward, &camera->position, &camera->camTarget_off);
    t3d_viewport_set_perspective(viewport, T3D_DEG_TO_RAD(camera->FOV), camera->aspect_ratio, camera->near_plane, camera->far_plane);
    t3d_viewport_look_at(viewport, (T3DVec3*)&camera->position, (T3DVec3*)&forward, &(T3DVec3){{0,1,0}});
}