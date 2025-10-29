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
#include <GL/gl.h>

/* screen == monitor */
#define YOUR_SCREEN_MAXX    1023
#define YOUR_SCREEN_MAXY     766

#define YOUR_WINDOW_PADX       8
#define YOUR_WINDOW_PADY      32

#define MOUSEX 266
#define MOUSEY 267


typedef unsigned short Colorindex;
typedef unsigned short Device;

typedef GLfloat Coord;
typedef GLint Icoord;

extern void
getorigin (long *x, long *y);
/* no origin redoing is done */

extern void
gl_sincos (GLfloat ang, float *sine, float *cosine);
/* ang is in tenths of degrees */

extern long
getvaluator (Device dev);
/* gl4: origin in bottom right
 *  tk: assumes origin in top left
 */

extern void
glGetMatrix (GLfloat mat[]);

#define maxDEPTH 4294967295.0

#define lsetdepth(ln, lf) \
    glDepthRange ( ln / maxDEPTH, lf / maxDEPTH)


extern void
mapcolor (Colorindex index, short r, short g, short b);

extern void
polf2i (long n, Icoord parray[][2]);

extern void
polf2 (long n, Coord parray[][2]);

extern void
polfi (long n, Icoord parray[][3]);

extern void
polf (long n, Coord parray[][3]);

extern void
poly2i (long n, Icoord parray[][2]);

extern void
poly2 (long n, Coord parray[][2]);

extern void
polyi (long n, Icoord parray[][3]);

extern void
poly (long n, Coord parray[][3]);
