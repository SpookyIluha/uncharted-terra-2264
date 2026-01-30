#a list of custom asset flags to use when converting to Libradgon formats

filesystem/fonts/neuropolitical.font64: 				MKFONT_FLAGS+= --size 13 --range 00-17F
filesystem/fonts/recharge.font64: 						MKFONT_FLAGS+= --size 13  --range 00-17F

filesystem/fonts/ru/science-gothic.font64: 		MKFONT_FLAGS+= --size 13  --range 00-17F --range 0400-052F

filesystem/fonts/ja/SmartFontUI.font64: 	MKFONT_FLAGS+= --size 16   --charset "assets/scripts/locale/ja/storytime.txt" --charset "assets/scripts/locale/ja/end_a.txt" --charset "assets/scripts/locale/ja/end_b.txt" --charset "assets/scripts/locale/ja/credits.txt" --charset "assets/scripts/locale/ja/dictionary.ini" --charset "assets/scripts/locale/ja/game/items.ini" --charset "assets/scripts/locale/ja/game/journals.ini"

filesystem/textures/%.sprite: MKSPRITE_FLAGS += --dither ORDERED
filesystem/models/%.sprite:  MKSPRITE_FLAGS += --mipmap BOX

filesystem/sfx/ambience/ambience0.wav64:  AUDIOCONV_FLAGS += --wav-resample 11050
filesystem/sfx/ambience/ambience1.wav64:  AUDIOCONV_FLAGS += --wav-resample 11050
filesystem/sfx/ambience/ambience2.wav64:  AUDIOCONV_FLAGS += --wav-resample 11050
filesystem/sfx/ambience/ambience3.wav64:  AUDIOCONV_FLAGS += --wav-resample 11050
filesystem/sfx/ambience/ambience4.wav64:  AUDIOCONV_FLAGS += --wav-resample 11050
filesystem/sfx/ambience/ambience5.wav64:  AUDIOCONV_FLAGS += --wav-resample 11050
filesystem/sfx/ambience/ambience6.wav64:  AUDIOCONV_FLAGS += --wav-resample 11050
filesystem/sfx/ambience/ambience_intro.wav64:  AUDIOCONV_FLAGS += --wav-resample 7000 --wav-compress 1,bits=2

filesystem/sfx/intro/movie1.wav64:  AUDIOCONV_FLAGS += --wav-resample 28000

filesystem/sfx/journal.wav64:  AUDIOCONV_FLAGS += --wav-resample 20000