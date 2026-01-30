#include <libdragon.h>
#include <fstream>
#include <map>
#include "audioutils.h"
#include "engine_gamestatus.h"
#include "tortellini.hh"
#include "engine_eeprom.h"
#include "engine_filesystem.h"

std::map<std::string, std::string> dictionary;
std::map<std::string, std::string> languages;

void engine_load_dictionary(){
    rspq_wait();
    dictionary.clear();

    tortellini::ini ini;
	std::ifstream in(filesystem_getfn(DIR_SCRIPT_LANG, "dictionary.ini").c_str());
	in >> ini;
    // iterate over all Dictionary section
    tortellini::ini::section section = ini["Dictionary"];
    // for each entry add it to map of strings
    for(auto& pair : section._mapref){
        dictionary[pair.first] = unquote(pair.second);
    }
}

std::string& unquote(std::string& s){
    if ( s.front() == '"' ) {
        s.erase( 0, 1 ); // erase the first character
        s.erase( s.size() - 1 ); // erase the last character
    }
    size_t idx = 0; while ((idx = s.find("\\n", idx)) != std::string::npos) {
        s.replace(idx, 2, "\n");
        ++idx;
    }
    return s;
}


/// @brief Load a default .ini configuration file with all the info on the game's structure (rom://scripts/lang/config.ini)
void engine_config_load(){
    tortellini::ini ini;

	// (optional) Read INI in from a file.
	// Default construction and subsequent assignment is just fine, too.
	std::ifstream in(filesystem_getfn(DIR_SCRIPT_LANG, "config.ini").c_str());
	in >> ini;

    gamestatus.state.scripts.debug =  ini["Scripts"]["debug"] | false;

    strcpy(gamestatus.startlevelname,  (ini["Game"]["startlevel"] | "START_LEVEL_NOT_SET").c_str());

    gamestatus.fonts.mainfont       =  ini["Fonts"]["mainfont"]         | 1;
    gamestatus.fonts.mainfontstyle  =  ini["Fonts"]["mainfontstyle"]    | 0;
    gamestatus.fonts.mainfontselected  =  ini["Fonts"]["mainfontselected"]    | 0;
    gamestatus.fonts.titlefont      =  ini["Fonts"]["titlefont"]        | 1;
    gamestatus.fonts.titlefontstyle =  ini["Fonts"]["titlefontstyle"]   | 0;
    gamestatus.fonts.subtitlefont      =  ini["Fonts"]["subtitlefont"]        | 1;
    gamestatus.fonts.subtitlefontstyle =  ini["Fonts"]["subtitlefontstyle"]   | 0;
    gamestatus.fonts.unavailablefontstyle = ini["Fonts"]["unavailablefontstyle"] | 0;

    std::string wrappingmode = ini["Fonts"]["wrappingmode"] | "WRAP_WORD";
    if(wrappingmode == "WRAP_WORD"){
        gamestatus.fonts.wrappingmode = WRAP_WORD;
    } else if (wrappingmode == "WRAP_CHAR"){
        gamestatus.fonts.wrappingmode = WRAP_CHAR;
    }

    if(gamestatus.fonts.fontcount){
        for(int i = 1; i <= gamestatus.fonts.fontcount; i++){
            rdpq_text_unregister_font(i);
            if(gamestatus.fonts.fonts[i - 1].font) {
                rdpq_font_free(gamestatus.fonts.fonts[i - 1].font); 
                gamestatus.fonts.fonts[i - 1].font = NULL;
            }
        }
    }
    {
        int fontid = 1;
        char keyfont[128]; char keyfontcolor[128]; char keyfontcoloroutline[128];
        std::string font;
        do{
            sprintf(keyfont, "font%i", fontid);
            font = ini["Fonts"][keyfont] | "";
            if(!font.empty()){
                std::string fontfn = filesystem_getfn(DIR_FONT, (font).c_str());
                debugf("Font %i: %s\n", fontid,fontfn.c_str() );
                gamestatus.fonts.fonts[fontid - 1].font = rdpq_font_load(fontfn.c_str());
                rdpq_text_register_font(fontid, gamestatus.fonts.fonts[fontid - 1].font);
                gamestatus.fonts.fontcount = fontid;

                int styleid = 0;
                uint32_t color1 = 0x000000FE; uint32_t color2 = 0xFEFEFEFE;
                do{
                    sprintf(keyfontcolor, "font%i_color%itext", fontid, styleid);
                    sprintf(keyfontcoloroutline, "font%i_color%ioutline", fontid, styleid);
                    color1 = ini["Fonts"][keyfontcolor] | 0xFAFAFAFE;
                    color2 = ini["Fonts"][keyfontcoloroutline] | 0xFEFEFEFE;
                    debugf("Font %i Style %i: %08lx %08lx\n", fontid, styleid, color1, color2);
                    rdpq_fontstyle_t style; style.color = color_from_packed32(color1); style.outline_color = color_from_packed32(color2);
                    rdpq_font_style(gamestatus.fonts.fonts[fontid - 1].font, styleid, &style);
                    gamestatus.fonts.fonts[fontid - 1].styles[styleid] = style;
                    styleid++;
                } while (color1 != 0xFAFAFAFE);
            }

            fontid++;
        } while (!font.empty() && fontid < MAX_FONTS);
    }

    in.close();
}

const char* dictstr(const char* name){
    if(dictionary.count(name))
        return dictionary[name].c_str();
    else return name;
}

void engine_load_languages(){
    languages.clear();

    tortellini::ini ini;
	std::ifstream in(filesystem_getfn(DIR_SCRIPT, "languages.ini").c_str());
	in >> ini;
    // iterate over all Dictionary section
    tortellini::ini::section section = ini["Languages"];
    // for each entry add it to map of strings
    for(auto& pair : section._mapref)
        languages[pair.first] = pair.second;
}

void engine_set_language(const char* lang){
    if(gamestatus.state.scripts.debug)
    debugf("Set language: %s\n", lang);
    strcpy(gamestatus.state_persistent.curlang, lang);
}

char* engine_get_language(){
    if(gamestatus.state_persistent.curlang[0]) return gamestatus.state_persistent.curlang;
    else return NULL;
}

filesystem_info_t filesysteminfo;

bool filesystem_ismodded(){
    return false;
}

void filesystem_init(){
    strcpy(filesysteminfo.romrootdir, "rom:/");
    strcpy(gamestatus.data.bgmfolder, "music");
    strcpy(gamestatus.data.sfxfolder, "sfx");
    strcpy(gamestatus.data.fontfolder, "fonts");
    strcpy(gamestatus.data.imagesfolder, "textures");
    strcpy(gamestatus.data.scriptsfolder, "scripts");
    strcpy(gamestatus.data.moviesfolder, "movies");
}

bool filesystem_chkexist(const char* fn){
    FILE *f = fopen(fn, "r"); 
    if(f){
        fclose(f);
        if(gamestatus.state.scripts.debug)
            debugf("File: %s is found\n", fn);
        return true;}
    if(gamestatus.state.scripts.debug)
        debugf("File: %s does not exist\n", fn);
    return false;
}

std::string filesystem_getfolder(assetdir_t dir, bool modded, bool en_lang){
    std::string romfn = std::string(filesysteminfo.romrootdir);
    std::string sdfn = std::string(filesysteminfo.rootdir);

    std::string romfn_en = std::string(filesysteminfo.romrootdir);
    std::string sdfn_en = std::string(filesysteminfo.rootdir);

    switch(dir){
        case DIR_ROOT:
            romfn += "/" ;
            sdfn += "/" ;
            break;
        case DIR_SCRIPT:
            romfn += std::string("/") + gamestatus.data.scriptsfolder + "/" ;
            sdfn += std::string("/") + gamestatus.data.scriptsfolder + "/" ;
            break;
        case DIR_SCRIPT_LANG:
            romfn += std::string("/") + gamestatus.data.scriptsfolder + "/" + gamestatus.state_persistent.curlang + "/" ;
            sdfn += std::string("/") + gamestatus.data.scriptsfolder + "/" + gamestatus.state_persistent.curlang + "/" ;
            romfn_en += std::string("/") + gamestatus.data.scriptsfolder + "/en/" ;
            sdfn_en += std::string("/") + gamestatus.data.scriptsfolder + "/en/" ;
            break;
        case DIR_IMAGE:
            romfn += std::string("/") + gamestatus.data.imagesfolder + "/";
            sdfn += std::string("/") + gamestatus.data.imagesfolder + "/";
            break;
        case DIR_FONT:
            romfn += std::string("/") + gamestatus.data.fontfolder + "/";
            sdfn += std::string("/") + gamestatus.data.fontfolder + "/";
            break;
        case DIR_MUSIC:
            romfn += std::string("/") + gamestatus.data.bgmfolder + "/";
            sdfn += std::string("/") + gamestatus.data.bgmfolder + "/";
            break;
        case DIR_SOUND:
            romfn += std::string("/") + gamestatus.data.sfxfolder + "/";
            sdfn += std::string("/") + gamestatus.data.sfxfolder + "/";
            break;
        case DIR_MOVIES:
            romfn += std::string("/") + gamestatus.data.moviesfolder + "/";
            sdfn += std::string("/") + gamestatus.data.moviesfolder + "/";
            break;
        case DIR_MODEL:
            romfn += std::string("/") + gamestatus.data.modelsfolder + "/";
            sdfn += std::string("/") + gamestatus.data.modelsfolder + "/";
            break;
    }
    if(en_lang && dir == DIR_SCRIPT_LANG) {
        return romfn_en;
    }
    return romfn;
}

std::string filesystem_getfn(assetdir_t dir, const char* name){
    std::string romfn = std::string(filesysteminfo.romrootdir);
    std::string sdfn = std::string(filesysteminfo.rootdir);

    std::string romfn_en = std::string(filesysteminfo.romrootdir);
    std::string sdfn_en = std::string(filesysteminfo.rootdir);

    switch(dir){
        case DIR_ROOT:
            romfn += std::string("/") + name;
            sdfn += std::string("/") + name;
            break;
        case DIR_SCRIPT:
            romfn += std::string("/") + gamestatus.data.scriptsfolder + "/" + name;
            sdfn += std::string("/") + gamestatus.data.scriptsfolder + "/" + name;
            break;
        case DIR_SCRIPT_LANG:
            romfn += std::string("/") + gamestatus.data.scriptsfolder + "/locale/" + gamestatus.state_persistent.curlang + "/" + name;
            sdfn += std::string("/") + gamestatus.data.scriptsfolder + "/locale/" + gamestatus.state_persistent.curlang + "/" + name;
            romfn_en += std::string("/") + gamestatus.data.scriptsfolder + "/locale/" + "en" + "/" ;
            sdfn_en += std::string("/") + gamestatus.data.scriptsfolder + "/locale/" + "en" + "/" ;
            break;
        case DIR_IMAGE:
            romfn += std::string("/") + gamestatus.data.imagesfolder + "/" + name + ".sprite";
            sdfn += std::string("/") + gamestatus.data.imagesfolder + "/" + name + ".sprite";
            break;
        case DIR_FONT:
            romfn += std::string("/") + gamestatus.data.fontfolder + "/" + name + ".font64";
            sdfn += std::string("/") + gamestatus.data.fontfolder + "/" + name + ".font64";
            break;
        case DIR_MUSIC:
            romfn += std::string("/") + gamestatus.data.bgmfolder + "/" + name + ".wav64";
            sdfn += std::string("/") + gamestatus.data.bgmfolder + "/" + name + ".wav64";
            break;
        case DIR_SOUND:
            romfn += std::string("/") + gamestatus.data.sfxfolder + "/" + name + ".wav64";
            sdfn += std::string("/") + gamestatus.data.sfxfolder + "/" + name + ".wav64";
            break;
        case DIR_MOVIES:
            romfn += std::string("/") + gamestatus.data.moviesfolder + "/" + name + ".m1v";
            sdfn += std::string("/") + gamestatus.data.moviesfolder + "/" + name + ".m1v";
            break;
        case DIR_MODEL:
            romfn += std::string("/") + gamestatus.data.modelsfolder + "/" + name + ".t3dm";
            sdfn += std::string("/") + gamestatus.data.modelsfolder + "/" + name + ".t3dm";
            break;
    }

    if(filesystem_chkexist(romfn.c_str())) return romfn;
    if(filesystem_chkexist(romfn_en.c_str())) return romfn_en;
    
    if(gamestatus.state.scripts.debug)
        debugf("File %s does not exist in SD nor in ROM!\n", romfn.c_str());
    return romfn;
}