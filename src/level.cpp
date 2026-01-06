#include <vector>
#include <string>
#include <fstream>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "tortellini.hh"
#include "engine_filesystem.h"
#include "engine_t3dm_wrapper.h"
#include "utils.h"
#include "level.h"

Level currentlevel;

// string to vec, also +Z to +Y
fm_vec3_t string_to_vec(const std::string& input)
{
    const char* str = input.c_str();
    char* end;

    fm_vec3_t result;
    result.x = std::strtof(str, &end);
    result.y = std::strtof(end, &end);
    result.z = std::strtof(end, &end);

    return result;
}

float exposure = 1.0f;
float average_brightness = 0;

#define RAND_SAMPLE_COUNT  16
uint32_t sampleOffsets[RAND_SAMPLE_COUNT] = {0};

// autoexposure function for HDR lighting, this will control how bright the vertex colors are in range of 0-1 after T&L for the RDP HDR modulation through color combiner
void exposure_set(void* framebuffer){
  if(!framebuffer) return;
  surface_t* frame = (surface_t*) framebuffer;

  // sample points across the screen
  uint64_t *pixels = (uint64_t*)frame->buffer; // get the previous framebuffer (assumed 320x240 RGBA16 format), it shouldn't be cleared for consistency
  uint32_t maxIndex = frame->width * frame->height * 2 / 8;

  if(sampleOffsets[0] == 0) {
    for(int i = 0; i < RAND_SAMPLE_COUNT; i++){
      sampleOffsets[i] = rand() % maxIndex;
    }
  }

  uint64_t brightInt = 0;
  for (int i = 0; i < RAND_SAMPLE_COUNT; ++i) {
    uint64_t pixels4 = pixels[sampleOffsets[i]];
    for(int j = 0; j < 4; j++){
      brightInt += (pixels4 & 0b11111'00000'00000'0) >> 10;
      brightInt += (pixels4 & 0b00000'11111'00000'0) >> 5;
      brightInt += (pixels4 & 0b00000'00000'11111'0);
      pixels4 >>= 16;
    }
  }

  brightInt /= RAND_SAMPLE_COUNT * 4;
  average_brightness = brightInt / (63.0f * 3.0f);

  // exposure bracket uses an overall bias of how the bright the framebuffer is at 0-1 scale
  // eg. if the avegare brightness of the framebuffer is > 0.7, then the exposure needs to go down until
  // the average brightness is 0.7, and the other way around
  if(average_brightness > currentlevel.hdr.tonemappingaverage) {
    exposure -= 0.01f;
  }
  else if(average_brightness < currentlevel.hdr.tonemappingaverage - 0.1f) {
    exposure += 0.01f;
  }

  // min/max exposure levels
  if(exposure > 2) exposure -= 0.05f;
  if(exposure < 0.9f) exposure = 0.9f;
}


void Level::load(char* levelname){
    free();
    // general data
    {
        tortellini::ini ini;
        std::string levelfilename = filesystem_getfn(DIR_SCRIPT, (std::string("levels/") + std::string(levelname) + ".ini").c_str());
        assertf(filesystem_chkexist(levelfilename.c_str()), "Level file %s does not exist", levelfilename.c_str());
        std::ifstream in(levelfilename.c_str());
        in >> ini;

        std::string bgtype_str = ini["General"]["bgtype"] | "none";
        if (bgtype_str == "none") {
            bgtype = NONE;
            debugf("Level %s: background type set to none\n", levelname);
        } else if (bgtype_str == "fill") {
            bgtype = FILL;
            uint32_t color1 = ini["General"]["bgfillcolor"] | 0xFF00FFFF;
            bgfillcolor = color_from_packed32(color1);
            bgfillcolor = color_from_packed16(color_to_packed16(bgfillcolor)); // to have the same value as fogcolor
            debugf("Level %s: background fill color set to %08X\n", levelname, color1);
        } else if (bgtype_str == "skybox") {
            bgtype = SKYBOX;
            std::string skyboxname = ini["General"]["skyboxmodel"] | "";
            debugf("Level %s: background type set to skybox with model %s\n", levelname, skyboxname.c_str());
            assertf(!skyboxname.empty(), "Level %s: skyboxmodel not specified for skybox bgtype", levelname);
            skyboxmodel.load(filesystem_getfn(DIR_MODEL, ("levels/" + skyboxname).c_str()).c_str());
        }

        fogenabled = ini["General"]["fogenabled"] | false;
        fogcolor = color_from_packed32(ini["General"]["fogcolor"] | (uint32_t)0xFF00FFFF);
        fogcolor = color_from_packed16(color_to_packed16(fogcolor)); // to have the same value as fillcolor

        fogfardistance = ini["General"]["fogfardistance"] | 100.0f;
        fogneardistance = ini["General"]["fogneardistance"] | 0.0f;
        drawdistance = ini["General"]["drawdistance"] | 100.0f;

        hdr.enabled = ini["General"]["hdr"] | false;
        hdr.tonemappingaverage = ini["General"]["tonemappingaverage"] | 0.5f;

        debugf("Level %s: fog enabled set to %d\n", levelname, fogenabled);
        debugf("Level %s: fog color set to %08X\n", levelname, fogcolor);
        debugf("Level %s: fog far distance set to %f\n", levelname, fogfardistance);
        debugf("Level %s: fog near distance set to %f\n", levelname, fogneardistance);
        debugf("Level %s: draw distance set to %f\n", levelname, drawdistance);

        std::string levelmodel_str = ini["General"]["levelmodel"] | "";
        assertf(!levelmodel_str.empty(), "Level %s: levelmodel not specified", levelname);
        levelmodel.load(filesystem_getfn(DIR_MODEL, ("levels/" + levelmodel_str).c_str()).c_str());
    }
    // collisions
    {
        tortellini::ini ini;
        std::string collisionfilename = filesystem_getfn(DIR_SCRIPT, (std::string("levels/") + std::string(levelname) + ".collision.ini").c_str());
        assertf(filesystem_chkexist(collisionfilename.c_str()), "Collision file %s does not exist", collisionfilename.c_str());
        std::ifstream in(collisionfilename.c_str());
        in >> ini;

        int enabledcollisioncount = 0;
        for(int i = 0; i < MAX_AABB_COLLISIONS; i++) {
            fm_vec3_t center, half_extents;
            center = string_to_vec(ini["Collision"]["colpos" + std::to_string(i)] | "0 0 0");
            half_extents = string_to_vec(ini["Collision"]["colscl" + std::to_string(i)] | "0 0 0");
            transform_upz_to_upy(&center, &half_extents, &aabb_collisions[i].center, &aabb_collisions[i].half_extents);
            aabb_collisions[i].enabled = ini["Collision"]["colenb" + std::to_string(i)] | false;
            if(aabb_collisions[i].enabled) {
                enabledcollisioncount++;
            }
        }
        debugf("Level %s: %d AABB collisions enabled\n", levelname, enabledcollisioncount);
    }
    // traversals
    {
        tortellini::ini ini;
        std::string traversalfilename = filesystem_getfn(DIR_SCRIPT, (std::string("levels/") + std::string(levelname) + ".traversal.ini").c_str());
        assertf(filesystem_chkexist(traversalfilename.c_str()), "Traversal file %s does not exist", traversalfilename.c_str());
        std::ifstream in(traversalfilename.c_str());
        in >> ini;

        int traversalcount = 0;
        // loop over sections in the ini
        for (auto pair : ini) {
            strcpy(traversals[traversalcount].name, pair.name.c_str());
            strcpy(traversals[traversalcount].destinationlevel, (pair.section["destlevel"] | "").c_str());
            strcpy(traversals[traversalcount].destinationname, (pair.section["destname"] | pair.name).c_str());

            assertf(traversals[traversalcount].destinationlevel, "Traversal %s: destination level not specified", pair.name.c_str());

            fm_vec3_t center, half_extents;
            center = string_to_vec(pair.section["position"] | "0 0 0");
            half_extents = string_to_vec(pair.section["scale"] | "0 0 0");
            transform_upz_to_upy(&center, &half_extents, &traversals[traversalcount].collision.center, &traversals[traversalcount].collision.half_extents);
            traversals[traversalcount].collision.enabled = pair.section["enabled"] | true;
            traversals[traversalcount].interact = pair.section["interact"] | false;

            fm_vec3_t exitposition = string_to_vec(pair.section["exitposition"] | "0 0 0");
            traversals[traversalcount].exitposition = vec_upz_to_upy(&exitposition);

            traversalcount++;
            assertf(traversalcount < MAX_TRAVERSALS, "Level %s: too many traversals defined (%i<%i)", levelname, traversalcount, MAX_TRAVERSALS);
        }
        debugf("Level %s: %d traversals loaded\n", levelname, traversalcount);

    }

}


void Level::draw(){
    surface_t fb = display_get_current_framebuffer();
    if(currentlevel.hdr.enabled) exposure_set(&fb);
    else exposure = 1;

    t3d_fog_set_enabled(false);
    t3d_light_set_ambient(RGBA32(255,255,255,255));
    t3d_light_set_count(0);
    t3d_light_set_exposure(exposure);
    color_t fogcolorexposed = fogcolor;
    fogcolorexposed.r = (uint8_t)fclampr((float)fogcolor.r * exposure, 0.0f, 255.0f);
    fogcolorexposed.g = (uint8_t)fclampr((float)fogcolor.g * exposure, 0.0f, 255.0f);
    fogcolorexposed.b = (uint8_t)fclampr((float)fogcolor.b * exposure, 0.0f, 255.0f);

    color_t bgfillcolorexposed = bgfillcolor;
    bgfillcolorexposed.r = (uint8_t)fclampr((float)bgfillcolor.r * exposure, 0.0f, 255.0f);
    bgfillcolorexposed.g = (uint8_t)fclampr((float)bgfillcolor.g * exposure, 0.0f, 255.0f);
    bgfillcolorexposed.b = (uint8_t)fclampr((float)bgfillcolor.b * exposure, 0.0f, 255.0f);


    switch(bgtype){
        case FILL:
            rdpq_clear(bgfillcolorexposed);
            break;
        case SKYBOX:
            skyboxmodel.draw();
            break;
        default:
            break;
    }
    t3d_fog_set_enabled(fogenabled);
    t3d_fog_set_range(T3D_TOUNITS(fogfardistance), T3D_TOUNITS(fogneardistance));
    rdpq_set_fog_color(fogcolorexposed);
    levelmodel.draw();
}

Level::Level(){
    name[0] = '\0';
    bgtype = NONE;
    bgfillcolor = RGBA32(255,0,255,255);
    fogenabled = true;
    fogcolor = RGBA32(255,0,255,255);
    fogfardistance = 100.0f;
    fogneardistance = 0.0f;
    memset(aabb_collisions, 0, sizeof(aabb_collisions));
    memset(traversals, 0, sizeof(traversals));
}

void Level::free(){
    skyboxmodel.free();
    levelmodel.free();
}

Level::~Level(){
    free();
}