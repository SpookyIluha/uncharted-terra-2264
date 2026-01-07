#ifndef ENGINE_COMMAND_H
#define ENGINE_COMMAND_H

#include <functional>
#include <vector>
#include <string>

class Entity;

typedef std::function<void(std::vector<std::string>)> ConsoleCommand;
typedef std::function<void(Entity*, std::vector<std::string>)> EntityConsoleCommand;

void register_game_console_command(const std::string& name, ConsoleCommand func);
bool execute_console_command(const std::string& command);

void register_entity_console_command(const std::string& class_type, const std::string& func_name, EntityConsoleCommand func);
bool invoke_entity_console_command(Entity* entity, const std::string& func_name, const std::vector<std::string>& args);

#endif
