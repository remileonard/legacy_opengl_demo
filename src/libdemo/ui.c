/*
 * Copyright 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */
/*
 *	A function (ui) taking one argument, which is a pointer to a
 * function to draw a 3-dimensional scene.  The function will be
 * passed an incremental quaternion rotation, and an incremental
 * translation, and should redraw the object in the new orientation
 * when called.
 *	Implemented by Gavin Bell, lots of ideas from Thant Tessman and
 * the August '88 issue of Siggraph's "Computer Graphics," pp. 121-129.
 *
 * See 'ui.h' for visible programmers interface.
 */
#include "ui.h"
#include "porting/iris2ogl.h"
#include <math.h>
#include <stdio.h>

/* Externally visible state: */
/* Is the user currently using the interface? */
int ui_quiet = TRUE;
int ui_noisy = FALSE;

/* States global to only this file */
static int exitflag = FALSE;   /* Becomes 1 when user quits */
static int activeflag = TRUE;  /* User interface active */
static int rotateflag = FALSE; /* When rotating with middle mouse */
static int zoomflag = FALSE;   /* Left AND Middle, zoom */
static int panflag = FALSE;    /* Left mouse, pan */
static int sbflag = FALSE;     /* Getting spaceball events */

/*
 * We mustn't call the user's redraw-the-scene function every time an
 * event occurs, or the event queue may overflow.  Therefore, these
 * two flags are used for two update functions, one of which is active
 * when the mouse is generating input (a mouse button is down) and one
 * when the spaceball is generating input (the spaceball sends an
 * all-zero event when it is done).  If both are true at the same
 * time, then the user's function will be called twice, and the two
 * devices will effectively battle it out.  No biggie.
 */
static int mouse_noisy = FALSE;     /* Interacting with mouse */
static int spaceball_noisy = FALSE; /* Interacting with spaceball */

/*
 * Window dimensions, used to convert mouse-clicks into a more
 * convenient coordinate system.
 */
static int sizex, sizey, origx, origy;

/*
 *	Function prototypes for the functions local to this file
 */
static void ui_init(void), ui_mouseupdate(void);
static void ui_sbupdate(void);
static void ui_to_worldspace(short, short, float *, float *);
static void ui_zoom(void), ui_pan(void), ui_redraw(void);
static void ui_lmdown(void), ui_lmup(void);
static void ui_mmdown(void), ui_mmup(void);
static void ui_SBevent(long, short);
static void ui_mousemotion();
static void ui_schedulemouse();

/*
 *	Used to remember what function was passed
 */
static void (*user_fn)(float *, float *);

/*
 *	And now....
 * The routines.
 *
 * This is the mail loop; it will call the supplied function (taking
 * two float * arguments) when necessary to update the object
 * rotations/translations and redraw the scene.
 */
void ui(void (*fn)(float *, float *)) {
    static int initialized = 0; /* Initialized yet? */

    user_fn = fn;
    if (!initialized) {
        ui_init();
        initialized = 1;
    }

    while (exitflag == 0) {
        /* All the action occurs in response to add_updates */
        event();
    }
}

static void ui_init() {
    long gid;

    gid = winget();
    getsize(&sizex, &sizey);   /* Gotta know where center of */
    getorigin(&origx, &origy); /* window is */

    if (activeflag) {
        qdevice(REDRAW); /* Keep track of window size changes */
        qdevice(LEFTMOUSE);
        qdevice(MIDDLEMOUSE);
        qdevice(CURSORX);
        qdevice(CURSORY);

        add_event(ANY, REDRAW, gid, ui_redraw, NULL);
        add_event(ANY, LEFTMOUSE, DOWN, ui_lmdown, NULL);
        add_event(ANY, LEFTMOUSE, UP, ui_lmup, NULL);
        add_event(ANY, MIDDLEMOUSE, DOWN, ui_mmdown, NULL);
        add_event(ANY, MIDDLEMOUSE, UP, ui_mmup, NULL);
        add_event(ANY, CURSORX, ANY, ui_schedulemouse, NULL);
        add_event(ANY, CURSORY, ANY, ui_schedulemouse, NULL);

        add_update(&mouse_noisy, ui_mouseupdate, NULL);

        add_event(ANY, SBTX, ANY, ui_SBevent, (void *)SBTX);
        add_event(ANY, SBTY, ANY, ui_SBevent, (void *)SBTY);
        add_event(ANY, SBTZ, ANY, ui_SBevent, (void *)SBTZ);
        add_event(ANY, SBRX, ANY, ui_SBevent, (void *)SBRX);
        add_event(ANY, SBRY, ANY, ui_SBevent, (void *)SBRY);
        add_event(ANY, SBRZ, ANY, ui_SBevent, (void *)SBRZ);
        add_event(ANY, SBPERIOD, ANY, ui_SBevent, (void *)SBPERIOD);
        qdevice(SBTX);
        qdevice(SBTY);
        qdevice(SBTZ);
        qdevice(SBRX);
        qdevice(SBRY);
        qdevice(SBRZ);
        qdevice(SBPERIOD);

        add_update(&spaceball_noisy, ui_sbupdate, NULL);
    }
}

/*
 * These keep track of the mouse position on the screen.  They are
 * initialized to (-1) so we can tell when the user first starts
 * interacting.  They are reset to (-1) when the mouse buttons go up.
 */
static short omx = (-1), omy = (-1), nmx = (-1), nmy = (-1);

/*
 * This function is repeatedly called as the user manipulates the
 * mouse.  It is only called if the mouse moves, however.
 */
static void ui_mouseupdate() {
    nmx = getvaluator(CURSORX);
    if (omx == (-1))
        omx = nmx;
    nmy = getvaluator(CURSORY);
    if (omy == (-1))
        omy = nmy;

    if (panflag)
        ui_pan();
    else if (zoomflag)
        ui_zoom();
    else if (rotateflag) {
        float p1x, p1y, p2x, p2y;
        float r[4], t[3];

        vzero(t);

        ui_to_worldspace(omx - origx, omy - origy, &p1x, &p1y);
        ui_to_worldspace(nmx - origx, nmy - origy, &p2x, &p2y);
        trackball(r, p1x, p1y, p2x, p2y);
        (*user_fn)(r, t);
    }
    omx = nmx;
    omy = nmy;

    mouse_noisy = FALSE; /* Update done */
}

static void ui_schedulemouse() { mouse_noisy = TRUE; }

/*
 *	Map mouse click sx, sy to a more convenient (-1.0,1.0)
 * range, based on window size.
 */
static void ui_to_worldspace(short sx, short sy, float *wx, float *wy) {
    (*wx) = (2.0 * sx) / (float)sizex - 1.0;
    (*wy) = (2.0 * sy) / (float)sizey - 1.0;
}

/*
 *	Zoom in/out; a translation of 1.0 is equal to a full sweep across
 * the window-- the user's function must scale accordingly.
 */
static void ui_zoom() {
    float r[4], t[3];

    vzero(r);
    r[3] = 1.0;
    vzero(t);

    t[2] = (float)(nmx - omx) / (float)sizex + (float)(nmy - omy) / (float)sizey;

    (*user_fn)(r, t);
}

/*
 *	Translate in xy plane.  The window is assumed to be unit-sized in
 * the x and y directions; the values returned must be scaled
 * accordingly.
 */
static void ui_pan() {
    float r[4], t[3];

    vzero(r);
    r[3] = 1.0;
    vset(t, (float)(nmx - omx) / (float)sizex, (float)(nmy - omy) / (float)sizey, 0.0);

    (*user_fn)(r, t);
}

/*
 *	Called in case of REDRAW events to keep track of window size.
 */
static void ui_redraw() {
    reshapeviewport();
    getsize(&sizex, &sizey);
    getorigin(&origx, &origy);
}

void ui_exit() { exitflag = 1; }

static void figure_ui_noisy() {
    ui_noisy = zoomflag | panflag | rotateflag | sbflag;
    ui_quiet = !ui_noisy;
}

void ui_active(int flag) {
    activeflag = flag;
    if (!flag) {
        zoomflag = panflag = rotateflag = sbflag = FALSE;

        figure_ui_noisy();
    }
}

static void ui_lmdown() {
    if (activeflag) {
        if (rotateflag == TRUE) {
            zoomflag = TRUE;
            rotateflag = FALSE;
        } else {
            panflag = TRUE;
            mouse_noisy = TRUE;
            figure_ui_noisy();
        }
    }
}

static void ui_lmup() {
    if (activeflag) {
        if (zoomflag == TRUE) {
            zoomflag = FALSE;
            rotateflag = TRUE;
        } else {
            panflag = FALSE;
            mouse_noisy = FALSE;
            figure_ui_noisy();
            omx = omy = nmx = nmy = (-1);
        }
    }
}

static void ui_mmdown() {
    if (activeflag) {
        if (panflag == TRUE) {
            zoomflag = TRUE;
            panflag = FALSE;
        } else {
            rotateflag = TRUE;
            mouse_noisy = TRUE;
            figure_ui_noisy();
        }
    }
}

static void ui_mmup() {
    if (activeflag) {
        if (zoomflag == TRUE) {
            zoomflag = FALSE;
            panflag = TRUE;
        } else {
            rotateflag = FALSE;
            mouse_noisy = FALSE;
            figure_ui_noisy();
            omx = omy = nmx = nmy = (-1);
        }
    }
}

/*
 *---------------------------
 * Support for the SpaceBall
 *---------------------------
 */

/*
 * SBaxistoquat will take an axis rotation vector and create an equivalent
 * quaternion.
 */
static void SBaxistoquat(float scale, float rvec[3], float q[4]) {
    float rads;

    /* Find the length of the vector */
    rads = sqrt(rvec[0] * rvec[0] + rvec[1] * rvec[1] + rvec[2] * rvec[2]);

    /* If the vector has zero length - return the identity matrix */
    if (fabs(rads) < 0.001) {
        q[0] = 0.0;
        q[1] = 0.0;
        q[2] = 0.0;
        q[3] = 1.0;

        return;
    }

    axis_to_quat(rvec, rads * scale / 2.0, q);
}

static float t_rate = 0.00000005;
static float r_rate = 0.0000005;

/*
 * These keep track of the spaceball translation and rotation.
 */
static float txyz[3], rxyz[3], period;

static void ui_sbupdate() {
    float tran[3], rot[4];

    tran[0] = t_rate * period * txyz[0];
    tran[1] = t_rate * period * txyz[1];
    tran[2] = t_rate * period * txyz[2];

    SBaxistoquat(-r_rate * period, rxyz, rot);

    (*user_fn)(rot, tran);

    spaceball_noisy = FALSE; /* Update done */
}

/*
 * Translate spaceball events into rotations/translations.
 * This is a very simplistic way of handling the spaceball.
 *
 * Note: The spaceball seems to be in a left-handed coordinate system;
 * hence the negatives below.  Then again, my math might be backwards
 * (it has happened before!).
 *
 */
static void ui_SBevent(long event, short val) {
    static int all_zero = 0;
    static int have_all = 0;
    int bit;

    switch (event) {
    case SBTX:
        txyz[0] = (float)val;
        bit = 0x1;
        break;
    case SBTY:
        txyz[1] = (float)val;
        bit = 0x2;
        break;
    case SBTZ:
        txyz[2] = -(float)val;
        bit = 0x4;
        break;
    case SBRX:
        rxyz[0] = (float)val;
        bit = 0x8;
        break;
    case SBRY:
        rxyz[1] = (float)val;
        bit = 0x10;
        break;
    case SBRZ:
        rxyz[2] = -(float)val;
        bit = 0x20;
        break;
    case SBPERIOD:
        period = (float)val;
        bit = 0x40;
        break;
    }
    if (val == 0)
        all_zero |= bit;
    /*
     * have_all lets us figure out when we've got all 7 spaceball
     * events, and can go ahead and redraw.
     */
    have_all |= bit;
    if (have_all == 0x7F) {
        /*
         * If all spaceball events were zero, then the spaceball is no
         * longer being interacted with.  If we had noticed that we
         * were interacting, notice that we aren't any more!
         */
        if ((all_zero == 0x3F) && sbflag) {
            spaceball_noisy = FALSE;
            sbflag = FALSE;
            figure_ui_noisy();
            /*
             * Let ui_sbupdate call the user's function to let them
             * the spaceball has stopped moving.
             */
            ui_sbupdate();
        }
        /*
         * If they were not all zero, and we haven't already noticed
         * that we are getting spaceball events, notice!
         */
        if (all_zero != 0x3F) {
            spaceball_noisy = TRUE; /* Turn update on */
            sbflag = TRUE;
            figure_ui_noisy();
        }

        have_all = 0;
        all_zero = 0;
    }
}
