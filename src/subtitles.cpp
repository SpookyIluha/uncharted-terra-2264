#include <libdragon.h>
#include <vector>
#include <string>
#include "engine_gamestatus.h"
#include "engine_command.h"
#include "utils.h"
#include "subtitles.h"

char current_subtitle[SUBTITLES_MAX_LENGTH] = {0};
float subtitle_duration = 0.0f;

void subtitles_init(){
    subtitle_duration = 0.0f;
    current_subtitle[0] = '\0';

    register_game_console_command("say", [](const std::vector<std::string>& args){
        assert(args.size() == 1);
        subtitles_add(args[0].c_str(), max(args[0].length() * (1.0f / 20.0f), 2.0f));
    });
}

void subtitles_update(){
    if(subtitle_duration > 0.0f){
        subtitle_duration -= DELTA_TIME;
    }
}

void subtitles_add(char* text, float duration){
    assertf(strlen(text) < SUBTITLES_MAX_LENGTH, "Subtitle text too long %s", text);
   strcpy(current_subtitle, text);
   subtitle_duration = duration;
}

void subtitles_draw(){
   if(subtitle_duration <= 0.0f) return;

   float alpha = fclampr(subtitle_duration, 0.0f, 2.0f);

   rdpq_fontstyle_t style = gamestatus.fonts.fonts[gamestatus.fonts.subtitlefont - 1].styles[gamestatus.fonts.subtitlefontstyle];
   style.color.a = alpha*127;
   style.outline_color.a = alpha*127;
   rdpq_font_style(gamestatus.fonts.fonts[gamestatus.fonts.subtitlefont - 1].font, gamestatus.fonts.subtitlefontstyle, &style);
   rdpq_textparms_t parms;
   parms.width = display_get_width();
   parms.height = SUBTITLES_HEIGHT;
   parms.align = ALIGN_CENTER;
   parms.style_id = gamestatus.fonts.subtitlefontstyle;
   rdpq_text_printf(&parms, gamestatus.fonts.subtitlefont, 0, SUBTITLES_START_Y, "%s", current_subtitle);

}

