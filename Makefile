#---------------------------------------------------------------------------------
# Nanny Diaries - 3DS build (devkitARM + libctru + citro2d)
#   make        -> NannyDiaries.3dsx
#   make cia    -> NannyDiaries.cia   (needs makerom + bannertool on PATH)
#---------------------------------------------------------------------------------
.SUFFIXES:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      := NannyDiaries
BUILD       := build
SOURCES     := source
DATA        := data
INCLUDES    := include
GRAPHICS    := gfx
ROMFS       := romfs

APP_TITLE       := Choices
APP_DESCRIPTION := 20 Y2K stories, your choices
APP_AUTHOR      := Kailyn
ICON            := meta/icon.png

ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS   := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) $(INCLUDE) -D__3DS__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS     := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS  := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT  := $(CURDIR)/$(TARGET)
export TOPDIR  := $(CURDIR)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export LD := $(CXX)
export OFILES   := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export APP_ICON := $(TOPDIR)/$(ICON)
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh --romfs=$(CURDIR)/$(ROMFS)

.PHONY: $(BUILD) clean all cia assets
all: $(BUILD)
$(BUILD): assets
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

# fonts/*.ttf -> romfs/fonts/*.bcfnt   and   art/portraits/*.png -> romfs/gfx/*.t3x
# (both optional: the game falls back to the system font / placeholder busts)
assets:
	@mkdir -p $(ROMFS)/fonts $(ROMFS)/gfx
	-@for f in fonts/*.ttf; do [ -f "$$f" ] && mkbcfnt -o $(ROMFS)/fonts/$$(basename $${f%.ttf}).bcfnt "$$f"; done; true
	-@for f in art/portraits/*.png; do [ -f "$$f" ] && tex3ds -f rgba8888 -z auto -o $(ROMFS)/gfx/$$(basename $${f%.png}).t3x "$$f"; done; true

cia: all
	bannertool makebanner -i meta/banner.png -a meta/banner.wav -o $(BUILD)/banner.bnr
	bannertool makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i meta/icon.png -o $(BUILD)/icon.icn
	makerom -f cia -o $(TARGET).cia -elf $(TARGET).elf -rsf meta/app.rsf -target t -exefslogo \
	        -icon $(BUILD)/icon.icn -banner $(BUILD)/banner.bnr -DAPP_ROMFS=$(ROMFS)

clean:
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).cia
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).3dsx : $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf  : $(OFILES)
-include $(DEPENDS)
endif
