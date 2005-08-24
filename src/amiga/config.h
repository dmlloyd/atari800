#ifndef CONFIG_H
#define CONFIG_H

/* Define to use back slash as directory separator. */
#undef BACK_SLASH

/* Target: standard I/O. */
#undef BASIC

/* Define to use buffered debug output. */
#undef BUFFERED_LOG

/* Define to allow sound clipping. */
#undef CLIP_SOUND

/* Define to 1 if the `closedir' function returns void instead of `int'. */
#undef CLOSEDIR_VOID

/* Define to allow console sound (keyboard clicks). */
#define CONSOLE_SOUND

/* Define to activate crash menu after CIM instruction. */
#define CRASH_MENU

/* Define to disable bitmap graphics emulation in CURSES target. */
#undef CURSES_BASIC

/* Define to allow color changes inside a scanline. */
#define CYCLE_EXACT

/* Alternate config filename due to 8+3 fs limit. */
#define DEFAULT_CFG_NAME "PROGDIR:Atari800.cfg"

/* Target: Windows with DirectX. */
#undef DIRECTX

/* Target: DOS VGA. */
#undef DOSVGA

/* Define to enable DOS style drives support. */
#undef DOS_DRIVES

/* Target: Atari Falcon system. */
#undef FALCON

/* Define to use m68k assembler CPU core for Falcon target. */
#undef FALCON_CPUASM

/* Define to 1 if you have the <arpa/inet.h> header file. */
#undef HAVE_ARPA_INET_H

/* Define to 1 if you have the `atexit' function. */
#define HAVE_ATEXIT

/* Define to 1 if you have the `chmod' function. */
#undef HAVE_CHMOD

/* Define to 1 if you have the `clock' function. */
#undef HAVE_CLOCK

/* Define to 1 if you have the <direct.h> header file. */
#undef HAVE_DIRECT_H

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
#undef HAVE_DOPRNT

/* Define to 1 if you have the <errno.h> header file. */
#undef HAVE_ERRNO_H

/* Define to 1 if you have the <fcntl.h> header file. */
#undef HAVE_FCNTL_H

/* Define to 1 if you have the `fdopen' function. */
#undef HAVE_FDOPEN

/* Define to 1 if you have the `fflush' function. */
#undef HAVE_FFLUSH

/* Define to 1 if you have the <file.h> header file. */
#undef HAVE_FILE_H

/* Define to 1 if you have the `floor' function. */
#undef HAVE_FLOOR

/* Define to 1 if you have the `fstat' function. */
#undef HAVE_FSTAT

/* Define to 1 if you have the `getcwd' function. */
#undef HAVE_GETCWD

/* Define to 1 if you have the `gethostbyaddr' function. */
#undef HAVE_GETHOSTBYADDR

/* Define to 1 if you have the `gethostbyname' function. */
#undef HAVE_GETHOSTBYNAME

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY

/* Define to 1 if you have the `inet_ntoa' function. */
#undef HAVE_INET_NTOA

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the `png' library (-lpng). */
#undef HAVE_LIBPNG

/* Define to 1 if you have the `z' library (-lz). */
#undef HAVE_LIBZ

/* Define to 1 if you have the `localtime' function. */
#undef HAVE_LOCALTIME

/* Define to 1 if you have the `memmove' function. */
#undef HAVE_MEMMOVE

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET

/* Define to 1 if you have the `mkdir' function. */
#undef HAVE_MKDIR

/* Define to 1 if you have the `mkstemp' function. */
#undef HAVE_MKSTEMP

/* Define to 1 if you have the `mktemp' function. */
#undef HAVE_MKTEMP

/* Define to 1 if you have the `modf' function. */
#undef HAVE_MODF

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
#undef HAVE_NDIR_H

/* Define to 1 if you have the <netdb.h> header file. */
#undef HAVE_NETDB_H

/* Define to 1 if you have the <netinet/in.h> header file. */
#undef HAVE_NETINET_IN_H

/* Define to 1 if you have the `opendir' function. */
#define HAVE_OPENDIR

/* Define to 1 if you have the `rename' function. */
#undef HAVE_RENAME

/* Define to 1 if you have the `rewind' function. */
#undef HAVE_REWIND

/* Define to 1 if you have the `rmdir' function. */
#undef HAVE_RMDIR

/* Define to 1 if you have the `select' function. */
#undef HAVE_SELECT

/* Define to 1 if you have the `signal' function. */
#undef HAVE_SIGNAL

/* Define to 1 if you have the <signal.h> header file. */
#undef HAVE_SIGNAL_H

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF

/* Define to 1 if you have the `socket' function. */
#undef HAVE_SOCKET

/* Define to 1 if you have the `stat' function. */
#undef HAVE_STAT

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
#undef HAVE_STAT_EMPTY_STRING_BUG

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H

/* Define to 1 if you have the `strcasecmp' function. */
#undef HAVE_STRCASECMP

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR

/* Define to 1 if you have the `strdup' function. */
#undef HAVE_STRDUP

/* Define to 1 if you have the `strerror' function. */
#undef HAVE_STRERROR

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H

/* Define to 1 if you have the `strrchr' function. */
#undef HAVE_STRRCHR

/* Define to 1 if you have the `strstr' function. */
#undef HAVE_STRSTR

/* Define to 1 if you have the `strtol' function. */
#undef HAVE_STRTOL

/* Define to 1 if you have the `system' function. */
#undef HAVE_SYSTEM

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
#undef HAVE_SYS_DIR_H

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#undef HAVE_SYS_IOCTL_H

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
#undef HAVE_SYS_NDIR_H

/* Define to 1 if you have the <sys/select.h> header file. */
#undef HAVE_SYS_SELECT_H

/* Define to 1 if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H

/* Define to 1 if you have the <sys/soundcard.h> header file. */
#undef HAVE_SYS_SOUNDCARD_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/time.h> header file. */
#undef HAVE_SYS_TIME_H

/* Define to 1 if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H

/* Define to 1 if you have the <termios.h> header file. */
#undef HAVE_TERMIOS_H

/* Define to 1 if you have the `time' function. */
#undef HAVE_TIME

/* Define to 1 if you have the <time.h> header file. */
#undef HAVE_TIME_H

/* Define to 1 if you have the `tmpnam' function. */
#undef HAVE_TMPNAM

/* Define to 1 if you have the `uclock' function. */
#undef HAVE_UCLOCK

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H

/* Define to 1 if you have the <unixio.h> header file. */
#undef HAVE_UNIXIO_H

/* Define to 1 if you have the `unlink' function. */
#undef HAVE_UNLINK

/* Define to 1 if you have the `vprintf' function. */
#undef HAVE_VPRINTF

/* Define to 1 if you have the `vsnprintf' function. */
#undef HAVE_VSNPRINTF

/* Define to allow sound interpolation. */
#undef INTERPOLATE_SOUND

/* Define to use LINUX joystick. */
#undef LINUX_JOYSTICK

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
#undef LSTAT_FOLLOWS_SLASHED_SYMLINK

/* Define to activate assembler in monitor. */
#undef MONITOR_ASSEMBLER

/* Define to activate BREAK command in monitor. */
#undef MONITOR_BREAK

/* Define to activate hints in disassembler. */
#undef MONITOR_HINTS

/* Target: X11 with Motif. */
#undef MOTIF

/* Define to allow new cycle-exact scanline routines. */
#define NEW_CYCLE_EXACT

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Define to use page-based attribute array. */
#undef PAGED_ATTRIB

/* Define as the return type of signal handlers (`int' or `void'). */
#undef RETSIGTYPE

/* Define to use R: device. */
#undef R_IO_DEVICE

/* Target: SDL library. */
#undef SDL

/* Define to the type of arg 1 for `select'. */
#undef SELECT_TYPE_ARG1

/* Define to the type of args 2, 3 and 4 for `select'. */
#undef SELECT_TYPE_ARG234

/* Define to the type of arg 5 for `select'. */
#undef SELECT_TYPE_ARG5

/* Define to allow serial in/out sound. */
#undef SERIO_SOUND

/* Target: X11 with shared memory extensions. */
#undef SHM

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* Define to activate sound support. */
#define SOUND

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS

/* Define to allow stereo sound. */
#undef STEREO_SOUND

/* Save additional config file options. */
#define SUPPORTS_ATARI_CONFIGSAVE

/* Additional config file options. */
#define SUPPORTS_ATARI_CONFIGURE

/* Target: Linux with SVGALib. */
#undef SVGALIB

/* Define to use Toshiba Joystick Mouse support. */
#undef SVGA_JOYMOUSE

/* Define for drawing every 1/50 sec only 1/refresh of screen. */
#undef SVGA_SPEEDUP

/* Alternate system-wide config file for non-Unix OS. */
#undef SYSTEM_WIDE_CFG_FILE

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#undef TIME_WITH_SYS_TIME

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
#undef TM_IN_SYS_TIME

/* Define if unaligned long access is ok. */
#undef UNALIGNED_LONG_OK

/* Define to use clock() instead of gettimeofday(). */
#undef USE_CLOCK

/* Target: Curses-compatible library. */
#undef USE_CURSES

/* Define for using cursor/ctrl keys for keyboard joystick. */
#undef USE_CURSORBLOCK

/* Target: Ncurses library. */
#undef USE_NCURSES

/* Define to use very slow computer support (faster -refresh). */
#undef VERY_SLOW

/* Define to allow volume only sound. */
#define VOL_ONLY_SOUND

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
#define WORDS_BIGENDIAN

/* Target: Standard X11. */
#undef X11

/* Target: X11 with XView. */
#undef XVIEW

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define as `__inline' if that's what the C compiler calls it, or to nothing
   if it is not supported. */
#undef inline

/* Define to `unsigned' if <sys/types.h> does not define. */
#undef size_t

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
#undef volatile






/*

#define HAVE_UNISTD_H
#define HAVE_GETTIMEOFDAY

#define MONITOR_BREAK

#define DEFAULT_CFG_NAME "PROGDIR:Atari800.cfg"

#define SUPPORTS_ATARI_CONFIGINIT
#define SUPPORTS_ATARI_CONFIGSAVE
#define SUPPORTS_ATARI_CONFIGURE
#define DONT_USE_RTCONFIGUPDATE

#define WORDS_BIGENDIAN
*/

//#define SIGNED_SAMPLES
/*#define STEREO*/
/*#define VERY_SLOW*/
/*#define SET_LED*/

/*#define SOUND
#define VOL_ONLY_SOUND
#define INTERPOLATE_SOUND
#define CONSOLE_SOUND
#define SERIO_SOUND

#define DO_DIR
#define NEW_CYCLE_EXACT
*/

void usleep(unsigned long usec);

#endif
