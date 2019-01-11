TARGET		:= vitaHexenII
TITLE		:= HXEN00001
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include

LIBS = -lvitaGL -lvorbisfile -lvorbis -logg -lspeexdsp -lmpg123 \
	-lc -lpng -lz -lvita2d -lSceAudio_stub -lSceLibKernel_stub \
	-lSceCommonDialog_stub -lSceDisplay_stub -lSceGxm_stub \
	-lSceSysmodule_stub -lSceCtrl_stub -lSceTouch_stub -lm \
	-lSceRtc_stub -lScePgf_stub -ljpeg -lSceRtc_stub -lc -lScePower_stub

COMMON_OBJS =	source/chase.o \
	source/cl_demo.o \
	source/cl_input.o \
	source/cl_main.o \
	source/cl_parse.o \
	source/cl_tent.o \
	source/cl_effect.o \
	source/cmd.o \
	source/common.o \
	source/console.o \
	source/crc.o \
	source/cvar.o \
	source/host.o \
	source/host_cmd.o \
	source/keys.o \
	source/mathlib.o \
	source/menu.o \
	source/net_dgrm.o \
	source/net_loop.o \
	source/net_main.o \
	source/net_vcr.o \
	source/pr_cmds.o \
	source/pr_edict.o \
	source/pr_exec.o \
	source/r_part.o \
	source/sbar.o \
	source/sv_main.o \
	source/sv_move.o \
	source/sv_phys.o \
	source/sv_user.o \
	source/view.o \
	source/wad.o \
	source/world.o \
	source/zone.o \
	source/sys_psp2.o \
	source/d_edge.o \
	source/d_fill.o \
	source/d_init.o \
	source/d_modech.o \
	source/d_part.o \
	source/d_polyse.o \
	source/d_scan.o \
	source/d_sky.o \
	source/d_sprite.o \
	source/d_surf.o \
	source/d_vars.o \
	source/d_zpoint.o \
	source/draw.o \
	source/model.o \
	source/fnmatch.o \
	source/nonintel.o \
	source/r_aclip.o \
	source/r_alias.o \
	source/r_bsp.o \
	source/r_draw.o \
	source/r_edge.o \
	source/r_efrag.o \
	source/r_light.o \
	source/r_main.o \
	source/r_misc.o \
	source/r_sky.o \
	source/r_sprite.o \
	source/r_surf.o \
	source/r_vars.o \
	source/screen.o \
	source/snd_dma.o \
	source/snd_mix.o \
	source/snd_mem.o \
	source/snd_psp2.o \
	source/vid_psp2.o \
	source/net_none.o \
	source/in_psp2.o

CPPSOURCES	:= source/audiodec	

CFILES		:=	$(COMMON_OBJS)
CPPFILES   := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -fno-lto -Wl,-q -Wall -O3 -g -Did386="0" -DHAVE_OGGVORBIS -DHAVE_MPG123 -DHAVE_LIBSPEEXDSP -DUSE_AUDIO_RESAMPLER -DWANT_FMMIDI=1 -DQUAKE2RJ -DRJNET -fno-short-enums -ffast-math
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -s $< build\eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) "$(TARGET)" param.sfo
	cp -f param.sfo build/sce_sys/param.sfo
	
	#------------ Comment this if you don't have 7zip ------------------
	7z a -tzip ../$(TARGET).vpk -r build\sce_sys\* build\eboot.bin 
	#-------------------------------------------------------------------

%.velf: %.elf
	cp -f $< $@.unstripped.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS)

run: $(TARGET).velf
	@sh run_homebrew_unity.sh $(TARGET).velf
