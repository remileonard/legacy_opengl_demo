/*
 * Copyright 1989, 1990, 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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
 *  flight/tex.c $Revision: 1.1 $
 */

#include "fcntl.h"
#include "flight.h"
#include "porting/iris2ogl.h"
#include "stdio.h"

static float texps[] = {TX_MAGFILTER, TX_BILINEAR, TX_MINFILTER, TX_MIPMAP_LINEAR, 0};
static float texps_point[] = {TX_MAGFILTER, TX_POINT, TX_MINFILTER, TX_MIPMAP_POINT, 0};
static float tevps[] = {0};

int texon = FALSE;

int init_texturing() {
    long *image;
    unsigned char bw[128 * 128];
    int i;

    
    readtex("hills.t", bw, 128 * 128);
    
    texdef2d(1, 1, 128, 128, bw, 5, texps_point);
    tevdef(1, 0, tevps);
    
    printf("Texture system initialized\n");
    printf("======================\n\n");
    fflush(stdout);
}

int texturing(int b) {
    if (b) {
        texon = TRUE;
        texbind(0, 1);
        tevbind(0, 1);
    } else {
        texon = FALSE;
        texbind(0, 0);
        tevbind(0, 0);
    }
}

readtex(fname, buf, size) char *fname;
unsigned long *buf;
{
    long ifd;
    char file[80];

    strcpy(file, datadir);
    strcat(file, fname);

    if ((ifd = open(file, O_RDONLY)) == -1) {
        fprintf(stderr, "flight: can't open texture file %s\n", file);
        exit(1);
    }

    int readt = read(ifd, buf, size);
    if (readt != size) {
        fprintf(stderr, "flight: error reading texture file %s\n", file);
        close(ifd);
        exit(1);
    }
    close(ifd);
}
