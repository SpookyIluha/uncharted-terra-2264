#include "example_entity.h"
#include "engine_command.h"
#include <libdragon.h>

ExampleEntity::ExampleEntity(const std::string& name, int id)
    : Entity(name, EXAMPLE_ENTITY_TYPE_NAME, id), custom_value(0.0f) {

    // Register functions that other entities can call

    register_console_command("set_value", [this](const std::vector<std::string>& args) {
        if (args.size() >= 1) {
            custom_value = std::stof(args[0]);
            debugf("ExampleEntity '%s': set_value called with %f\n", get_name().c_str(), custom_value);
        }
    });
}

void ExampleEntity::load_from_ini(const tortellini::ini::section& section) {
    // Load custom properties from INI
    custom_value = section["custom_value"] | 0.0f;
    enabled = section["enabled"] | true;
    
    debugf("ExampleEntity '%s': loaded from INI (value=%.2f, enabled=%d)\n", 
           get_name().c_str(), custom_value, enabled);
}

void ExampleEntity::load_from_eeprom(uint16_t data) {
    custom_value = (float)(data & 0x7FFF) / 100.0f;
    
    debugf("ExampleEntity '%s': loaded from EEPROM (value=%.2f, enabled=%d)\n",
           get_name().c_str(), custom_value, enabled);
}

uint16_t ExampleEntity::save_to_eeprom() const {
    // Encode data: bits 0-14 = custom_value * 100 (as int), bit 15 = enabled
    uint16_t value_int = (uint16_t)(custom_value * 100.0f) & 0x7FFF;
    uint16_t enabled_bit = enabled ? 0x8000 : 0;
    return value_int | enabled_bit;
}

void ExampleEntity::init() {
    debugf("ExampleEntity '%s': initialized at position (%.2f, %.2f, %.2f)\n",
           get_name().c_str(),
           get_transform().position.x,
           get_transform().position.y,
           get_transform().position.z);
}

void ExampleEntity::update() {
    // Update logic here
    if (enabled) {
        // Example: rotate slowly
        get_transform_mutable().rotation.y += 0.01f;
    }
}

void ExampleEntity::draw() {
    // Draw logic here
    // This is where you would draw the entity's visual representation
    // For now, it's empty as an example
}

// Factory function
Entity* ExampleEntity::create(const std::string& name, int id) {
    return new ExampleEntity(name, id);
}

