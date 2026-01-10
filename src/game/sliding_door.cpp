#include "sliding_door.h"
#include "engine_command.h"
#include "level.h" // for string_to_vec

SlidingDoor::SlidingDoor(const std::string& name, int id)
    : Entity(name, SLIDING_DOOR_TYPE_NAME, id), 
      direction({0, 0, 0}), move_distance(0), speed(0), 
      state(CLOSED), current_progress(0.0f) {

    register_console_command("open", [this](const std::vector<std::string>& args) {
        if (state == CLOSED || state == CLOSING) {
            state = OPENING;
            // debugf("SlidingDoor '%s': Opening\n", get_name().c_str());
        }
    });

    register_console_command("close", [this](const std::vector<std::string>& args) {
        if (state == OPEN || state == OPENING) {
            state = CLOSING;
            // debugf("SlidingDoor '%s': Closing\n", get_name().c_str());
        }
    });
}

void SlidingDoor::load_from_ini(const tortellini::ini::section& section) {
    enabled = section["enabled"] | true;
    model_path = section["model"] | "";
    
    std::string dir_str = section["direction"] | "0 0 0";
    direction = string_to_vec(dir_str);
    fm_vec3_norm(&direction, &direction); // Ensure normalized
    
    move_distance = section["distance"] | 0.0f;
    speed = section["speed"] | 1.0f;

    if (!model_path.empty()) {
        model.load(filesystem_getfn(DIR_MODEL, model_path.c_str()).c_str());
    }
}

void SlidingDoor::load_from_eeprom(uint16_t data) {
    // Bit 0: 0 = Closed/Closing, 1 = Open/Opening
    // This is a simplification, maybe store actual state if needed
    // For now, let's assume if it was open, it stays open?
    // Or just ignore persistence for door state if not required.
    // The user didn't specify persistence behavior, but generally doors might need it.
    // Let's use 1 bit for open/closed target.
    bool is_open = (data & 1);
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
    return is_open ? 1 : 0;
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

    if (state == OPENING || state == CLOSING) {
        fm_vec3_t offset;
        fm_vec3_scale(&offset, &direction, move_distance * current_progress);
        fm_vec3_add(&transform.position, &initial_position, &offset);
    }
}

void SlidingDoor::draw() {
    if (enabled && !model_path.empty()) {
        T3DMat4FP* transform_fp = model.get_transform_fp();
        fm_vec3_t position; 
        fm_vec3_scale(&position, &transform.position, T3D_TOUNITSCALE);
        t3d_mat4fp_from_srt(transform_fp, (float*)&transform.scale, (float*)&transform.rotation, (float*)&position);
        model.draw();
    }
}

Entity* SlidingDoor::create(const std::string& name, int id) {
    return new SlidingDoor(name, id);
}
