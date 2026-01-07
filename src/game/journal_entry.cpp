#include "journal_entry.h"
#include "engine_command.h"
#include <libdragon.h>

JournalEntry::JournalEntry(const std::string& name, int id)
    : Entity(name, JOURNAL_ENTRY_TYPE_NAME, id), entry_id(0), pickedup(false), journalmodel() {

    // Register functions that other entities can call
    /*register_console_command("set_value", [this](const std::vector<std::string>& args) {
        if (args.size() >= 1) {
            custom_value = std::stof(args[0]);
            debugf("ExampleEntity '%s': set_value called with %f\n", get_name().c_str(), custom_value);
        }
    });*/
}


void JournalEntry::load_from_ini(const tortellini::ini::section& section) {
    // Load custom properties from INI
    entry_id = section["entry_id"] | -1;
    assertf(entry_id >= 0, "JournalEntry '%s': entry_id must be valid: %d >= 0", get_name().c_str(), entry_id);

    std::string modelname = section["model"] | "entity/journalmodel";
    journalmodel.load(filesystem_getfn(DIR_MODEL, modelname.c_str()).c_str());
    
    debugf("JournalEntry '%s': loaded from INI (entry_id=%d)\n", 
           get_name().c_str(), entry_id);
}

void JournalEntry::load_from_eeprom(uint16_t data) {
    pickedup = (gamestatus.state.game.journalscollectedbitflag & (1 << entry_id)) != 0;
    
    debugf("JournalEntry '%s': loaded from EEPROM (pickedup=%d)\n",
           get_name().c_str(), pickedup);
}

uint16_t JournalEntry::save_to_eeprom() const {
    if(pickedup) gamestatus.state.game.journalscollectedbitflag |= (1 << entry_id);
    return 0;
}

void JournalEntry::init() {
    debugf("JournalEntry '%s': initialized at position (%.2f, %.2f, %.2f)\n",
           get_name().c_str(),
           transform.position.x,
           transform.position.y,
           transform.position.z);
}

void JournalEntry::update() {
    // Update logic here
    if (enabled && !pickedup) {
        T3DMat4FP* transform_fp = journalmodel.get_transform_fp();
        t3d_mat4fp_from_srt_euler(transform_fp, transform.position, transform.rotation, transform.scale);
    }
}

void JournalEntry::draw() {
    if(enabled && !pickedup)
        journalmodel.draw();
}

// Factory function
Entity* JournalEntry::create(const std::string& name, int id) {
    return new JournalEntry(name, id);
}

