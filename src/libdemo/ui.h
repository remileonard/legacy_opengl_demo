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
 *	ui.h
 *  Written by Gavin Bell for Silicon Graphics, November 1988.
 *
 * This interface uses the LEFTMOUSE and MIDDLEMOUSE buttons to
 * implement 3D rotations and translations.  X-Y translation (pan) is
 * done using the left mousebutton.  Z translation (zoom) is done
 * using the left AND middle mouse buttons, held down together.  XYZ
 * rotations are done using the middle mouse button; the center of the
 * window becomes the center of a virtual trackball.
 *
 * The interface will automatically use the Spatial Systems Spaceball
 * 3D input device, if it is available.
 *
 * The interface uses the input-queue event handler program; see
 * 'event.h' for a description of those routines.  Also see
 * 'example.c' for a complete but simple example of how to use all of
 * these routines.
 */

#include "event.h"
#include "trackball.h"
#include "vect.h"

/*
 * The main routine takes one argument; the function to be called when
 * the user interacts with the interface.  This function will be
 * passed two arguments; a rotation quaternion, and a translation
 * vector.  See 'example.c' for an example application using this
 * interface.
 *
 * Since the user interface does several gl calls, a window must be
 * open and active when ui() is first called (this user interface will
 * respond only to events which happen in the window that is active
 * when ui() is first called).
 *
 * Note that this function does not terminate until ui_exit is called.
 */
void ui(void (*fn)(float *, float *));

/*
 *	Call ui_active(FALSE) to make the user interface become inactive.
 * Calling ui_active(TRUE) will make it active again.  This allows the
 * program to free-up the left and middle buttons for other tasks.
 */
void ui_active(int);

/*
 *	Call ui_exit when it is time for the interface to go away permanently.
 */
void ui_exit(void);

/*
 *	These two flags are always opposites, and keep track of if the
 * interface is being used or not (if the mouse buttons are being held
 * down and the object being dragged).
 */
extern int ui_noisy, ui_quiet;
