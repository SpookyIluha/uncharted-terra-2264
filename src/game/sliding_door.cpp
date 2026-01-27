#include "sliding_door.h"
#include "engine_command.h"
#include "level.h"
#include "utils.h"

SlidingDoor::SlidingDoor(const std::string& name, int id)
    : Entity(name, SLIDING_DOOR_TYPE_NAME, id), 
      direction({0, 0, 0}), move_distance(0), speed(0), 
      state(CLOSED), current_progress(0.0f) {

    register_console_command("open", [this](const std::vector<std::string>& args) {
        if (state == CLOSED || state == CLOSING) {
            state = OPENING;
        }
    });

    register_console_command("close", [this](const std::vector<std::string>& args) {
        if (state == OPEN || state == OPENING) {
            state = CLOSING;
        }
    });

    register_console_command("toggle", [this](const std::vector<std::string>& args) {
        if (state == OPEN || state == OPENING) {
            state = CLOSING;
        }
        else if (state == CLOSED || state == CLOSING) {
            state = OPENING;
        }
    });
}

void SlidingDoor::load_from_ini(const tortellini::ini::section& section) {
    enabled = section["enabled"] | true;
    model_path = section["model"] | "";
    collision_associated_id = section["collision_id"] | -1;
    
    std::string dir_str = section["direction"] | "0 0 0";
    direction = string_to_vec(dir_str.c_str());
    fm_vec3_norm(&direction, &direction); // Ensure normalized
    
    move_distance = section["distance"] | 0.0f;
    speed = section["speed"] | 0.25f;

    if (!model_path.empty()) {
        model.load(filesystem_getfn(DIR_MODEL, model_path.c_str()).c_str());
    }

    state = (section["open"] | false)? OPEN : CLOSED;
    if(state == OPEN)
        current_progress = 1.0f;

    debugf("SlidingDoor '%s': loaded with model '%s', move_distance %.2f, speed %.2f\n",
           get_name().c_str(), model_path.c_str(), move_distance, speed);
}

void SlidingDoor::load_from_eeprom(uint16_t data) {
    bool is_open = (data & SLIDING_DOOR_OPENED_BIT);
    if (is_open) {
        state = OPEN;
        current_progress = 1.0f;
    } else {
        state = CLOSED;
        current_progress = 0.0f;
    }
}

uint16_t SlidingDoor::save_to_eeprom() const {
    // Save target state
    bool is_open = (state == OPEN || state == OPENING);
    return is_open ? SLIDING_DOOR_OPENED_BIT : 0;
}

void SlidingDoor::init() {
    initial_position = transform.position;
    
    // Update position based on current progress (if loaded from eeprom)
    if (current_progress > 0.0f) {
        fm_vec3_t offset;
        fm_vec3_scale(&offset, &direction, move_distance * current_progress);
        fm_vec3_add(&transform.position, &initial_position, &offset);
    }
}

void SlidingDoor::update() {
    if (!enabled) return;

    if (state == OPENING) {
        current_progress += speed * display_get_delta_time();
        if (current_progress >= 1.0f) {
            current_progress = 1.0f;
            state = OPEN;
        }
    } else if (state == CLOSING) {
        current_progress -= speed * display_get_delta_time();
        if (current_progress <= 0.0f) {
            current_progress = 0.0f;
            state = CLOSED;
        }
    }

    if(collision_associated_id != -1)
        currentlevel.aabb_collisions[collision_associated_id].enabled = (state == OPENING || state == OPEN)? false : true;

}

void SlidingDoor::draw() {
    if (enabled && !model_path.empty()) {
        T3DMat4FP* transform_fp = model.get_transform_fp();
        fm_vec3_t position; 
        {
            fm_vec3_t offset;
            fm_vec3_scale(&offset, &direction, move_distance * current_progress);
            fm_vec3_add(&position, &transform.position, &offset);
        }
        fm_vec3_scale(&position, &position, T3D_TOUNITSCALE);
        t3d_mat4fp_from_srt(transform_fp, (float*)&transform.scale, (float*)&transform.rotation, (float*)&position);
        model.draw();
    }
}

Entity* SlidingDoor::create(const std::string& name, int id) {
    return new SlidingDoor(name, id);
}
