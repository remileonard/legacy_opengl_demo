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

#undef GEORGE_DEBUG

#ifdef GEORGE_DEBUG
#   include <stdio.h>
#endif

#include <math.h>
#include <stdio.h>
#include "tk.h"
#include "gl42ogl.h"

int windX, windY;

// Variables pour stocker la taille réelle de l'écran
int actualScreenWidth = YOUR_SCREEN_MAXX;
int actualScreenHeight = YOUR_SCREEN_MAXY;


void
getorigin (long *x, long *y) {
    *x = windX;
    *y = windY;
}


long
getvaluator (Device dev) {
/* gl4: origin in bottom right
 */
    long originX, originY;
    int mouseX, mouseY;
    long val;

/* cost in performance in repeated calls,
 *   but hey you want an easy port as possible
 */
    getorigin (&originX, &originY);
    switch (dev) {
	case MOUSEX:
	    tkGetMouseLoc (&mouseX, &mouseY);
	    val = mouseX + originX;
	    break;

	case MOUSEY:
	    tkGetMouseLoc (&mouseX, &mouseY);
	    // Utiliser actualScreenHeight au lieu de YOUR_SCREEN_MAXY hardcodé
	    val = actualScreenHeight - (mouseY + originY);
	    break;

	default:
	    fprintf (stderr, "unsupported device: %d\n", dev);
	    break;
    }
    return (val);

}


#define PI 3.141593

void
gl_sincos (GLfloat ang, float *sine, float *cosine) {
    float rads = ang * PI / 1800;
    *sine   = (float)sin (rads);
    *cosine = (float)cos (rads);
}


void
glGetMatrix (GLfloat mat[]) {
    
    short i;
    GLint mode, ptr;
    static GLfloat tmp[100];

    glGetIntegerv (GL_MATRIX_MODE, &mode);
    switch (mode) {
	case GL_MODELVIEW:
	    glGetIntegerv (GL_MODELVIEW_STACK_DEPTH, &ptr);
	    glGetFloatv (GL_MODELVIEW_MATRIX, tmp);
	    break;
	case GL_PROJECTION:
	    glGetIntegerv (GL_PROJECTION_STACK_DEPTH, &ptr);
	    glGetFloatv (GL_PROJECTION_MATRIX, tmp);
	    break;
	case GL_TEXTURE:
	    glGetIntegerv (GL_TEXTURE_STACK_DEPTH, &ptr);
	    glGetFloatv (GL_TEXTURE_MATRIX, tmp);
	    break;
	default:
	    fprintf (stderr, "unknown matrix mode: %d\n", mode);
	    break;
    }

    for (i = 0; i < 16; i++)
	mat[i] = tmp[i];

}


void
mapcolor (Colorindex index, short r, short g, short b) {
/* gl4 -> rgb = [1,255]
 * ogl -> rgb = [0,1]
 */
    tkSetOneColor (index, r/255.0, g/255.0, b/255.0);
}


// Fonction helper pour simuler glIndexi en mode RGB
// Récupère la couleur de la palette et l'applique avec glColor3f
void
glIndexi_compat(Colorindex index) {
    float r, g, b;
    tkGetColorRGB(index, &r, &g, &b);
    glColor3f(r, g, b);
}

void
glIndexf_compat(float index) {
    glIndexi_compat((Colorindex)index);
}


void
polf2i (long n, Icoord parray[][2]) {
      long i;

    glBegin (GL_POLYGON);
    for (i = 0; i < n; i++)
	glVertex2iv (parray[i]);
    glEnd();
}


void
polf2 (long n, Coord parray[][2]) {
      long i;

    glBegin (GL_POLYGON);
    for (i = 0; i < n; i++)
	glVertex2fv (parray[i]);
    glEnd();
}


void
polfi (long n, Icoord parray[][3]) {
      long i;

    glBegin (GL_POLYGON);
    for (i = 0; i < n; i++)
	glVertex3iv (parray[i]);
    glEnd();
}


void
polf (long n, Coord parray[][3]) {
      long i;

    glBegin (GL_POLYGON);
    for (i = 0; i < n; i++)
	glVertex3fv (parray[i]);
    glEnd();
}


void
poly2i (long n, Icoord parray[][2]) {
      long i;

    glBegin (GL_LINE_LOOP);
    for (i = 0; i < n; i++)
	glVertex2iv (parray[i]);
    glEnd();
}


void
poly2 (long n, Coord parray[][2]) {
      long i;

    glBegin (GL_LINE_LOOP);
    for (i = 0; i < n; i++)
	glVertex2fv (parray[i]);
    glEnd();
}


void
polyi (long n, Icoord parray[][3]) {
      long i;

    glBegin (GL_LINE_LOOP);
    for (i = 0; i < n; i++)
	glVertex3iv (parray[i]);
    glEnd();
}


void
poly (long n, Coord parray[][3]) {
      long i;

    glBegin (GL_LINE_LOOP);
    for (i = 0; i < n; i++)
	glVertex3fv (parray[i]);
    glEnd();
}
