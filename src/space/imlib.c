/*
 * Copyright (C) 1992, 1993, 1994, Silicon Graphics, Inc.
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
#include "image_compat.h"

#include <stdint.h>
#include <windows.h>
#include <GL/gl.h>

typedef short sint16;
typedef unsigned short uint16;
typedef long sint32;
typedef unsigned long uint32;
typedef float flot32;
typedef double flot64;

void savescreen(uint32, uint32);
sint32 imgtolrect(char *);
void lrecttoimg(char *, sint32, sint32, sint32, sint32, uint32 *);

static void sizeofimage(char *name, sint32 *xsize, sint32 *ysize);
static void bwtocpack(uint16 *b, uint32 *l, sint32 n);
static void rgbtocpack(uint16 *r, uint16 *g, uint16 *b, uint32 *l, sint32 n);
static void rgbatocpack(uint16 *r, uint16 *g, uint16 *b, uint16 *a, uint32 *l, sint32 n);
static void cpacktorgba(uint32 *l, uint16 *r, uint16 *g, uint16 *b, uint16 *a, sint32 n);

/**********************************************************************
 *  savescreen()  -
 **********************************************************************/
void savescreen(uint32 dx, uint32 dy)

{
    uint32 *ptr;

    if ((ptr = (uint32 *)malloc(4 * dx * dy)) == 0) {
        printf("screendump malloc failed\n");
        exit(0);
    }

    glViewport(0, 0, dx, dy);

    glReadPixels(0, 0, dx, dy, GL_RGBA, GL_UNSIGNED_BYTE, ptr);

    lrecttoimg("image.rgb", 0, 0, dx - 1, dy - 1, ptr);

    free(ptr);
}

/***********************************************************************
 *  sizeofimage()  -
 **********************************************************************/
static void sizeofimage(char *name, long *xsize, long *ysize) {
    IMAGE *image;

    image = iopen(name, "r");
    if (!image) {
        fprintf(stderr, "sizeofimage: can't open image file %s\n", name);
        exit(1);
    }
    *xsize = image->xsize;
    *ysize = image->ysize;
    iclose(image);
}

/**********************************************************************
 *  imgtolrect()  -
 **********************************************************************/
sint32 imgtolrect(char *name) {
    uint32 *base, *lptr;
    uint16 *rbuf, *gbuf, *bbuf, *abuf;
    IMAGE *image;
    sint32 y;

    image = iopen(name, "r");
    if (!image) {
        fprintf(stderr, "longimagedata: can't open image file %s\n", name);
        exit(1);
    }
    base = (unsigned long *)malloc(image->xsize * image->ysize * sizeof(unsigned long));
    rbuf = (uint16 *)malloc(image->xsize * sizeof(short));
    gbuf = (uint16 *)malloc(image->xsize * sizeof(short));
    bbuf = (uint16 *)malloc(image->xsize * sizeof(short));
    abuf = (uint16 *)malloc(image->xsize * sizeof(short));
    if (!base || !rbuf || !gbuf || !bbuf) {
        fprintf(stderr, "longimagedata: can't malloc enough memory\n");
        exit(1);
    }
    lptr = base;
    for (y = 0; y < image->ysize; y++) {
        if (image->zsize >= 4) {
            getrow(image, rbuf, y, 0);
            getrow(image, gbuf, y, 1);
            getrow(image, bbuf, y, 2);
            getrow(image, abuf, y, 3);
            rgbatocpack(rbuf, gbuf, bbuf, abuf, lptr, image->xsize);
            lptr += image->xsize;
        } else if (image->zsize == 3) {
            getrow(image, rbuf, y, 0);
            getrow(image, gbuf, y, 1);
            getrow(image, bbuf, y, 2);
            rgbtocpack(rbuf, gbuf, bbuf, lptr, image->xsize);
            lptr += image->xsize;
        } else {
            getrow(image, rbuf, y, 0);
            bwtocpack(rbuf, lptr, image->xsize);
            lptr += image->xsize;
        }
    }
    iclose(image);
    free(rbuf);
    free(gbuf);
    free(bbuf);
    free(abuf);
    return ((intptr_t)base);
}

/**********************************************************************
 *  bwtocpack()  -
 **********************************************************************/
static void bwtocpack(uint16 *b, uint32 *l, sint32 n) {
    while (n >= 8) {
        l[0] = 0x00010101 * b[0];
        l[1] = 0x00010101 * b[1];
        l[2] = 0x00010101 * b[2];
        l[3] = 0x00010101 * b[3];
        l[4] = 0x00010101 * b[4];
        l[5] = 0x00010101 * b[5];
        l[6] = 0x00010101 * b[6];
        l[7] = 0x00010101 * b[7];
        l += 8;
        b += 8;
        n -= 8;
    }
    while (n--)
        *l++ = 0x00010101 * (*b++);
}

/**********************************************************************
 *  rgbtocpack()  -
 **********************************************************************/
static void rgbtocpack(uint16 *r, uint16 *g, uint16 *b, uint32 *l, sint32 n) {
    while (n >= 8) {
        l[0] = r[0] | (g[0] << 8) | (b[0] << 16);
        l[1] = r[1] | (g[1] << 8) | (b[1] << 16);
        l[2] = r[2] | (g[2] << 8) | (b[2] << 16);
        l[3] = r[3] | (g[3] << 8) | (b[3] << 16);
        l[4] = r[4] | (g[4] << 8) | (b[4] << 16);
        l[5] = r[5] | (g[5] << 8) | (b[5] << 16);
        l[6] = r[6] | (g[6] << 8) | (b[6] << 16);
        l[7] = r[7] | (g[7] << 8) | (b[7] << 16);
        l += 8;
        r += 8;
        g += 8;
        b += 8;
        n -= 8;
    }
    while (n--)
        *l++ = *r++ | ((*g++) << 8) | ((*b++) << 16);
}

/**********************************************************************
 *  rgbatocpack()  -
 **********************************************************************/
static void rgbatocpack(uint16 *r, uint16 *g, uint16 *b, uint16 *a, uint32 *l, sint32 n) {
    while (n >= 8) {
        l[0] = r[0] | (g[0] << 8) | (b[0] << 16) | (a[0] << 24);
        l[1] = r[1] | (g[1] << 8) | (b[1] << 16) | (a[1] << 24);
        l[2] = r[2] | (g[2] << 8) | (b[2] << 16) | (a[2] << 24);
        l[3] = r[3] | (g[3] << 8) | (b[3] << 16) | (a[3] << 24);
        l[4] = r[4] | (g[4] << 8) | (b[4] << 16) | (a[4] << 24);
        l[5] = r[5] | (g[5] << 8) | (b[5] << 16) | (a[5] << 24);
        l[6] = r[6] | (g[6] << 8) | (b[6] << 16) | (a[6] << 24);
        l[7] = r[7] | (g[7] << 8) | (b[7] << 16) | (a[7] << 24);
        l += 8;
        r += 8;
        g += 8;
        b += 8;
        a += 8;
        n -= 8;
    }
    while (n--)
        *l++ = *r++ | ((*g++) << 8) | ((*b++) << 16) | ((*a++) << 24);
}

/**********************************************************************
 *  lrecttoimg()  -
 **********************************************************************/
void lrecttoimg(char *name, sint32 x1, sint32 y1, sint32 x2, sint32 y2, uint32 *lbuf)

{
    sint32 xsize, ysize, y;
    IMAGE *oimage;
    uint16 *rbuf, *gbuf, *bbuf, *abuf;

    xsize = x2 - x1 + 1;
    ysize = y2 - y1 + 1;
    rbuf = (uint16 *)malloc(xsize * sizeof(short));
    gbuf = (uint16 *)malloc(xsize * sizeof(short));
    bbuf = (uint16 *)malloc(xsize * sizeof(short));
    abuf = (uint16 *)malloc(xsize * sizeof(short));
    oimage = iopen(name, "w", RLE(1), 3, xsize, ysize, 4);
    if (!oimage) {
        fprintf(stderr, "lrecttoimage: can't open output file\n");
        exit(1);
    }
    for (y = 0; y < ysize; y++) {
        cpacktorgba(lbuf, rbuf, gbuf, bbuf, abuf, xsize);
        putrow(oimage, rbuf, y, 0);
        putrow(oimage, gbuf, y, 1);
        putrow(oimage, bbuf, y, 2);
        putrow(oimage, abuf, y, 3);
        lbuf += xsize;
    }
    iclose(oimage);
    free(lbuf);
    free(rbuf);
    free(gbuf);
    free(bbuf);
}

#define CPACKTORGBA(l, r, g, b, a)                                                                                     \
    val = (l);                                                                                                         \
    (r) = (val >> 0) & 0xff;                                                                                           \
    (g) = (val >> 8) & 0xff;                                                                                           \
    (b) = (val >> 16) & 0xff;                                                                                          \
    (a) = (val >> 24) & 0xff;

/**********************************************************************
 *  cpacktorgba()  -
 **********************************************************************/
static void cpacktorgba(uint32 *l, uint16 *r, uint16 *g, uint16 *b, uint16 *a, sint32 n) {
    unsigned long val;

    while (n >= 8) {
        CPACKTORGBA(l[0], r[0], g[0], b[0], a[0]);
        CPACKTORGBA(l[1], r[1], g[1], b[1], a[1]);
        CPACKTORGBA(l[2], r[2], g[2], b[2], a[2]);
        CPACKTORGBA(l[3], r[3], g[3], b[3], a[3]);
        CPACKTORGBA(l[4], r[4], g[4], b[4], a[4]);
        CPACKTORGBA(l[5], r[5], g[5], b[5], a[5]);
        CPACKTORGBA(l[6], r[6], g[6], b[6], a[6]);
        CPACKTORGBA(l[7], r[7], g[7], b[7], a[7]);
        l += 8;
        r += 8;
        g += 8;
        b += 8;
        a += 8;
        n -= 8;
    }
    while (n--) {
        CPACKTORGBA(l[0], r[0], g[0], b[0], a[0]);
        l++;
        r++;
        g++;
        b++;
        a++;
    }
}
