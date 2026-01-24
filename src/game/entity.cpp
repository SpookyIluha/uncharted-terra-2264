#include "entity.h"
#include "engine_filesystem.h"
#include "engine_gamestatus.h"
#include "engine_command.h"
#include "player.h"
#include "utils.h"
#include "level.h" // For string_to_vec
#include <assert.h>
#include <fstream>

std::map<std::string, ConsoleCommand> Entity::commands;

void Entity::clear_commands(){
    Entity::commands.clear();
}

// Entity class implementation
Entity::Entity(const std::string& entity_name, const std::string& entity_class_type, int entity_id)
    : name(entity_name), class_type(entity_class_type), unique_id(entity_id) {

    // shared console commands for each entity
    register_console_command("enable", [this](std::vector<std::string> args) {
        enabled = true; 
    });
    
    register_console_command("disable", [this](const std::vector<std::string>& args) {
        enabled = false;
    });

}

bool Entity::console_command_invoke(const std::string& func_name, const std::vector<std::string>& args) {
    auto it = Entity::commands.find(name + "_" + func_name);
    if (it != Entity::commands.end()) {
        it->second(args);
        return true;
    }
    return false;
}

// EntitySystem static members
std::map<std::string, EntityFactory> EntitySystem::factories;
std::map<int, Entity*> EntitySystem::entities;

void EntitySystem::register_entity_type(const std::string& class_type, EntityFactory factory) {
    factories[class_type] = factory;
    debugf("Registered entity type: %s\n", class_type.c_str());
}

Entity* EntitySystem::create_entity(const std::string& class_type, const std::string& name, int id) {
    auto it = factories.find(class_type);

    assertf(it != factories.end(), "Unknown entity class type: %s. Did you forget to register it?\n", class_type.c_str());

    // Use provided ID
    assertf(id >= 0 && id < MAX_ENTITIES, "Entity ID must be valid: %d >= 0 && %d < %d", id, id, MAX_ENTITIES);
    
    Entity* entity = it->second(name, id);
    if (entity) {
        entities[id] = entity;
        debugf("Created entity: %s (type: %s, id: %d)\n", name.c_str(), class_type.c_str(), id);
    }
    
    return entity;
}

void EntitySystem::load_entities_from_ini(const std::string& levelname) {
    // Clear existing entities first
    clear_all();
    Entity::clear_commands();
    
    // Load entities.ini file
    std::string entities_filename = filesystem_getfn(DIR_SCRIPT, (std::string("levels/") + levelname + ".entities.ini").c_str());
    
    // Check if file exists (entities are optional)
    if (!filesystem_chkexist(entities_filename.c_str())) {
        debugf("No entities file found for level %s\n", levelname.c_str());
        return;
    }
    
    tortellini::ini ini;
    std::ifstream in(entities_filename.c_str());

    assertf(in.is_open(),"Could not open entities file: %s", entities_filename.c_str());

    in >> ini;
    
    int entity_count = 0;
    
    // Loop over all sections in the INI file
    for (const auto& pair : ini) {
        const std::string& section_name = pair.name;
        const auto& section = pair.section;
        
        // Get required fields
        std::string class_type = section["class"] | "";
        std::string name = section["name"] | section_name;
        bool enabled = section["enabled"] | true;
        int id = section["id"] | -1;

        assertf(id >= 0 && id < MAX_ENTITIES, "Entity '%s' has invalid id '%i'\n", name.c_str(), id);
        
        if (class_type.empty()) {
            debugf("WARNING: Entity section '%s' missing 'class' field, skipping\n", section_name.c_str());
            continue;
        }
        
        // Create entity instance
        Entity* entity = create_entity(class_type, name, id);
        entity->enabled = enabled;
        assertf(entity, "Failed to create entity '%s' of type '%s'\n", name.c_str(), class_type.c_str());
        
        // Load transform if provided
        {
            std::string pos_str = section["position"] | "0 0 0";
            entity->get_transform_mutable().position = string_to_vec(pos_str.c_str());
        }
        {
            std::string rot_str = section["rotation"] | "0 0 0";
            entity->get_transform_mutable().rotation = string_to_quat(rot_str.c_str());
        }   
        {
            std::string scale_str = section["scale"] | "1 1 1";
            entity->get_transform_mutable().scale = string_to_vec(scale_str.c_str());
        }
        
        // Load entity-specific data from INI
        entity->load_from_ini(ini[section_name]);
        if((gamestatus.state.game.entities[id].flags & ENTITY_HAS_EEPROM_DATA)){
            entity->load_from_eeprom(gamestatus.state.game.entities[id].flags);
            entity->enabled = (gamestatus.state.game.entities[id].flags & ENTITY_IS_ENABLED)? true : false;
        }
        player_load_from_eeprom();
        
        // Initialize the entity
        entity->init();
        
        entity_count++;
    }
    
    debugf("Loaded %d entities for level %s\n", entity_count, levelname.c_str());
}

Entity* EntitySystem::get_entity(int id) {
    auto it = entities.find(id);
    if (it != entities.end()) {
        return it->second;
    }
    return nullptr;
}

Entity* EntitySystem::get_entity_by_name(const std::string& name) {
    for (auto& pair : entities) {
        if (pair.second->get_name() == name) {
            return pair.second;
        }
    }
    return nullptr;
}

void EntitySystem::update_all() {
    for (auto& pair : entities) {
        pair.second->update();
    }
}

void EntitySystem::draw_all() {
    for (auto& pair : entities) {
        t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);
        pair.second->draw();
    }
}

void EntitySystem::clear_all() {
    for (auto& pair : entities) {
        delete pair.second;
    }
    entities.clear();
}

void EntitySystem::save_all_to_eeprom() {
    for (auto& pair : entities) {
        Entity* entity = pair.second;
        gamestatus.state.game.entities[entity->get_id()].flags = entity->save_to_eeprom() | (entity->enabled? ENTITY_IS_ENABLED : 0) | ENTITY_HAS_EEPROM_DATA;
    }
    player_save_to_eeprom();
}



