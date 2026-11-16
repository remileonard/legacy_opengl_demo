/*
 * Copyright 1993, 1995, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 *
 * UNPUBLISHED -- Rights reserved under the copyright laws of the United
 * States.   Use of a copyright notice is precautionary only and does not
 * imply publication or disclosure.
 *
 * U.S. GOVERNMENT RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in FAR 52.227.19(c)(2) or subparagraph (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS 252.227-7013 and/or
 * in similar or successor clauses in the FAR, or the DOD or NASA FAR
 * Supplement.  Contractor/manufacturer is Silicon Graphics, Inc.,
 * 2011 N. Shoreline Blvd. Mountain View, CA 94039-7311.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that (i) the above copyright notices and this
 * permission notice appear in all copies of the software and related
 * documentation, and (ii) the name of Silicon Graphics may not be
 * used in any advertising or publicity relating to the software
 * without the specific, prior written permission of Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SILICON GRAPHICS BE LIABLE FOR ANY SPECIAL,
 * INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY
 * THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 */


/*
 * gui.h
 *
 * $Revision: 1.23 $
 * $Date: 1995/11/22 14:34:26 $
 *
 */

#ifndef __GUI_H__
#define __GUI_H__

enum GUIID 
{
    GUI_AA,
    GUI_COLLIDE,
    GUI_CULL,
    GUI_ESKY,
    GUI_FAR,
    GUI_FILES,
    GUI_FOG,
    GUI_NEAR_FOG_RANGE,
    GUI_FAR_FOG_RANGE,
    GUI_FOV,
    GUI_FOV_CULL,
    GUI_GUI,
    GUI_HIGH_STYLE,
    GUI_LIGHTING,
    GUI_LOD_FADE,
    GUI_LOD_FADE_RANGE,
    GUI_LOD_SCALE,
    GUI_MSG,
    GUI_NEAR,
    GUI_PHASE,
    GUI_PIX_LIMIT,
    GUI_QUIT,
    GUI_REPOSITION,
    GUI_RESET_ALL,
    GUI_STATS,
    GUI_STRESS,
    GUI_STRESS_HIGH,
    GUI_STRESS_LOW,
    GUI_STRESS_MAX,
    GUI_STRESS_SCALE,
    GUI_TEXTURE,
    GUI_TOD,
    GUI_DRAW_STYLE,
    GUI_XFORMER, 
    GUI_CENTER, 
    GUI_SNAP,
    GUI_TREE,
    GUI_PANX,
    GUI_PANY,
    GUI_SCALE
};
typedef enum GUIID GUIId;

extern void 	initGUI(pfChannel *);
extern void	updateGUI(int);
extern void	updateGUIViewMsg(void);
extern void	updateWidget(int, float);

#endif
