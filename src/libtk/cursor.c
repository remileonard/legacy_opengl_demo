/*
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tk.h"
#include "private.h"

/******************************************************************************/

#define MAX_CURSOR 32

typedef struct _cursorRec {
    GLint id;
    Cursor cursor;
} cursorRec;

int cursorNum = 0;
cursorRec cursors[MAX_CURSOR];

/******************************************************************************/

void tkNewCursor(GLint id, GLubyte *shapeBuf, GLubyte *maskBuf, GLenum fgColor,
		 GLenum bgColor, GLint hotX, GLint hotY)
{
    GLubyte buf[32];
    Pixmap shapeMap, maskMap;
    XColor c1, c2;
    int i;

    if (cursorNum == MAX_CURSOR-1) {
	return;
    }

    for (i = 0; i < 32; i += 2) {
	buf[i] = shapeBuf[i+1];
	buf[i+1] = shapeBuf[i];
    }
    shapeMap = XCreatePixmapFromBitmapData(xDisplay, wRoot, buf, 16, 16,
					   1, 0, 1);
    for (i = 0; i < 32; i += 2) {
	buf[i] = maskBuf[i+1];
	buf[i+1] = maskBuf[i];
    }
    maskMap = XCreatePixmapFromBitmapData(xDisplay, wRoot, buf, 16, 16,
					  1, 0, 1);
    c1.red = (unsigned short)(tkRGBMap[fgColor][0] * 65535.0 + 0.5);
    c1.green = (unsigned short)(tkRGBMap[fgColor][1] * 65535.0 + 0.5);
    c1.blue = (unsigned short)(tkRGBMap[fgColor][2] * 65535.0 + 0.5);
    c1.flags = DoRed | DoGreen | DoBlue;
    c2.red = (unsigned short)(tkRGBMap[bgColor][0] * 65535.0 + 0.5);
    c2.green = (unsigned short)(tkRGBMap[bgColor][1] * 65535.0 + 0.5);
    c2.blue = (unsigned short)(tkRGBMap[bgColor][2] * 65535.0 + 0.5);
    c2.flags = DoRed | DoGreen | DoBlue;

    cursors[cursorNum].id = id;
    cursors[cursorNum].cursor = XCreatePixmapCursor(xDisplay, shapeMap, maskMap,
					            &c1, &c2, hotX, hotY);
    cursorNum++;
}

/******************************************************************************/

void tkSetCursor(GLint id)
{
    int i;

    for (i = 0; i < cursorNum; i++) {
	if (cursors[i].id == id) {
	    XDefineCursor(xDisplay, w.wMain, cursors[i].cursor);
	}
    }
}

/******************************************************************************/
