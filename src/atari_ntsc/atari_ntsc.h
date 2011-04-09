/* Atari TIA, CTIA, GTIA and MARIA NTSC video filter */

/* based on nes_ntsc 0.2.2 */
#ifndef ATARI_NTSC_H
#define ATARI_NTSC_H

#include "atari_ntsc_config.h"

#ifdef __cplusplus
	extern "C" {
#endif

/* Image parameters, ranging from -1.0 to 1.0. Actual internal values shown
in parenthesis and should remain fairly stable in future versions. */
typedef struct atari_ntsc_setup_t
{
	/* Basic parameters */
	double hue;        /* -1 = -180 degrees     +1 = +180 degrees */
	double saturation; /* -1 = grayscale (0.0)  +1 = oversaturated colors (2.0) */
	double contrast;   /* -1 = dark (0.5)       +1 = light (1.5) */
	double brightness; /* -1 = dark (0.5)       +1 = light (1.5) */
	double sharpness;  /* edge contrast enhancement/blurring */

	/* Advanced parameters */
	double gamma;      /* -1 = dark (1.5)       +1 = light (0.5) */
	double resolution; /* image resolution */
	double artifacts;  /* artifacts caused by color changes */
	double fringing;   /* color artifacts caused by brightness changes */
	double bleed;      /* color bleed (color resolution reduction) */
/* Atari change: no alternating burst phases - remove merge_fields field. */
	float const* decoder_matrix; /* optional RGB decoder matrix, 6 elements */
	
	unsigned char* palette_out;  /* optional RGB palette out, 3 bytes per color */
	
	/* You can replace the standard NES color generation with an RGB palette. The
	first replaces all color generation, while the second replaces only the core
	64-color generation and does standard color emphasis calculations on it. */
	unsigned char const* palette;/* optional 256-entry RGB palette in, 3 bytes per color */
/* Atari change: only one palette - remove base_palette field. */

/* Atari change: additional setup fields */
	double burst_phase; /* Phase at which colorburst signal is turned on;
	                      this defines colors of artifacts.
			      In radians; -1.0 = -180 degrees, 1.0 = +180 degrees */
	double *yiq_palette;
} atari_ntsc_setup_t;

/* Video format presets */
extern atari_ntsc_setup_t const atari_ntsc_composite; /* color bleeding + artifacts */
extern atari_ntsc_setup_t const atari_ntsc_svideo;    /* color bleeding only */
extern atari_ntsc_setup_t const atari_ntsc_rgb;       /* crisp image */
extern atari_ntsc_setup_t const atari_ntsc_monochrome;/* desaturated + artifacts */

enum { atari_ntsc_palette_size = 256 };

/* Initializes and adjusts parameters. Can be called multiple times on the same
atari_ntsc_t object. Can pass NULL for either parameter. */
typedef struct atari_ntsc_t atari_ntsc_t;
void atari_ntsc_init( atari_ntsc_t* ntsc, atari_ntsc_setup_t const* setup );

/* Filters one or more rows of pixels. Input pixels are 6/9-bit palette indicies.
In_row_width is the number of pixels to get to the next input row. Out_pitch
is the number of *bytes* to get to the next output row. Output pixel format
is set by ATARI_NTSC_OUT_DEPTH (defaults to 16-bit RGB). */
/* Atari change: no alternating burst phases - remove burst_phase parameter.
   Also removed the atari_ntsc_blit function and added specific blitters for various
   pixel formats. */
void atari_ntsc_blit_rgb16( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* atari_in,
		long in_row_width, int in_width, int in_height,
		void* rgb_out, long out_pitch );
void atari_ntsc_blit_bgr16( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* atari_in,
		long in_row_width, int in_width, int in_height,
		void* rgb_out, long out_pitch );
void atari_ntsc_blit_argb32( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* atari_in,
		long in_row_width, int in_width, int in_height,
		void* rgb_out, long out_pitch );
void atari_ntsc_blit_bgra32( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* atari_in,
		long in_row_width, int in_width, int in_height,
		void* rgb_out, long out_pitch );

/* Number of output pixels written by blitter for given input width. Width might
be rounded down slightly; use ATARI_NTSC_IN_WIDTH() on result to find rounded
value. Guaranteed not to round 256 down at all. */
#define ATARI_NTSC_OUT_WIDTH( in_width ) \
	((((in_width) - 1) / atari_ntsc_in_chunk + 1)* atari_ntsc_out_chunk)

/* Number of input pixels that will fit within given output width. Might be
rounded down slightly; use ATARI_NTSC_OUT_WIDTH() on result to find rounded
value. */
#define ATARI_NTSC_IN_WIDTH( out_width ) \
	(((out_width) / atari_ntsc_out_chunk - 1) * atari_ntsc_in_chunk + 1)


/* Interface for user-defined custom blitters. */

enum { atari_ntsc_in_chunk    = 4  }; /* number of input pixels read per chunk */
enum { atari_ntsc_out_chunk   = 7  }; /* number of output pixels generated per chunk */
enum { atari_ntsc_black       = 0  }; /* palette index for black */
enum { atari_ntsc_burst_count = 1  }; /* burst phase cycles through 0, 1, and 2 */

/* Begins outputting row and starts three pixels. First pixel will be cut off a bit.
Use nes_ntsc_black for unused pixels. Declares variables, so must be before first
statement in a block (unless you're using C++). */
/* Atari change: no alternating burst phases; adapted to 4/7 pixel ratio. */
#define ATARI_NTSC_BEGIN_ROW( ntsc, pixel0, pixel1, pixel2, pixel3 ) \
	char const* const ktable = \
		(char const*) (ntsc)->table [0];\
	ATARI_NTSC_BEGIN_ROW_8_( pixel0, pixel1, pixel2, pixel3, ATARI_NTSC_ENTRY_, ktable )


/* Begins input pixel */
#define ATARI_NTSC_COLOR_IN( in_index, color_in ) \
	ATARI_NTSC_COLOR_IN_( in_index, color_in, ATARI_NTSC_ENTRY_, ktable )

/* Generates output pixel. Bits can be 24, 16, 15, 32 (treated as 24), or 0:
24:          RRRRRRRR GGGGGGGG BBBBBBBB (8-8-8 RGB)
16:                   RRRRRGGG GGGBBBBB (5-6-5 RGB)
15:                    RRRRRGG GGGBBBBB (5-5-5 RGB)
 0: xxxRRRRR RRRxxGGG GGGGGxxB BBBBBBBx (native internal format; x = junk bits) */
#define ATARI_NTSC_RGB_OUT( index, rgb_out, bits ) \
	ATARI_NTSC_RGB_OUT_14_( index, rgb_out, bits, 0 )


/* private */
enum { atari_ntsc_entry_size = 56 };
typedef unsigned long atari_ntsc_rgb_t;
struct atari_ntsc_t {
	atari_ntsc_rgb_t table [atari_ntsc_palette_size] [atari_ntsc_entry_size];
};
enum { atari_ntsc_burst_size = atari_ntsc_entry_size / atari_ntsc_burst_count };

#define ATARI_NTSC_ENTRY_( ktable, n ) \
	(atari_ntsc_rgb_t const*) (ktable + (n) * (atari_ntsc_entry_size * sizeof (atari_ntsc_rgb_t)))

/* deprecated */
#define ATARI_NTSC_RGB24_OUT( x, out ) ATARI_NTSC_RGB_OUT( x, out, 24 )
#define ATARI_NTSC_RGB16_OUT( x, out ) ATARI_NTSC_RGB_OUT( x, out, 16 )
#define ATARI_NTSC_RGB15_OUT( x, out ) ATARI_NTSC_RGB_OUT( x, out, 15 )
#define ATARI_NTSC_RAW_OUT( x, out )   ATARI_NTSC_RGB_OUT( x, out,  0 )

enum { atari_ntsc_min_in_width   = 320 }; /* minimum width that doesn't cut off active area */
enum { atari_ntsc_min_out_width  = ATARI_NTSC_OUT_WIDTH( atari_ntsc_min_in_width ) };
	
enum { atari_ntsc_640_in_width  = 336 }; /* room for 8-pixel left & right overscan borders */
enum { atari_ntsc_640_out_width = ATARI_NTSC_OUT_WIDTH( atari_ntsc_640_in_width ) };
enum { atari_ntsc_640_overscan_left = 8 };
enum { atari_ntsc_640_overscan_right = atari_ntsc_640_in_width - atari_ntsc_min_in_width - atari_ntsc_640_overscan_left };

enum { atari_ntsc_full_in_width  = 384 }; /* room for full overscan */
enum { atari_ntsc_full_out_width = ATARI_NTSC_OUT_WIDTH( atari_ntsc_full_in_width ) };
enum { atari_ntsc_full_overscan_left = 32 };
enum { atari_ntsc_full_overscan_right = atari_ntsc_full_in_width - atari_ntsc_min_in_width - atari_ntsc_full_overscan_left };

/* common 4->7 ntsc macros */
/* Atari change: adapted to 4/7 pixel ratio. */
#define ATARI_NTSC_BEGIN_ROW_8_( pixel0, pixel1, pixel2, pixel3, ENTRY, table ) \
	unsigned const atari_ntsc_pixel0_ = (pixel0);\
	atari_ntsc_rgb_t const* kernel0  = ENTRY( table, atari_ntsc_pixel0_ );\
	unsigned const atari_ntsc_pixel1_ = (pixel1);\
	atari_ntsc_rgb_t const* kernel1  = ENTRY( table, atari_ntsc_pixel1_ );\
	unsigned const atari_ntsc_pixel2_ = (pixel2);\
	atari_ntsc_rgb_t const* kernel2  = ENTRY( table, atari_ntsc_pixel2_ );\
	unsigned const atari_ntsc_pixel3_ = (pixel3);\
	atari_ntsc_rgb_t const* kernel3  = ENTRY( table, atari_ntsc_pixel3_ );\
	atari_ntsc_rgb_t const* kernelx0;\
	atari_ntsc_rgb_t const* kernelx1 = kernel0;\
	atari_ntsc_rgb_t const* kernelx2 = kernel0;\
	atari_ntsc_rgb_t const* kernelx3 = kernel0

/* Atari change: adapted to 4/7 pixel ratio. */
#define ATARI_NTSC_RGB_OUT_14_( x, rgb_out, bits, shift ) {\
	atari_ntsc_rgb_t raw_ =\
		kernel0  [x  ] + kernel1  [(x+5)%7+14] + kernel2  [(x+3)%7+28] + kernel3  [(x+1)%7+42] +\
		kernelx0 [(x+7)%14] + kernelx1 [(x+5)%7+21] + kernelx2 [(x+3)%7+35] + kernelx3 [(x+1)%7+49];\
	ATARI_NTSC_CLAMP_( raw_, shift );\
	ATARI_NTSC_RGB_OUT_( rgb_out, bits, shift );\
}

/* common ntsc macros */
#define atari_ntsc_rgb_builder    ((1L << 21) | (1 << 11) | (1 << 1))
#define atari_ntsc_clamp_mask     (atari_ntsc_rgb_builder * 3 / 2)
#define atari_ntsc_clamp_add      (atari_ntsc_rgb_builder * 0x101)
#define ATARI_NTSC_CLAMP_( io, shift ) {\
	atari_ntsc_rgb_t sub = (io) >> (9-(shift)) & atari_ntsc_clamp_mask;\
	atari_ntsc_rgb_t clamp = atari_ntsc_clamp_add - sub;\
	io |= clamp;\
	clamp -= sub;\
	io &= clamp;\
}

#define ATARI_NTSC_COLOR_IN_( index, color, ENTRY, table ) {\
	unsigned color_;\
	kernelx##index = kernel##index;\
	kernel##index = (color_ = (color), ENTRY( table, color_ ));\
}

/* Atari change: modified ATARI_NTSC_RGB_OUT_ so its BITS parameter is
   no longer a straight number of bits, but an enumerated value. Then
   added a few additional bit formats. Also added the ATARI_NTSC_RGB_FORMAT
   enumerated values. */
enum {
	ATARI_NTSC_RGB_FORMAT_RGB16,
	ATARI_NTSC_RGB_FORMAT_BGR16,
	ATARI_NTSC_RGB_FORMAT_ARGB32,
	ATARI_NTSC_RGB_FORMAT_BGRA32,
	ATARI_NTSC_RGB_FORMAT_RGB15
};

/* x is always zero except in snes_ntsc library */
#define ATARI_NTSC_RGB_OUT_( rgb_out, bits, x ) {\
	if ( bits == ATARI_NTSC_RGB_FORMAT_RGB16 )\
		rgb_out = (raw_>>(13-x)& 0xF800)|(raw_>>(8-x)&0x07E0)|(raw_>>(4-x)&0x001F);\
	else if ( bits == ATARI_NTSC_RGB_FORMAT_BGR16 )\
		rgb_out = (raw_>>(24-x)& 0x001F)|(raw_>>(8-x)&0x07E0)|(raw_<<(7+x)&0xF800);\
	else if ( bits == ATARI_NTSC_RGB_FORMAT_ARGB32 )\
		rgb_out = (raw_>>(5-x)&0xFF0000)|(raw_>>(3-x)&0xFF00)|(raw_>>(1-x)&0xFF) | 0xFF000000;\
	else if ( bits == ATARI_NTSC_RGB_FORMAT_BGRA32 )\
		rgb_out = (raw_>>(13-x)&0xFF00)|(raw_<<(5+x)&0xFF0000)|(raw_<<(23+x)&0xFF000000) | 0xFF;\
	else if ( bits == ATARI_NTSC_RGB_FORMAT_RGB15 )\
		rgb_out = (raw_>>(14-x)& 0x7C00)|(raw_>>(9-x)&0x03E0)|(raw_>>(4-x)&0x001F);\
	else if ( bits == 0 )\
		rgb_out = raw_ << x;\
}

#ifdef __cplusplus
	}
#endif

#endif
