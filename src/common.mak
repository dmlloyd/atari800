# $Id: common.mak,v 1.3 2001/04/15 09:14:33 knik Exp $

TARGET = atari800$(EXE)

CONFDEPS = config.h .atari800

ifeq (.atari800,$(wildcard .atari800))
include .atari800
endif

ifdef HAVE_LIBZ
LIBS += -lz
endif

OBJ = \
	atari.o \
	cpu.o \
	monitor.o \
	sio.o \
	devices.o \
	antic.o \
	gtia.o \
	pokey.o \
	pia.o \
	supercart.o \
	prompts.o \
	rt-config.o \
	ui.o \
	binload.o \
	list.o \
	ataripcx.o \
	log.o \
	compfile.o \
	memory.o \
	pbi.o \
	statesav.o \
	diskled.o \
	colours.o \
	pokeysnd.o \
	sndsave.o

%.o: %.c
	$(CC) -c -o $@ $(DEFS) $(INCLUDE) $(CFLAGS) $<

%.o: %.cpp
	$(CC) -c -o $@ $(DEFS) $(INCLUDE) $(CFLAGS) $<

config config.h .atari800: config.in
	$(MAKE) configure$(EXE)
	./configure

configure$(EXE): configure.o prompts.o
	$(CC) -o $@ $(LDFLAGS) configure.o prompts.o

$(TARGET): $(OBJ) $(CONFDEPS)
	$(CC) -o $@ $(LDFLAGS) $(OBJ) $(LIBS)

distclean: clean
	rm -f $(CONFDEPS) configure.o configure$(EXE)

dep:
	find . -name '*.c' -printf "%P\n" \
		| xargs makedepend -f common.mak $(DEFS) -Y


# DO NOT DELETE

amiga/async.o: amiga/async.h
antic.o: antic.h atari.h config.h cpu.h log.h gtia.h memory.h memory-d.h
antic.o: platform.h pokey.h rt-config.h statesav.h
atari.o: atari.h config.h cpu.h memory.h memory-d.h antic.h gtia.h pia.h
atari.o: pokey.h supercart.h devices.h sio.h monitor.h platform.h prompts.h
atari.o: rt-config.h ui.h ataripcx.h log.h statesav.h diskled.h colours.h
atari.o: binload.h
atari_amiga.o: config.h atari.h binload.h colours.h monitor.h pokeysnd.h
atari_amiga.o: sio.h statesav.h rt-config.h atari_amiga.h amiga/amiga_asm.h
atari_amiga.o: amiga/support.h
atari_basic.o: atari.h config.h monitor.h
atari_curses.o: atari.h config.h cpu.h monitor.h memory.h memory-d.h
atari_falcon.o: falcon/xcb.h falcon/res.h config.h cpu.h atari.h colours.h
atari_falcon.o: ui.h antic.h platform.h monitor.h log.h
atari_svgalib.o: config.h atari.h colours.h monitor.h sound.h platform.h
atari_svgalib.o: log.h
atari_vga.o: config.h cpu.h atari.h colours.h ui.h log.h dos/sound_dos.h
atari_vga.o: monitor.h pcjoy.h diskled.h antic.h dos/vga_gfx.h
atari_x11.o: config.h atari.h colours.h monitor.h sio.h sound.h platform.h
atari_x11.o: rt-config.h
ataripcx.o: antic.h atari.h config.h colours.h
binload.o: atari.h config.h log.h cpu.h memory-d.h pia.h
colours.o: atari.h config.h
compfile.o: atari.h config.h log.h
configure.o: prompts.h
cpu.o: atari.h config.h cpu.h memory.h memory-d.h statesav.h
devices.o: config.h ui.h atari.h cpu.h memory.h memory-d.h devices.h
devices.o: rt-config.h log.h binload.h sio.h
diskled.o: config.h
gtia.o: antic.h atari.h config.h gtia.h platform.h statesav.h
joycfg.o: config.h pcjoy.h
list.o: config.h list.h
log.o: config.h log.h
memory-d.o: atari.h config.h antic.h cpu.h gtia.h log.h memory.h memory-d.h
memory-d.o: pia.h rt-config.h statesav.h emuos.h
memory-p.o: atari.h config.h log.h rt-config.h antic.h cpu.h gtia.h pbi.h
memory-p.o: pia.h pokey.h supercart.h memory.h memory-d.h
memory.o: config.h memory-d.c atari.h antic.h cpu.h gtia.h log.h memory.h
memory.o: memory-d.h pia.h rt-config.h statesav.h emuos.h
mkimg.o: config.h
monitor.o: atari.h config.h cpu.h memory.h memory-d.h antic.h pia.h gtia.h
monitor.o: prompts.h
pbi.o: atari.h config.h
pia.o: atari.h config.h cpu.h memory.h memory-d.h pia.h platform.h sio.h
pia.o: log.h statesav.h
pokey.o: atari.h config.h cpu.h pia.h pokey.h gtia.h sio.h platform.h
pokey.o: statesav.h pokeysnd.h
pokeysnd.o: pokeysnd.h config.h atari.h sndsave.h
prompts.o: prompts.h
rt-config.o: atari.h config.h prompts.h rt-config.h
sio.o: atari.h config.h cpu.h memory.h memory-d.h sio.h pokeysnd.h platform.h
sio.o: log.h diskled.h binload.h
sndsave.o: sndsave.h atari.h config.h
sound.o: config.h
sound_falcon.o: config.h
statesav.o: atari.h config.h log.h
supercart.o: atari.h config.h memory.h memory-d.h log.h
ui.o: rt-config.h atari.h config.h cpu.h memory.h memory-d.h platform.h
ui.o: prompts.h gtia.h sio.h list.h ui.h log.h statesav.h antic.h ataripcx.h
ui.o: binload.h sndsave.h
dos/dos_sb.o: dos/dos_sb.h dos/dos_ints.h log.h config.h
dos/dossound.o: config.h atari.h pokeysnd.h
dos/sound_dos.o: config.h pokeysnd.h dos/dos_sb.h log.h
dos/vga_gfx.o: atari.h config.h
win32/atari_win32.o: config.h platform.h atari.h win32/screen.h
win32/atari_win32.o: win32/keyboard.h win32/main.h sound.h monitor.h
win32/keyboard.o: atari.h config.h win32/main.h win32/keyboard.h
win32/main.o: win32/main.h win32/screen.h atari.h config.h win32/keyboard.h
win32/main.o: sound.h
win32/screen.o: win32/screen.h atari.h config.h win32/main.h colours.h log.h
win32/sound.o: sound.h win32/main.h pokeysnd.h config.h atari.h log.h
