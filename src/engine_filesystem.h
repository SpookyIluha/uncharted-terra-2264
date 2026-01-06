#ifndef ENGINE_FILESYSTEM_H
#define ENGINE_FILESYSTEM_H
/// made by SpookyIluha
/// Filesystem, language and modding code

#include <libdragon.h>
#include <map>
#include "audioutils.h"
#include "engine_gamestatus.h"
#include "tortellini.hh"
#include "engine_eeprom.h"

/// @brief Dictionary map string/string for GUI translations
extern std::map<std::string, std::string> dictionary;
/// @brief Language map string/string for a translation list
extern std::map<std::string, std::string> languages;

/// @brief Load a dictionary with selected language
void engine_load_dictionary();

/// @brief Get a translated string with the current language by a key
/// @param name key of the translated string as found in dictionary.ini
/// @return translated string
const char* dictstr(const char* name);

/// @brief Load languages list into the engine
void engine_load_languages();

/// @brief Load config file with game settings
void engine_config_load();

/// @brief Set the desired language of the game
/// @param lang shortname of the language as found in languages.ini (i.e en/ru/de/fr etc)
void engine_set_language(const char* lang);

/// @brief Get the current language code
/// @return shortname of the language as found in languages.ini (i.e en/ru/de/fr etc)
char* engine_get_language();

std::string& unquote(std::string& s);


/// @brief Types of files and folders the engine supports
typedef enum{
    DIR_ROOT, // root as in rom:/ or sd:/mod_folder/
    DIR_SCRIPT, // scripts folder scripts/
    DIR_SCRIPT_LANG, // scripts translated folder scripts/lang/
    DIR_IMAGE, // a generic image file
    DIR_FONT, // font file
    DIR_MUSIC, // music file .wav64
    DIR_SOUND, // sound effect file .wav64
    DIR_MOVIES, // movies folder
    DIR_MODEL, // t3dm model files
} assetdir_t;

typedef struct{
    char rootdir[256]; // rom:/
    char romrootdir[256]; // sd:/[mod_folder]/
} filesystem_info_t;


extern filesystem_info_t filesysteminfo;

/// @brief Check if the game is modded or not
/// @return true - modded (can access custom files in rootdir)
bool filesystem_ismodded();

/// Init a filesystem of the engine, checking for a mod folder
void filesystem_init();

/// Check if a certain file exists at all (in a mod or in rom)
bool filesystem_chkexist(const char* fn);

/// @brief Get a folder in the game's assets
/// @param dir Type of the folder
/// @param modded Whether the modded folder should be returned
/// @param en_lang Whether an english translation of the folder should be returned
/// @return fn of the directory
std::string filesystem_getfolder(assetdir_t dir, bool modded, bool en_lang);

/// @brief Get a filename to open, checks if it exists in a mod or rom
/// @param dir Type of the file and where it can be found
/// @param name filename without the extension (i.e DIR_IMAGE with "background1" will find a "rom:/[image_folder]/background1.sprite" on an unmodded game)
/// @return a filename to open
std::string filesystem_getfn(assetdir_t dir, const char* name);

#ifdef __cplusplus
extern "C"{
#endif

#ifdef __cplusplus
}
#endif

#endif