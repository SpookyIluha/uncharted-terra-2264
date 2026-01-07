#ifndef LEVEL_H
#define LEVEL_H
/// Level class functions

#include <vector>
#include <string>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "engine_filesystem.h"
#include "engine_t3dm_wrapper.h"

// world AABB's for collision
typedef struct{
    fm_vec3_t center;
    fm_vec3_t half_extents;
    bool enabled;
} AABB_t;

// struct that links levels together
typedef struct{
    char name[SHORTSTR_LENGTH];
    char destinationlevel[SHORTSTR_LENGTH];
    char destinationname[SHORTSTR_LENGTH];
    bool interact;

    AABB_t collision;
    fm_vec3_t exitposition;
} traversal_t;

void engine_level_init();

void traversal_update();
void traversal_fade_update();
void traversal_fade_draw();

#define MAX_AABB_COLLISIONS 256
#define MAX_TRAVERSALS 8
#define TRAVERSAL_FADE_DURATION 1.0f  // Duration of fade from black in seconds

class Level {
    public:
        char name[SHORTSTR_LENGTH];

        enum eBGType{
            NONE = 0,
            FILL = 1,
            SKYBOX = 2
        };

        eBGType bgtype;
        color_t bgfillcolor;

        bool    fogenabled;
        color_t fogcolor;
        float   fogfardistance;
        float   fogneardistance;

        float   drawdistance;

        struct{
            bool enabled;
            bool bloomenabled;
            float tonemappingaverage;
        } hdr;

        sprite_t* bloomsprite;

        T3DMWModel skyboxmodel;
        T3DMWModel levelmodel;

        AABB_t aabb_collisions[MAX_AABB_COLLISIONS];
        traversal_t traversals[MAX_TRAVERSALS];

        void load(char* levelname);

        Level();
        ~Level();

        void draw();
        void free();

};

extern Level currentlevel;

#endif