#include <libdragon.h>
#include <fstream>
#include "engine_command.h"
#include "player.h"
#include "subtitles.h"
#include "journal_entry.h"

bool journalsloaded = false;
tortellini::ini journals;

int journals_collected_count(){
    int count = 0;
    for(int i = 0; i < MAX_JOURNAL_ENTRIES; i++){
        if(gamestatus.state.game.journalscollectedbitflag & (1 << i)){
            debugf("JournalEntry: journalscollectedbitflag: %d, i = %d\n", gamestatus.state.game.journalscollectedbitflag, i);
            count++;
        }
    }
    return count;
}

//Add a return statement to handle cases where no journal entry is found by index
int get_collected_journal_entry_id_by_collected_index(int id){
    id+=1;
    int count = 0;  
    for(int i = 0; i < MAX_JOURNAL_ENTRIES; i++){
        if(gamestatus.state.game.journalscollectedbitflag & (1 << i)){
            count++;
        }
        if(count == id){
            return i;
        }
    }
    assertf(false, "JournalEntry: get_collected_journal_entry_name_by_index: index out of range: %d\n", id);

    // default return, should never be reached
    return 0;
}

void JournalEntry::preinit(){
    if(journalsloaded) return;
    std::string journalfn = filesystem_getfn(DIR_SCRIPT_LANG, "game/journals.ini");
    assertf(filesystem_chkexist(journalfn.c_str()), "JournalEntry: journals.ini not found at %s\n", journalfn.c_str());

    std::ifstream in(journalfn.c_str());
	in >> journals;
    journalsloaded = true;

    int entriescount = 0;
    for (auto pair : journals) {
        debugf("JournalEntries: %s\n", pair.name.c_str());
        entriescount++;
    }
    debugf("JournalEntry: Loaded %d journal entries from %s\n", entriescount, journalfn.c_str());        
}

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

    std::string modelname = section["model"] | "entity/journal";
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

}

void JournalEntry::update() {
    // Update logic here
    if (!enabled || pickedup) return;

    T3DMat4FP* transform_fp = journalmodel.get_transform_fp();
    fm_vec3_t position; fm_vec3_scale(&position, &transform.position, T3D_TOUNITSCALE);
    t3d_mat4fp_from_srt(transform_fp, (float*)&transform.scale, (float*)&transform.rotation, (float*)&position);

    if(fm_vec3_distance(&transform.position, &player.position) < 1.0f){
        fm_vec3_t towards; fm_vec3_sub(&towards, &transform.position, &player.camera.position);
        fm_vec3_norm(&towards, &towards);
        float dot = fm_vec3_dot(&towards, &player.camera.forward);
        if(dot > 0.7f){
            subtitles_add(dictstr("pickup"), 1.0f, 'A');
            if(player.joypad.held.a){
                pickedup = true;
                gamestatus.state.game.journalscollectedbitflag |= (1 << entry_id);
                std::string pickupmessage = (journals[std::to_string(entry_id)]["pickupmessage"] | "");
                if(!pickupmessage.empty()){
                    subtitles_add(pickupmessage.c_str(), 6.0f, '\0');
                } else{
                    subtitles_add((dictstr("journalpickedup") + (journals[std::to_string(entry_id)]["name"] | "JOURNAL_NAME")).c_str(), 6.0f, '\0');
                }
            }
        }
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

