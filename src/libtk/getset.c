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

int tkGetColorMapSize(void)
{

    if (!xDisplay) {
	return 0;
    } else {
	return w.vInfoMain->colormap_size;
    }
}

/******************************************************************************/

void tkGetMouseLoc(int *x, int *y)
{
    int junk;

    *x = 0;
    *y = 0;
    XQueryPointer(xDisplay, w.wMain, (Window *)&junk, (Window *)&junk,
		  &junk, &junk, x, y, (unsigned int *)&junk);
}

/******************************************************************************/

void tkGetSystem(GLenum type, void *ptr)
{

    switch (type) {
      case TK_X_DISPLAY:
	ptr = (void *)xDisplay;
	break;
      case TK_X_WINDOW:
	ptr = (void *)w.wMain;
	break;
    }
}

/******************************************************************************/

void tkSetFogRamp(int density, int startIndex)
{
    XColor c[256];
    int rShift, gShift, bShift, intensity, fogValues, colorValues;
    int i, j, k;

    switch (w.vInfoMain->class) {
      case DirectColor:
	fogValues = 1 << density;
	colorValues = 1 << startIndex;
	for (i = 0; i < colorValues; i++) {
	    for (j = 0; j < fogValues; j++) {
		k = i * fogValues + j;
		intensity = i * fogValues + j * colorValues;
		if (intensity > w.vInfoMain->colormap_size) {
		    intensity = w.vInfoMain->colormap_size;
		}
		intensity = (intensity << 8) | intensity;
		rShift = ffs((unsigned int)w.vInfoMain->red_mask) - 1;
		gShift = ffs((unsigned int)w.vInfoMain->green_mask) - 1;
		bShift = ffs((unsigned int)w.vInfoMain->blue_mask) - 1;
		c[k].pixel = ((k << rShift) & w.vInfoMain->red_mask) |
			     ((k << gShift) & w.vInfoMain->green_mask) |
			     ((k << bShift) & w.vInfoMain->blue_mask);
		c[k].red = (unsigned short)intensity;
		c[k].green = (unsigned short)intensity;
		c[k].blue = (unsigned short)intensity;
		c[k].flags = DoRed | DoGreen | DoBlue;
	    }
	}
	XStoreColors(xDisplay, w.cMapMain, c, w.vInfoMain->colormap_size);
	break;
      case GrayScale:
      case PseudoColor:
	fogValues = 1 << density;
	colorValues = 1 << startIndex;
	for (i = 0; i < colorValues; i++) {
	    for (j = 0; j < fogValues; j++) {
		k = i * fogValues + j;
		intensity = i * fogValues + j * colorValues;
		if (intensity > w.vInfoMain->colormap_size) {
		    intensity = w.vInfoMain->colormap_size;
		}
		intensity = (intensity << 8) | intensity;
		c[k].pixel = k;
		c[k].red = (unsigned short)intensity;
		c[k].green = (unsigned short)intensity;
		c[k].blue = (unsigned short)intensity;
		c[k].flags = DoRed | DoGreen | DoBlue;
	    }
	}
	XStoreColors(xDisplay, w.cMapMain, c, w.vInfoMain->colormap_size);
	break;
    }

    XSync(xDisplay, 0);
}

/******************************************************************************/

void tkSetGreyRamp(void)
{
    XColor c[256];
    float intensity;
    int rShift, gShift, bShift, i;

    switch (w.vInfoMain->class) {
      case DirectColor:
	for (i = 0; i < w.vInfoMain->colormap_size; i++) {
	    intensity = (float)i / (float)w.vInfoMain->colormap_size *
			65535.0 + 0.5;
	    rShift = ffs((unsigned int)w.vInfoMain->red_mask) - 1;
	    gShift = ffs((unsigned int)w.vInfoMain->green_mask) - 1;
	    bShift = ffs((unsigned int)w.vInfoMain->blue_mask) - 1;
	    c[i].pixel = ((i << rShift) & w.vInfoMain->red_mask) |
			 ((i << gShift) & w.vInfoMain->green_mask) |
			 ((i << bShift) & w.vInfoMain->blue_mask);
	    c[i].red = (unsigned short)intensity;
	    c[i].green = (unsigned short)intensity;
	    c[i].blue = (unsigned short)intensity;
	    c[i].flags = DoRed | DoGreen | DoBlue;
	}
	XStoreColors(xDisplay, w.cMapMain, c, w.vInfoMain->colormap_size);
	break;
      case GrayScale:
      case PseudoColor:
	for (i = 0; i < w.vInfoMain->colormap_size; i++) {
	    intensity = (float)i / (float)w.vInfoMain->colormap_size *
			65535.0 + 0.5;
	    c[i].pixel = i;
	    c[i].red = (unsigned short)intensity;
	    c[i].green = (unsigned short)intensity;
	    c[i].blue = (unsigned short)intensity;
	    c[i].flags = DoRed | DoGreen | DoBlue;
	}
	XStoreColors(xDisplay, w.cMapMain, c, w.vInfoMain->colormap_size);
	break;
    }

    XSync(xDisplay, 0);
}

/******************************************************************************/

void tkSetOneColor(int index, float r, float g, float b)
{
    XColor c;
    int rShift, gShift, bShift;

    switch (w.vInfoMain->class) {
      case DirectColor:
	rShift = ffs((unsigned int)w.vInfoMain->red_mask) - 1;
	gShift = ffs((unsigned int)w.vInfoMain->green_mask) - 1;
	bShift = ffs((unsigned int)w.vInfoMain->blue_mask) - 1;
	c.pixel = ((index << rShift) & w.vInfoMain->red_mask) |
		  ((index << gShift) & w.vInfoMain->green_mask) |
		  ((index << bShift) & w.vInfoMain->blue_mask);
	c.red = (unsigned short)(r * 65535.0 + 0.5);
	c.green = (unsigned short)(g * 65535.0 + 0.5);
	c.blue = (unsigned short)(b * 65535.0 + 0.5);
	c.flags = DoRed | DoGreen | DoBlue;
	XStoreColor(xDisplay, w.cMapMain, &c);
	break;
      case GrayScale:
      case PseudoColor:
	if (index < w.vInfoMain->colormap_size) {
	    c.pixel = index;
	    c.red = (unsigned short)(r * 65535.0 + 0.5);
	    c.green = (unsigned short)(g * 65535.0 + 0.5);
	    c.blue = (unsigned short)(b * 65535.0 + 0.5);
	    c.flags = DoRed | DoGreen | DoBlue;
	    XStoreColor(xDisplay, w.cMapMain, &c);
	}
	break;
    }

    XSync(xDisplay, 0);
}

/******************************************************************************/

void tkSetOverlayMap(int size, float *rgb)
{
    XColor c;
    unsigned long *buf;
    int max, i;

    if (w.vInfoOverlay->class == PseudoColor) {
	max = (size > w.vInfoOverlay->colormap_size) ?
	      w.vInfoOverlay->colormap_size : size;
	buf = (unsigned long *)calloc(max, sizeof(unsigned long));
	XAllocColorCells(xDisplay, w.cMapOverlay, True, NULL, 0, buf, max-1);
	for (i = 1; i < max; i++) {
	    c.pixel = i;
	    c.red = (unsigned short)(rgb[i] * 65535.0 + 0.5);
	    c.green = (unsigned short)(rgb[size+i] * 65535.0 + 0.5);
	    c.blue = (unsigned short)(rgb[size*2+i] * 65535.0 + 0.5);
	    c.flags = DoRed | DoGreen | DoBlue;
	    XStoreColor(xDisplay, w.cMapOverlay, &c);
	}
	free(buf);
    }

    XSync(xDisplay, 0);
}

/******************************************************************************/

void tkSetRGBMap(int size, float *rgb)
{
    XColor c;
    int rShift, gShift, bShift, max, i;

    switch (w.vInfoMain->class) {
      case DirectColor:
	max = (size > w.vInfoMain->colormap_size) ? w.vInfoMain->colormap_size
						  : size;
	for (i = 0; i < max; i++) {
	    rShift = ffs((unsigned int)w.vInfoMain->red_mask) - 1;
	    gShift = ffs((unsigned int)w.vInfoMain->green_mask) - 1;
	    bShift = ffs((unsigned int)w.vInfoMain->blue_mask) - 1;
	    c.pixel = ((i << rShift) & w.vInfoMain->red_mask) |
		      ((i << gShift) & w.vInfoMain->green_mask) |
		      ((i << bShift) & w.vInfoMain->blue_mask);
	    c.red = (unsigned short)(rgb[i] * 65535.0 + 0.5);
	    c.green = (unsigned short)(rgb[size+i] * 65535.0 + 0.5);
	    c.blue = (unsigned short)(rgb[size*2+i] * 65535.0 + 0.5);
	    c.flags = DoRed | DoGreen | DoBlue;
	    XStoreColor(xDisplay, w.cMapMain, &c);
	}
	break;
      case GrayScale:
      case PseudoColor:
	max = (size > w.vInfoMain->colormap_size) ? w.vInfoMain->colormap_size
						  : size;
	for (i = 0; i < max; i++) {
	    c.pixel = i;
	    c.red = (unsigned short)(rgb[i] * 65535.0 + 0.5);
	    c.green = (unsigned short)(rgb[size+i] * 65535.0 + 0.5);
	    c.blue = (unsigned short)(rgb[size*2+i] * 65535.0 + 0.5);
	    c.flags = DoRed | DoGreen | DoBlue;
	    XStoreColor(xDisplay, w.cMapMain, &c);
	}
	break;
    }

    XSync(xDisplay, 0);
}

/******************************************************************************/

GLenum tkSetWindowLevel(GLenum level)
{

    switch (level) {
      case TK_OVERLAY:
	if (TK_HAS_OVERLAY(w.type)) {
	    if (!glXMakeCurrent(xDisplay, w.wOverlay, w.cOverlay)) {
		return GL_FALSE;
	    }
	} else {
	    return GL_FALSE;
	}
	break;
      case TK_RGB:
      case TK_INDEX:
	if (!glXMakeCurrent(xDisplay, w.wMain, w.cMain)) {
	    return GL_FALSE;
	}
	break;
    }
    return GL_TRUE;
}

/******************************************************************************/
