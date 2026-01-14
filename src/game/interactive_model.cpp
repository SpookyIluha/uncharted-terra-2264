#include "interactive_model.h"
#include "player.h"
#include "subtitles.h"
#include "item_instance.h" // for itemsdict
#include "engine_command.h"

extern Entity* player_potential_user; // Defined in player.cpp

InteractiveModel::InteractiveModel(const std::string& name, int id)
    : Entity(name, INTERACTIVE_MODEL_TYPE_NAME, id), 
      retrievable(false), range(2.5f), iteminserted(false) {

    register_console_command("use_item", [this](const std::vector<std::string>& args) {
        debugf("InteractiveModel '%s': use_item(), iteminserted %d argsize %d\n", get_name().c_str(), iteminserted, args.size());
        if (args.size() < 1) return;
        std::string item_name = args[0];

        if(iteminserted) return;
        if(item_name != required_item) {
            subtitles_add(dictstr(wrong_item_subtitle.c_str()), 5.0f, '\0', 10);
            return;
        };

        // Fire the command
        if (!on_use_command.empty()) {
            execute_console_command(on_use_command);
        }

        // Add subtitle
        if (!on_inserted_subtitle.empty()) {
            subtitles_add(dictstr(on_inserted_subtitle.c_str()), 5.0f, '\0', 10);
        }

        // Handle item consumption
        {
            // Find item ID
            int item_id = itemsdict[item_name]["item_id"] | -1;
            if (item_id != -1) {
                player_inventory_removeitem_by_item_id((uint8_t)item_id, 1);
                debugf("InteractiveModel '%s': Consumed item '%s' (id=%d)\n", get_name().c_str(), item_name.c_str(), item_id);
            } else {
                debugf("InteractiveModel '%s': Could not find item ID for '%s'\n", get_name().c_str(), item_name.c_str());
            }
        }
    });
}

void InteractiveModel::load_from_ini(const tortellini::ini::section& section) {
    iteminserted = false;
    enabled = section["enabled"] | true;
    model_path = section["model"] | "";
    inserted_model_path = section["inserted_model"] | "";
    on_use_command = section["on_use_command"] | "";
    on_retrieve_command = section["on_retrieve_command"] | "";
    retrievable = section["retrievable"] | false; // Default to false
    range = section["range"] | 2.5f;
    required_item = section["required_item"] | "";
    assertf(!required_item.empty(), "InteractiveModel '%s': required_item must be set", get_name().c_str());
    wrong_item_subtitle = section["wrong_item_subtitle"] | "";
    on_inserted_subtitle = section["on_inserted_subtitle"] | "";

    if (!model_path.empty()) {
        model.load(filesystem_getfn(DIR_MODEL, model_path.c_str()).c_str());
    }
    if (!inserted_model_path.empty()) {
        inserted_model.load(filesystem_getfn(DIR_MODEL, inserted_model_path.c_str()).c_str());
    }
    debugf("InteractiveModel '%s': loaded with model '%s', inserted_model '%s', on_use_command '%s', on_retrieve_command '%s', retrievable %d, range %.2f, required_item '%s', wrong_item_subtitle '%s', on_inserted_subtitle '%s'\n",
           get_name().c_str(), model_path.c_str(), inserted_model_path.c_str(), on_use_command.c_str(), on_retrieve_command.c_str(), retrievable, range, required_item.c_str(), wrong_item_subtitle.c_str(), on_inserted_subtitle.c_str());
}

void InteractiveModel::load_from_eeprom(uint16_t data) {
    iteminserted = (data & INTERACTIVE_MODEL_ITEMINSERTED_BIT) != 0;
}

uint16_t InteractiveModel::save_to_eeprom() const {
    return iteminserted? INTERACTIVE_MODEL_ITEMINSERTED_BIT : 0;
}

void InteractiveModel::init() {
    // Initial setup if needed
}

void InteractiveModel::update() {
    if (!enabled) return;

    // Check distance to player
    float dist = fm_vec3_distance(&transform.position, &player.position);
    
    if (dist < range && !iteminserted) {
        //subtitles_add(dictstr("somethingcanbeputhere"), 1.0f, '\0');
        player_potential_user = this;
    }
    if(dist < range && iteminserted && retrievable){
        subtitles_add(dictstr("pickup"), 1.0f, 'A');
        if(player.joypad.pressed.a){
            iteminserted = false;
            player_inventory_additem(itemsdict[required_item]["item_id"] | -1, 1);
            std::string pickupmessage = (itemsdict[required_item]["pickupmessage"] | "");
            if(!pickupmessage.empty()){
                subtitles_add(dictstr(pickupmessage.c_str()), 6.0f, '\0', 10);
            } else{
                subtitles_add((dictstr("itempickedup") + (itemsdict[required_item]["fullname"] | "ITEM_NAME")).c_str(), 4.0f, '\0', 10);
            }
            if(!on_retrieve_command.empty()){
                execute_console_command(on_retrieve_command);
            }
        }
    }
}

void InteractiveModel::draw() {
    if (enabled && !model_path.empty() && !iteminserted) {
        T3DMat4FP* transform_fp = model.get_transform_fp();
        fm_vec3_t position; 
        fm_vec3_scale(&position, &transform.position, T3D_TOUNITSCALE);
        t3d_mat4fp_from_srt(transform_fp, (float*)&transform.scale, (float*)&transform.rotation, (float*)&position);
        model.draw();
    } else if (enabled && !inserted_model_path.empty() && iteminserted) {
        T3DMat4FP* transform_fp = inserted_model.get_transform_fp();
        fm_vec3_t position; 
        fm_vec3_scale(&position, &transform.position, T3D_TOUNITSCALE);
        t3d_mat4fp_from_srt(transform_fp, (float*)&transform.scale, (float*)&transform.rotation, (float*)&position);
        inserted_model.draw();
    }
}

Entity* InteractiveModel::create(const std::string& name, int id) {
    return new InteractiveModel(name, id);
}
