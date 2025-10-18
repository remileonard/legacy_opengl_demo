/*
	defs.h
	Drew Olbrich, 1992
*/

#ifndef _DEFS
#define _DEFS

#ifdef WIN32
#pragma warning (disable:4244)  /* disable nasty conversion warnings. */
#endif

#define WIN_TITLE   "distort"
#define ICON_TITLE  "distort"

#define DEFAULT_IMAGE_FN  "data/distort.rgb"

#define DEFAULT_EFFECT  ripple

#define WIN_SIZE_X  400
#define WIN_SIZE_Y  400

#define GRID_SIZE_X  32
#define GRID_SIZE_Y  32

#define CLIP_NEAR  0.0
#define CLIP_FAR   1000.0

/*
	The following structure defines functions unique to each
	distortion effect.  Depending on which distortion is
	selected, a different set of functions is called.

	Obviously this should all be written in C++.  Maybe.
*/

typedef struct {
  void (* init)();
  void (* dynamics)();
  void (* redraw)();
  void (* click)();
} EFFECT;

extern EFFECT ripple;
extern EFFECT rubber;

#endif
