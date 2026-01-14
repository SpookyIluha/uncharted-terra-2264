#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include <t3d/t3dmodel.h>
#include "tortellini.hh"
#include "engine_filesystem.h"
#include "engine_t3dm_wrapper.h"
#include "engine_gamestatus.h"
#include "utils.h"
#include "level.h"
#include "player.h"
#include "camera.h"
#include "game/entity.h"

Level currentlevel;

void change_level_console_command(std::vector<std::string> args);

void engine_level_init(){
    register_game_console_command("change_level", change_level_console_command);
}

// Fade from black state for level transitions
static float traversal_fade_time = 1.5f;
static bool traversal_fade_active = true;

uint32_t state = 777;

char custom_rand()
{
   state = state * 1664525 + 1013904223;
   return state >> 24;
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

#define BLOOM_SIZE 48

void bloom_draw(void* framebuffer, sprite_t* bloomsprite){
    if(!framebuffer) return;
    surface_t* frame = (surface_t*) framebuffer;
    surface_t bloom = sprite_get_pixels(bloomsprite);

    rdpq_sync_pipe();
    rdpq_set_mode_standard();
    rdpq_mode_filter(FILTER_BILINEAR);
    temporal_dither(FRAME_NUMBER);
    rdpq_tex_upload(TILE0, &bloom, NULL);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM),(PRIM,0,TEX0,0)));
    rdpq_mode_alphacompare(15);

    state = 777;
    // sample points across the screen
    uint16_t *pixels = (uint16_t*)frame->buffer;

    int resx, resy;
    resx = display_get_width();
    resy = display_get_height();
    int samples_x, samples_y;
    samples_x = resx / BLOOM_SIZE;
    samples_y = resy / BLOOM_SIZE;

    for(int y = 0; y < samples_y; y++){
        for(int x = 0; x < samples_x; x++){
            if(custom_rand() & 0x1) continue;
            int offsetx = (custom_rand() & 0xF) - 7;
            int offsety = (custom_rand() & 0xF) - 7;

            int alignedx = ((x * BLOOM_SIZE) + offsetx) & 0b11111'11111'11100'0;

            uint64_t pixels4 = *(uint64_t*)&pixels[(alignedx + (y * BLOOM_SIZE + offsety) * resx)];
            int brightIntR = 0, brightIntG = 0, brightIntB = 0;

            for(int j = 0; j < 4; j++){
                brightIntR += (pixels4 & 0b11111'00000'00000'0) >> 10;
                brightIntG += (pixels4 & 0b00000'11111'00000'0) >> 5;
                brightIntB += (pixels4 & 0b00000'00000'11111'0);
                pixels4 >>= 16;
            }

            color_t col = RGBA32(brightIntR, brightIntG, brightIntB, 0);
            int brightness = maxi(col.r, maxi(col.g, col.b));
            brightness -= 222;
            brightness = maxi(0, brightness);

            if(brightness == 0) 
                continue;

            brightness *= 8;

            col.a = brightness;
            float alpha = (float)brightness * (1.0/200.0);

            rdpq_set_prim_color(col);
            int boxx = x * BLOOM_SIZE + offsetx - (BLOOM_SIZE * alpha);
            int boxy = y * BLOOM_SIZE + offsety - (BLOOM_SIZE * alpha);
            rdpq_texture_rectangle_scaled(TILE0, boxx, boxy, boxx + BLOOM_SIZE * 2 * alpha, boxy + BLOOM_SIZE * 2 * alpha, 0, 0, 16,16);
        }
    }
}

void Level::load(char* levelname){
    debugf("Loading level '%s'...\n", levelname);
    free();
    strncpy(name, levelname, SHORTSTR_LENGTH - 1);
    name[SHORTSTR_LENGTH - 1] = '\0';
    
    // Clear collisions and traversals before loading new ones
    memset(aabb_collisions, 0, sizeof(aabb_collisions));
    memset(traversals, 0, sizeof(traversals));
    
    bloomsprite = sprite_load("rom:/textures/effects/bloom.i4.sprite");
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
            debugf("Level %s: background fill color set to %08lX\n", levelname, color1);
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
        hdr.bloomenabled = ini["General"]["bloom"] | false;
        hdr.tonemappingaverage = ini["General"]["tonemappingaverage"] | 0.5f;

        debugf("Level %s: fog enabled set to %d\n", levelname, fogenabled);
        debugf("Level %s: fog color set to %08lX\n", levelname, fogcolor);
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
            aabb_collisions[i].center = string_to_vec((ini["Collision"]["colpos" + std::to_string(i)] | "0 0 0").c_str());
            aabb_collisions[i].half_extents = string_to_vec((ini["Collision"]["colscl" + std::to_string(i)] | "0 0 0").c_str());
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
            traversals[traversalcount].collision.center = string_to_vec((pair.section["position"] | "0 0 0").c_str());
            traversals[traversalcount].collision.half_extents = string_to_vec((pair.section["scale"] | "0 0 0").c_str());
            traversals[traversalcount].collision.enabled = pair.section["enabled"] | true;
            traversals[traversalcount].interact = pair.section["interact"] | false;

            traversals[traversalcount].exitposition = string_to_vec((pair.section["exitposition"] | "0 0 0").c_str());

            traversalcount++;
            assertf(traversalcount < MAX_TRAVERSALS, "Level %s: too many traversals defined (%i<%i)", levelname, traversalcount, MAX_TRAVERSALS);
        }
        debugf("Level %s: %d traversals loaded\n", levelname, traversalcount);

    }
    
    // Load entities
    {
        EntitySystem::load_entities_from_ini(levelname);
    }

    strcpy(gamestatus.state.game.levelname, levelname);
    
    debugf("Level '%s' loaded successfully\n", levelname);
    traversal_fade_time = TRAVERSAL_FADE_DURATION;
    traversal_fade_active = true;
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
    if(hdr.bloomenabled && bloomsprite) {
        bloom_draw(&fb, bloomsprite);
    }
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
    if(bloomsprite) {
        sprite_free(bloomsprite);
        bloomsprite = nullptr;
    }
    EntitySystem::clear_all(); // Clear all entities when level is freed
}

void Level::save_eeprom(){
    EntitySystem::save_all_to_eeprom();
}

Level::~Level(){
    free();
}


// Helper function to find a traversal by name in a level
static traversal_t* find_traversal_by_name(Level* level, const char* name) {
    debugf("Finding traversal '%s' in level '%s'\n", name, level->name);
    for(int i = 0; i < MAX_TRAVERSALS; i++) {
        if(level->traversals[i].collision.enabled && strcmp(level->traversals[i].name, name) == 0) {
            debugf("Found traversal '%s' at index %d\n", name, i);
            return &level->traversals[i];
        }
    }
    assertf(false, "Traversal '%s' not found in level '%s'\n", name, level->name);
    return nullptr;
}

// Helper function to change level and position player at destination
static void change_level(const char* levelname, const char* destinationname) {
    rspq_wait();
    currentlevel.save_eeprom();
    // Fade out
    for(int i = 0; i < 2; i++){
        rdpq_attach(display_get(), NULL);
        rdpq_disable_interlaced();
        rdpq_clear(RGBA32(0,0,0,255));
        rdpq_detach_show();
    }

    extern player_t player;
    
    char destinationnamebuffer[SHORTSTR_LENGTH];
    strcpy(destinationnamebuffer, destinationname);

    char levelnamebuffer[SHORTSTR_LENGTH];
    strcpy(levelnamebuffer, levelname);

    debugf("Changing level: '%s' -> '%s' (destination: '%s')\n", 
           currentlevel.name, levelnamebuffer, destinationnamebuffer);
    
    // Load the new level
    currentlevel.load(levelnamebuffer);
    
    // Find the destination traversal point
    traversal_t* dest_traversal = find_traversal_by_name(&currentlevel, destinationnamebuffer);
    
    assertf(dest_traversal != nullptr, "Destination traversal '%s' not found in level '%s'", destinationnamebuffer, levelnamebuffer);
        
    // Set player position to the exit position of the destination traversal
    fm_vec3_t old_pos = player.position;
    player_init();
    player.position = dest_traversal->exitposition;
    
    // Calculate direction from traversal origin to exit position
    fm_vec3_t direction;
    fm_vec3_sub(&direction, &dest_traversal->collision.center, &dest_traversal->exitposition);
    direction.y = 0;
    
    // Set player rotation to face the direction from origin to exit (horizontal only, pitch = 0)
    fm_vec3_euler_from_dir(&player.camera.rotation, &direction);
            
    // Immediately update camera target offset to match new rotation (no lerp delay)
    fm_vec3_dir_from_euler(&player.camera.camTarget_off, &player.camera.rotation);
    
    // Reset player velocity to prevent carrying momentum between levels
    player.velocity = (fm_vec3_t){{0, 0, 0}};
    
    // Update camera far plane to match new level's draw distance
    player.camera.far_plane = T3D_TOUNITS(currentlevel.drawdistance);
    
    debugf("Level change complete\n");
    debugf("Player position: (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)\n",
           old_pos.x, old_pos.y, old_pos.z,
           player.position.x, player.position.y, player.position.z);
    
    // Start fade from black
    traversal_fade_time = TRAVERSAL_FADE_DURATION;
    traversal_fade_active = true;
}

void change_level_console_command(std::vector<std::string> args) {
    if(args.size() < 2) {
        debugf("Usage: change_level <levelname> <destinationname>\n");
        return;
    }
    change_level(args[0].c_str(), args[1].c_str());
}

void traversal_update() {
    extern player_t player;
    
    // Check all traversals in the current level
    for(int i = 0; i < MAX_TRAVERSALS; i++) {
        traversal_t* traversal = &currentlevel.traversals[i];
        
        // Skip if traversal is not enabled
        if(!traversal->collision.enabled) {
            continue;
        }
        
        // Check if player is colliding with this traversal zone
        fm_vec3_t dummy_out;
        bool is_colliding = collideAABB(
            &player.position,
            0,
            &traversal->collision.center,
            &traversal->collision.half_extents,
            &dummy_out
        );
        
        if(is_colliding) {
            debugf("Traversal collision detected: '%s' (level: '%s', dest: '%s')\n",
                   traversal->name, currentlevel.name, traversal->destinationlevel);
            
            // Check if this is an interactive traversal
            if(traversal->interact) {
                // For interactive traversals, check for button press
                joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
                if(pressed.a) {
                    debugf("Interactive traversal '%s' triggered by A button press\n", traversal->name);
                    // Player pressed A, trigger traversal
                    change_level(traversal->destinationlevel, traversal->destinationname);
                    break; // Only process one traversal per frame
                } else {
                    // Player is in traversal zone but hasn't pressed A yet
                    static int last_message_frame = -1;
                    int current_frame = FRAME_NUMBER;
                    // Only print this message every 60 frames to avoid spam
                    if(current_frame - last_message_frame > 60) {
                        debugf("In interactive traversal zone '%s' - Press A to enter '%s'\n",
                               traversal->name, traversal->destinationlevel);
                        last_message_frame = current_frame;
                    }
                }
            } else {
                debugf("Automatic traversal '%s' triggered\n", traversal->name);
                // Automatic traversal - trigger immediately
                change_level(traversal->destinationlevel, traversal->destinationname);
                break; // Only process one traversal per frame
            }
        }
    }
}

void traversal_fade_update() {
    if(traversal_fade_active && traversal_fade_time > 0.0f) {
        traversal_fade_time -= DELTA_TIME;
        if(traversal_fade_time <= 0.0f) {
            traversal_fade_time = 0.0f;
            traversal_fade_active = false;
        }
    }
}

void traversal_fade_draw() {
    if(!traversal_fade_active || traversal_fade_time <= 0.0f) {
        return;
    }
    
    // Calculate fade alpha (1.0 = fully black, 0.0 = transparent)
    float fade_alpha = traversal_fade_time / TRAVERSAL_FADE_DURATION;
    fade_alpha = fclampr(fade_alpha, 0.0f, 1.0f);
    
    // Get screen dimensions
    int screen_width = display_get_width();
    int screen_height = display_get_height();
    
    // Draw black overlay with fade
    uint8_t alpha = (uint8_t)(fade_alpha * 255.0f);
    color_t fade_color = RGBA32(0, 0, 0, alpha);
    
    rdpq_set_mode_standard();
    rdpq_mode_zbuf(false, false);
    temporal_dither(FRAME_NUMBER);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER1((PRIM,0,0,0),(0,0,0,PRIM)));
    rdpq_set_prim_color(fade_color);
    rdpq_fill_rectangle(0, 0, screen_width, screen_height);
}