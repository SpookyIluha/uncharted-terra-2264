#include <assert.h>
#include <fstream>
#include "engine_filesystem.h"
#include "engine_gamestatus.h"
#include "utils.h"
#include "level.h" // For string_to_vec
#include "game/entity.h"

static std::map<std::string, ConsoleCommand> game_console_commands;

static std::vector<std::string> tokenize(const std::string& str) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;
    for (size_t i = 0; i < str.size(); i++) {
        char c = str[i];
        if (c == '"') {
            in_quotes = !in_quotes;
            continue;
        }
        if (!in_quotes && (c == ' ' || c == '\t')) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

void register_game_console_command(const std::string& name, ConsoleCommand func) {
    game_console_commands[name] = func;
    debugf("Registered game console command: %s\n", name.c_str());
}

bool execute_console_command(const std::string& command) {
    std::vector<std::string> tokens = tokenize(command);

    assertf(!tokens.empty(),"Received empty command\n");

    if (tokens[0] == "entity") {
        assertf(tokens.size() >= 3, "Entity command requires at least 3 tokens: entity <name> <func> [args...]\n");

        const std::string& entity_name = tokens[1];
        const std::string& func_name = tokens[2];
        std::vector<std::string> args(tokens.begin() + 3, tokens.end());

        Entity* e = EntitySystem::get_entity_by_name(entity_name);
        if (!e) {
            debugf("WARNING: execute_console_command could not find entity '%s'\n", entity_name.c_str());
            return false;
        }
        bool ok = e->console_command_invoke(func_name, args);
        if (!ok) {
            debugf("WARNING: execute_console_command entity '%s' has no function '%s'\n", entity_name.c_str(), func_name.c_str());
        }
        return ok;
    }

    const std::string& cmd_name = tokens[0];
    auto it = game_console_commands.find(cmd_name);

    assertf(it != game_console_commands.end(), "Unknown game console command: %s\n", cmd_name.c_str());

    std::vector<std::string> args(tokens.begin() + 1, tokens.end());
    it->second(args);
    return true;
}

static std::map<std::string, std::map<std::string, EntityConsoleCommand>> entity_console_commands;

void register_entity_console_command(const std::string& class_type, const std::string& func_name, EntityConsoleCommand func) {
    entity_console_commands[class_type][func_name] = func;
    debugf("Registered entity console command '%s' for type '%s'\n", func_name.c_str(), class_type.c_str());
}

bool invoke_entity_console_command(Entity* entity, const std::string& func_name, const std::vector<std::string>& args) {
    if (!entity) return false;
    
    // Try specific class type
    std::string type = entity->get_class_type();
    if (entity_console_commands.count(type) && entity_console_commands[type].count(func_name)) {
        entity_console_commands[type][func_name](entity, args);
        return true;
    }
    
    // Try base "Entity" type
    if (entity_console_commands.count("Entity") && entity_console_commands["Entity"].count(func_name)) {
        entity_console_commands["Entity"][func_name](entity, args);
        return true;
    }

    return false;
}
