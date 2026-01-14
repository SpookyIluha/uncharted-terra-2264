#include "trigger_zone.h"
#include "player.h"
#include "engine_command.h"

TriggerZone::TriggerZone(const std::string& name, int id)
    : Entity(name, TRIGGER_ZONE_TYPE_NAME, id), 
      one_shot(false), triggered(false) {
}

void TriggerZone::load_from_ini(const tortellini::ini::section& section) {
    enabled = section["enabled"] | true;
    on_trigger_command = section["command"] | "";
    one_shot = section["trigger_once"] | false;
}

void TriggerZone::load_from_eeprom(uint16_t data) {
    if (one_shot) {
        triggered = (data & TRIGGER_ZONE_TRIGGERED_BIT);
        if (triggered) enabled = false; // Disable if already triggered
    }
}

uint16_t TriggerZone::save_to_eeprom() const {
    return (one_shot && triggered) ? TRIGGER_ZONE_TRIGGERED_BIT : 0;
}

void TriggerZone::init() {

}

bool TriggerZone::check_collision() {
    return collideAABB(&player.position, 0, &transform.position, &transform.scale, nullptr);
}

void TriggerZone::update() {
    if (!enabled) return;
    if (one_shot && triggered) return;

    if (check_collision()) {
        if (!on_trigger_command.empty()) {
            execute_console_command(on_trigger_command);
            debugf("TriggerZone '%s': Fired command '%s'\n", get_name().c_str(), on_trigger_command.c_str());
        }

        if (one_shot) {
            triggered = true;
            enabled = false;
        }
    }
}

void TriggerZone::draw() {

}

Entity* TriggerZone::create(const std::string& name, int id) {
    return new TriggerZone(name, id);
}
