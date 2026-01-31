NAME = uncharted-terra-2264
 # Internal ROM name
ROM_NAME = "Uncharted Terra 2264"
SOURCE_DIR = src
GAME_SOURCE_DIR = game
BUILD_DIR = build
ASSETS_DIR = assets
FILESYSTEM_DIR = filesystem
T3D_INST=$(shell realpath ../..)

include $(N64_INST)/include/n64.mk
include $(N64_INST)/include/t3d.mk

include assetflags.mk

SRCS = $(shell find $(SOURCE_DIR)/ -type f -name '*.c') \
	   $(shell find $(SOURCE_DIR)/ -type f -name '*.cpp') \
		$(shell find $(SOURCE_DIR)/$(GAME_SOURCE_DIR)/ -type f -name '*.c') \
	    $(shell find $(SOURCE_DIR)/$(GAME_SOURCE_DIR)/ -type f -name '*.cpp')

IMAGE_LIST = $(shell find $(ASSETS_DIR)/models/ -type f -name '*.png') \
			 $(shell find $(ASSETS_DIR)/textures/ -type f -name '*.png')
FONT_LIST  = $(shell find $(ASSETS_DIR)/fonts/ -type f -name '*.ttf')
SOUND_LIST  = $(shell find $(ASSETS_DIR)/sfx/ -type f -name '*.wav')
MUSIC_LIST  = $(shell find $(ASSETS_DIR)/music/ -type f -name '*.wav')
XM_LIST  	= $(shell find $(ASSETS_DIR)/music/ -type f -name '*.xm')
MODELS_LIST = $(shell find $(ASSETS_DIR)/models/ -type f -name '*.glb')
TEXT_LIST   = $(shell find $(ASSETS_DIR)/scripts/ -type f -name '*.*')
MOVIES_LIST   = $(shell find $(ASSETS_DIR)/movies/ -type f -name '*.m1v')
ASSETS_LIST += $(subst $(ASSETS_DIR),$(FILESYSTEM_DIR),$(IMAGE_LIST:%.png=%.sprite))
ASSETS_LIST += $(subst $(ASSETS_DIR),$(FILESYSTEM_DIR),$(FONT_LIST:%.ttf=%.font64))
ASSETS_LIST += $(subst $(ASSETS_DIR),$(FILESYSTEM_DIR),$(SOUND_LIST:%.wav=%.wav64))
ASSETS_LIST += $(subst $(ASSETS_DIR),$(FILESYSTEM_DIR),$(MUSIC_LIST:%.wav=%.wav64))
ASSETS_LIST += $(subst $(ASSETS_DIR),$(FILESYSTEM_DIR),$(XM_LIST:%.xm=%.xm64))
ASSETS_LIST += $(subst $(ASSETS_DIR),$(FILESYSTEM_DIR),$(MODELS_LIST:%.glb=%.t3dm))
ASSETS_LIST += $(subst $(ASSETS_DIR),$(FILESYSTEM_DIR),$(TEXT_LIST))
ASSETS_LIST += $(subst $(ASSETS_DIR),$(FILESYSTEM_DIR),$(MOVIES_LIST))

N64_CFLAGS += -std=gnu2x
N64_C_AND_CXX_FLAGS += -I $(SOURCE_DIR) -I $(SOURCE_DIR)/$(GAME_SOURCE_DIR) -Os -DNDEBUG -ffunction-sections -fdata-sections -fmerge-constants -fomit-frame-pointer -ftrivial-auto-var-init=zero -fno-stack-protector -fno-ident -fvisibility=hidden -Wno-error=write-strings -Wno-error=narrowing -Wno-narrowing -Wno-write-strings -flto
N64_CXXFLAGS += -fno-rtti -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-threadsafe-statics -fno-use-cxa-atexit -flto
N64_LDFLAGS += --gc-sections -flto

all: $(NAME).z64

$(FILESYSTEM_DIR)/textures/%.sprite: $(ASSETS_DIR)/textures/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) --format AUTO $(MKSPRITE_FLAGS) --compress 1 -o $(dir $@) "$<"

$(FILESYSTEM_DIR)/models/%.sprite: $(ASSETS_DIR)/models/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) --mipmap NONE --compress 1 -o $(dir $@) "$<"

$(FILESYSTEM_DIR)/fonts/%.font64: $(ASSETS_DIR)/fonts/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) $(MKFONT_FLAGS) -o $(dir $@) "$<"

$(FILESYSTEM_DIR)/sfx/%.wav64: $(ASSETS_DIR)/sfx/%.wav
	@mkdir -p $(dir $@)
	@echo "    [SFX] $@"
	$(N64_AUDIOCONV)  --wav-compress 1,bits=3 --wav-resample 28000 $(AUDIOCONV_FLAGS) -o $(dir $@) "$<"

$(FILESYSTEM_DIR)/music/%.wav64: $(ASSETS_DIR)/music/%.wav
	@mkdir -p $(dir $@)
	@echo "    [MUSIC] $@"
	$(N64_AUDIOCONV) --wav-compress 1,bits=3 --wav-resample 28000 $(AUDIOCONV_FLAGS) -o $(dir $@) "$<"

$(FILESYSTEM_DIR)/music/%.xm64: $(ASSETS_DIR)/music/%.xm
	@mkdir -p $(dir $@)
	@echo "    [MUSIC] $@"
	$(N64_AUDIOCONV) $(AUDIOCONV_FLAGS) --xm-compress 1 -o $(dir $@) "$<"

$(FILESYSTEM_DIR)/%.t3dm: $(ASSETS_DIR)/%.glb
	@mkdir -p $(dir $@)
	@echo "    [T3D-MODEL] $@"
	$(T3D_GLTF_TO_3D) "$<" $@
	$(N64_BINDIR)/mkasset -c 1 -o $(dir $@) $@

$(FILESYSTEM_DIR)/scripts/%: $(ASSETS_DIR)/scripts/%
	@mkdir -p $(dir $@)
	@echo "    [TEXT] $@"
	$(N64_BINDIR)/mkasset -c 0 -o $(dir $@) "$<"

$(FILESYSTEM_DIR)/movies/%: $(ASSETS_DIR)/movies/%
	@mkdir -p $(dir $@)
	@echo "    [M1V] $@"
	$(N64_BINDIR)/mkasset -c 0 -o $(dir $@) "$<"

$(BUILD_DIR)/$(NAME).dfs: $(ASSETS_LIST)
$(BUILD_DIR)/$(NAME).elf: $(SRCS:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o) $(SRCS:$(SOURCE_DIR)/%.cpp=$(BUILD_DIR)/%.o) $(SRCS:$(SOURCE_DIR)/$(GAME_SOURCE_DIR)/%.c=$(BUILD_DIR)/$(GAME_SOURCE_DIR)/%.o) $(SRCS:$(SOURCE_DIR)/$(GAME_SOURCE_DIR)/%.cpp=$(BUILD_DIR)/$(GAME_SOURCE_DIR)/%.o)

$(NAME).z64: N64_ROM_TITLE=$(ROM_NAME)
$(NAME).z64: $(BUILD_DIR)/$(NAME).dfs
$(NAME).z64: N64_ROM_SAVETYPE = eeprom16k
$(NAME).z64: N64_ROM_METADATA=metadata/metadata.ini
$(NAME).z64: N64_METADATAFLAGS = --padding 1048576

rebuild:
	rm -rf $(BUILD_DIR)
	make all
	
clean:
	rm -rf $(BUILD_DIR) *.z64
	rm -rf $(FILESYSTEM_DIR)

build_lib:
	rm -rf $(BUILD_DIR) *.z64
	make -C $(T3D_INST)
	make all

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
