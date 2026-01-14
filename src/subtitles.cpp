#include <libdragon.h>
#include <vector>
#include <string>
#include "engine_gamestatus.h"
#include "engine_filesystem.h"
#include "engine_command.h"
#include "utils.h"
#include "subtitles.h"

char current_subtitle[SUBTITLES_MAX_LENGTH] = {0};
float subtitle_duration = 0.0f;
int current_priority = 0;

sprite_t* a_button;
sprite_t* b_button;
sprite_t* start_button;

sprite_t* selectedsprite;

void subtitles_init(){
    subtitle_duration = 0.0f;
    current_subtitle[0] = '\0';
    a_button = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/button_a.rgba32").c_str());
    b_button = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/button_b.rgba32").c_str());
    start_button = sprite_load(filesystem_getfn(DIR_IMAGE, "ui/button_start.rgba32").c_str());

    register_game_console_command("say", [](const std::vector<std::string>& args){
        assert(args.size() == 1 || args.size() == 2);
        float duration = std::max(args[0].length() * (1.0f / CHARACTERS_PER_SEC_DISPLAY), 2.0f);
        subtitles_add(dictstr(args[0].c_str()), duration, args.size() == 2 ? args[1][0] : '\0');
    });
}

void subtitles_free(){
    sprite_free(a_button);
    sprite_free(b_button);
    sprite_free(start_button);
}

void subtitles_update(){
    if(subtitle_duration > 0.0f){
        subtitle_duration -= DELTA_TIME;
    } else {
        current_priority = 0;
    }
}

void subtitles_add(const char* text, float duration, char buttonsprite){
   subtitles_add(text, duration, buttonsprite, 1);
}

void subtitles_add(const char* text, float duration, char buttonsprite, int priority){
    if(priority < current_priority) return;
    current_priority = priority;

    assertf(strlen(text) < SUBTITLES_MAX_LENGTH, "Subtitle text too long %s", text);
    strcpy(current_subtitle, text);
    subtitle_duration = duration;
    switch(buttonsprite){
        case 'A': selectedsprite = a_button; break;
        case 'B': selectedsprite = b_button; break;
        case 'S': selectedsprite = start_button; break;
        default: selectedsprite = nullptr; break;
   }
}

void subtitles_draw(){
   if(subtitle_duration <= 0.0f) return;

   float alpha = fclampr(subtitle_duration, 0.0f, 1.0f);

   rdpq_fontstyle_t style = gamestatus.fonts.fonts[gamestatus.fonts.subtitlefont - 1].styles[gamestatus.fonts.subtitlefontstyle];
   style.color.a = alpha*255;
   style.outline_color.a = alpha*255;
   rdpq_font_style(gamestatus.fonts.fonts[gamestatus.fonts.subtitlefont - 1].font, gamestatus.fonts.subtitlefontstyle, &style);
   rdpq_textparms_t parms;
   parms.width = display_get_width() * 0.75f;
   parms.height = SUBTITLES_HEIGHT;
   parms.align = ALIGN_CENTER;
   parms.wrap = WRAP_WORD;
   parms.style_id = gamestatus.fonts.subtitlefontstyle;
   rdpq_text_printf(&parms, gamestatus.fonts.subtitlefont, display_get_width() * 0.125f, display_get_height() - SUBTITLES_OFFSET_Y , "%s", current_subtitle);
   if(selectedsprite && alpha > 0.9f){
      rdpq_set_mode_standard();
      rdpq_mode_alphacompare(8);
      rdpq_sprite_blit(selectedsprite, (display_get_width() - selectedsprite->width)/2, display_get_height() - SUBTITLES_OFFSET_Y - selectedsprite->height - 5, NULL);
   }

}

