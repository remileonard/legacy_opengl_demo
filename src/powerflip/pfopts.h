/*
 * pfopts.h - the options and menus that powerflip adds to flip: background,
 * fog, clipping planes, motion blur, wireframe while moving, no lighting,
 * materials, position and orientation of the models, title messages.
 */
#ifndef PFOPTS_H
#define PFOPTS_H

#include "powerflip.h"

extern int pf_wireframe;        /* -w: the models are wireframe */
extern int pf_move_wireframe;   /* -W: wireframe while moving */
extern int pf_nolighting;       /* -L: lines model, no lighting */
extern int pf_redraw;           /* -r: continuous redraw */
extern int pf_material;         /* -m: material of the models, -1 for flip's */
extern int pf_backmaterial;     /* -m: material of the back faces, -1 for none */
extern int pf_have_position, pf_have_orientation;
extern float pf_position[3];    /* -p */
extern float pf_orientation[4]; /* -o, a quaternion */
extern int pf_fullscreen;       /* -R: root window */
extern flipobj pf_clipobj;      /* the frame of the clipping plane */

/* an option of the command line, 0 when it is not one of powerflip */
int pf_option(int c, const char *arg);
/* to call before the window is opened (accumulation buffer) */
void pf_before_window(void);
/* the title messages while the models are loaded */
void pf_title_message(void);
/* fog, clipping planes, background, once the window is opened */
void pf_init_scene(void);

/* the clear color, packed for czclear */
unsigned long pf_clear_color(void);
void pf_draw_background(void);
/* clipping of the opaque (pass 0) and transparent (pass 1) objects */
void pf_clip_begin(int pass);
void pf_clip_end(void);
/* motion blur of the frame just drawn */
void pf_blur(void);
/* something changes from frame to frame without moving */
int pf_animating(void);
/* the drawing style of an object of the given type */
int pf_draw_style(int type);

/* the entries of the main menu */
void pf_add_menus(long menu);
void pf_init_menus(void);

#endif
