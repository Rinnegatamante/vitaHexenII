TARGET		:= vitaHexenII
TITLE		:= HXEN00001
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	include

LIBS = -lvitaGL -lvorbisfile -lvorbis -logg -lspeexdsp -lmpg123 \
	-lc -lpng -lz -lvita2d -lSceAudio_stub -lSceLibKernel_stub \
	-lSceCommonDialog_stub -lSceDisplay_stub -lSceGxm_stub \
	-lSceSysmodule_stub -lSceCtrl_stub -lSceTouch_stub -lm -lSceMotion_stub \
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
	source/gl_draw.o \
	source/gl_mesh.o \
	source/gl_model.o \
	source/gl_refrag.o \
	source/gl_rlight.o \
	source/gl_rmain.o \
	source/gl_rmisc.o \
	source/gl_rsurf.o \
	source/gl_screen.o \
	source/gl_test.o \
	source/gl_warp.o \
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
	source/fnmatch.o \
	source/snd_dma.o \
	source/snd_mix.o \
	source/snd_mem.o \
	source/snd_psp2.o \
	source/net_none.o \
	source/in_psp2.o \
	source/gl_vidpsp2.o \
	source/neon_mathfun.o

CPPSOURCES	:= source/audiodec

CGSOURCES := shaders

CFILES   :=	$(COMMON_OBJS)
CPPFILES := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
CGFILES  := $(foreach dir,$(CGSOURCES), $(wildcard $(dir)/*.cg))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 
GXPFILES := $(CGFILES:.cg=.gxp) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -w -Wl,-q -O3 -g -Did386="0" -DGLQUAKE -DHAVE_OGGVORBIS \
	-DHAVE_MPG123 -DHAVE_LIBSPEEXDSP -DUSE_AUDIO_RESAMPLER -DWANT_FMMIDI=1 \
	-DQUAKE2RJ -DRJNET -fsigned-char -ffast-math -mtune=cortex-a9 -mfpu=neon \
	-fno-short-enums
CXXFLAGS  = $(CFLAGS) -fpermissive -fno-exceptions -std=gnu++11
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

shaders: $(GXPFILES)

%_f.gxp:
	psp2cgc -profile sce_fp_psp2 $(@:_f.gxp=_f.cg) -Wperf -fastprecision -O3 -o $@
	
%_v.gxp:
	psp2cgc -profile sce_vp_psp2 $(@:_v.gxp=_v.cg) -Wperf -fastprecision -O3 -o $@

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -s $< build\eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) -d ATTRIBUTE2=12 "$(TARGET)" param.sfo
	cp -f param.sfo build/sce_sys/param.sfo
	
	#------------ Comment this if you don't have 7zip ------------------
	7z a -tzip $(TARGET).vpk -r ./build/sce_sys ./build/eboot.bin ./build/shaders
	#-------------------------------------------------------------------

%.velf: %.elf
	cp -f $< $@.unstripped.elf
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS) $(TARGET).elf.unstripped.elf $(TARGET).vpk build/eboot.bin build/sce_sys/param.sfo ./param.sfo

run: $(TARGET).velf
	@sh run_homebrew_unity.sh $(TARGET).velf
