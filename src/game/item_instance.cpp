#include <libdragon.h>
#include <fstream>
#include "engine_command.h"
#include "player.h"
#include "subtitles.h"
#include "item_instance.h"

bool itemsloaded = false;
tortellini::ini itemsdict;
std::string items_names[MAX_ITEMS];

//Add a return statement to handle cases where no journal entry is found by index
std::string get_item_name_by_id(int id){
    assertf(id >= 0 && id < MAX_ITEMS, "ItemInstance: get_item_name_by_id: index out of range: %d\n", id);
    return items_names[id];
}

// Combine 2 items into 1 using recepie rules
int items_check_combine_recepies(uint8_t item1_id, uint8_t item2_id){
    for (auto pair : itemsdict) {
        std::string craft_left = pair.section["craft_left"] | "";
        std::string craft_right = pair.section["craft_right"] | "";
        if(craft_left == "" || craft_right == "") continue;

        uint8_t left_item_id = itemsdict[craft_left]["item_id"] | -1;
        uint8_t right_item_id = itemsdict[craft_right]["item_id"] | -1;

        // check if selected items are the ingredients of this recipe
        if(!((left_item_id == item1_id && right_item_id == item2_id) 
        || (left_item_id == item2_id && right_item_id == item1_id))) continue;
        
        uint8_t crafted_item_id = pair.section["item_id"] | -1;
        debugf("ItemInstance: items_check_combine_recepies %s + %s = %s (id=%d)\n", 
               craft_left.c_str(), craft_right.c_str(), pair.name.c_str(), crafted_item_id);

        return crafted_item_id;
    }
    return -1;
}

void ItemInstance::preinit(){
    if(itemsloaded) return;
    std::string itemsdfn = filesystem_getfn(DIR_SCRIPT_LANG, "game/items.ini");
    assertf(filesystem_chkexist(itemsdfn.c_str()), "ItemInstance: items.ini not found at %s\n", itemsdfn.c_str());

    std::ifstream in(itemsdfn.c_str());
	in >> itemsdict;
    itemsloaded = true;
    int entriescount = 0;
    for(int i = 0; i < MAX_ITEMS; i++){
        for (auto pair : itemsdict) {
            if((pair.section["item_id"] | -1) == i){
                debugf("items_names[%d]: %s\n", i, pair.name.c_str());
                items_names[i] = pair.name;
                entriescount++;
            }
        }
    }
    debugf("ItemInstance: Loaded %d item definitions from %s\n", entriescount, itemsdfn.c_str());        
}

ItemInstance::ItemInstance(const std::string& name, int id)
    : Entity(name, ITEM_INSTANCE_TYPE_NAME, id), item_id(0), pickedup(false), itemmodel() {

    // Register functions that other entities can call
    /*register_console_command("set_value", [this](const std::vector<std::string>& args) {
        if (args.size() >= 1) {
            custom_value = std::stof(args[0]);
            debugf("ExampleEntity '%s': set_value called with %f\n", get_name().c_str(), custom_value);
        }
    });*/
}

void ItemInstance::load_from_ini(const tortellini::ini::section& section) {
    // Load custom properties from INI
    item_id = itemsdict[name]["item_id"] | -1;
    assertf(item_id >= 0, "ItemInstance '%s': item_id must be valid: %d >= 0", get_name().c_str(), item_id);

    std::string modelname = itemsdict[name]["model"] | "entity/rope";
    itemmodel.load(filesystem_getfn(DIR_MODEL, modelname.c_str()).c_str());

    requireditemtopickup = section["requireditemtopickup"] | "";
    pickuprange = section["range"] | 2.0f;
    
    debugf("ItemInstance '%s': loaded from INI (item_id=%d, requireditemtopickup=%s, pickuprange=%.2f)\n", 
           get_name().c_str(), item_id, requireditemtopickup.c_str(), pickuprange);
}

void ItemInstance::load_from_eeprom(uint16_t data) {
    pickedup = (data & (ITEM_INSTANCE_PICKEDUP_BIT)) != 0;
    
    debugf("ItemInstance '%s': loaded from EEPROM (pickedup=%d)\n",
           get_name().c_str(), pickedup);
}

uint16_t ItemInstance::save_to_eeprom() const {
    return (pickedup ? (ITEM_INSTANCE_PICKEDUP_BIT) : 0);
}

void ItemInstance::init() {
    debugf("ItemInstance '%s': initialized at position (%.2f, %.2f, %.2f)\n",
           get_name().c_str(),
           transform.position.x,
           transform.position.y,
           transform.position.z);
}

void ItemInstance::update() {
    // Update logic here
    if (!enabled || pickedup) return;

    T3DMat4FP* transform_fp = itemmodel.get_transform_fp();
    fm_vec3_t position; fm_vec3_scale(&position, &transform.position, T3D_TOUNITSCALE);
    t3d_mat4fp_from_srt(transform_fp, (float*)&transform.scale, (float*)&transform.rotation, (float*)&position);

    if(fm_vec3_distance(&transform.position, &player.position) < pickuprange){
        fm_vec3_t towards; fm_vec3_sub(&towards, &transform.position, &player.camera.position);
        fm_vec3_norm(&towards, &towards);
        float dot = fm_vec3_dot(&towards, &player.camera.forward);
        if(dot > 0.7f){

            int player_slot_that_has_required_item = -1;
            if(!requireditemtopickup.empty())
                player_slot_that_has_required_item = player_check_has_item(requireditemtopickup);

            if(requireditemtopickup.empty() || player_slot_that_has_required_item != -1){
                subtitles_add(dictstr("pickup"), 1.0f, 'A');
                if(player.joypad.pressed.a){
                    pickedup = true;
                    player_inventory_additem(item_id, 1);
                    sound_play("item_pickup", false);
                    std::string pickupmessage = (itemsdict[name]["pickupmessage"] | "");
                    if(!pickupmessage.empty()){
                        subtitles_add(dictstr(pickupmessage.c_str()), 6.0f, '\0');
                    } else{
                        subtitles_add((dictstr("itempickedup") + (itemsdict[name]["fullname"] | "ITEM_NAME")).c_str(), 4.0f, '\0');
                    }
                    if(!requireditemtopickup.empty()){
                        player_inventory_removeitem_by_slot(player_slot_that_has_required_item, 1);
                    }
                }
            }
        }
    }

}

void ItemInstance::draw() {
    if(enabled && !pickedup)
        itemmodel.draw();
}

// Factory function
Entity* ItemInstance::create(const std::string& name, int id) {
    return new ItemInstance(name, id);
}

