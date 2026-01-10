#ifndef ENTITY_H
#define ENTITY_H
/// Entity system - base class for all game entities

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/t3dmath.h>
#include "engine_gamestatus.h"
#include "engine_command.h"
#include "utils.h"
#include "tortellini.hh"

#define ENTITY_IS_ENABLED (1<<1)
#define ENTITY_HAS_EEPROM_DATA (1<<0)

// Forward declarations
namespace tortellini {
    struct ini;
}

// Forward declaration for string_to_vec (defined in level.cpp)
extern fm_vec3_t string_to_vec(const std::string& input);

// Base abstract entity class
class Entity {
public:
    // Transform structure
    struct Transform {
        fm_vec3_t position;
        fm_vec4_t rotation;
        fm_vec3_t scale;
        
        Transform() {
            position = (fm_vec3_t){{0, 0, 0}};
            rotation = (fm_vec4_t){{0, 0, 0, 1}}; // Identity quaternion
            scale = (fm_vec3_t){{1, 1, 1}};
        }
    };

protected:
    std::string name;
    std::string class_type;
    static std::map<std::string, ConsoleCommand> commands;

    // Helper to register a function
    void register_console_command(const std::string& func_name, ConsoleCommand func) {
        std::string key = class_type + "_" + func_name;
        if(commands.count(key) > 0) return;
        Entity::commands[key] = func;
        debugf("Registered console command '%s' for entity '%s'\n", (key).c_str(), name.c_str());
    };

    int unique_id;
    Transform transform;

public:
    bool enabled;
    Entity(const std::string& entity_name, const std::string& entity_class_type, int entity_id);
    virtual ~Entity() = default;

    // Getters
    const std::string& get_name() const { return name; }
    const std::string& get_class_type() const { return class_type; }
    int get_id() const { return unique_id; }
    const Transform& get_transform() const { return transform; }
    Transform& get_transform_mutable() { return transform; }

    // Pure virtual functions that must be implemented by derived classes
    virtual void load_from_ini(const tortellini::ini::section& section) = 0;
    virtual void load_from_eeprom(uint16_t data) = 0;
    virtual uint16_t save_to_eeprom() const = 0;
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;

    // Call a function on this entity
    bool console_command_invoke(const std::string& func_name, const std::vector<std::string>& args);
};  

// Entity factory function type
typedef Entity* (*EntityFactory)(const std::string& name, int id);

class EntitySystem {
private:
    static std::map<std::string, EntityFactory> factories;
    static std::map<int, Entity*> entities;

public:
    // Register an entity class factory
    static void register_entity_type(const std::string& class_type, EntityFactory factory);
    
    // Create an entity instance
    static Entity* create_entity(const std::string& class_type, const std::string& name, int id = -1);
    
    // Load entities from INI file
    static void load_entities_from_ini(const std::string& levelname);
    
    // Get entity by ID
    static Entity* get_entity(int id);
    
    // Get entity by name
    static Entity* get_entity_by_name(const std::string& name);
    
    // Update all entities
    static void update_all();
    
    // Draw all entities
    static void draw_all();
    
    // Clear all entities
    static void clear_all();
    
    // Save all entities to EEPROM buffer
    static void save_all_to_eeprom();
};


#endif

