/*
 * Copyright 1984-1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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

#include "porting/iris2ogl.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h> // Pour _read, _open, _close sous Windows
#define read _read
#define open _open
#define close _close
#define O_RDONLY _O_RDONLY
#else
#include <unistd.h> // Pour read, open, close sous Unix/Linux
#endif
#include "collision.h"

/*
 * collide_tri() returns true if the given point has a y value less than the
 * y value of the given triangle at the given x and z location.
 *
 *	A-------B
 *	| P    /
 *	|    /
 *	|  /
 *	|/
 *	C
 */
int collide_tri(px, py, pz, ax, bx, az, cz, aelv, belv, celv)
float px, py, pz, ax, bx, az, cz, aelv, belv, celv;
{
    float delta1, elv1, delta2, elv2;

    delta1 = (ax - px) / (ax - bx);
    elv1 = aelv - ((aelv - belv) * delta1);
    delta2 = (az - pz) / (az - cz);
    elv2 = elv1 - ((aelv - celv) * delta2);

    if (py < elv2)
        return (TRUE);
    else
        return (FALSE);
}

/*
 * collide_grid() returns true if the given point is lower than the given grid
 * at the point's x and z location.
 */
int collide_grid(float px, float py, float pz, grid_t *g) {
    int gx, gz;
    float rpx, rpz;

    if (px > g->xmin && px < g->xmax && pz > g->zmin && pz < g->zmax) {
        gx = (px - g->xmin) / g->stepsize;
        gz = (pz - g->zmin) / g->stepsize;
        rpx = px - gx * g->stepsize - g->xmin;
        rpz = pz - gz * g->stepsize - g->zmin;
        if (rpx + rpz < g->stepsize)
            return (collide_tri(rpx, py, rpz, 0.0, g->stepsize, 0.0, g->stepsize, g->elv[gx][gz], g->elv[gx + 1][gz],
                                g->elv[gx][gz + 1]));
        else
            return (collide_tri(rpx, py, rpz, g->stepsize, 0.0, g->stepsize, 0.0, g->elv[gx + 1][gz + 1],
                                g->elv[gx][gz + 1], g->elv[gx + 1][gz]));
    }

    return (FALSE);
}

int32_t swap_int32(int32_t val) {
    uint8_t *bytes = (uint8_t *)&val;
    return ((int32_t)bytes[0] << 24) | ((int32_t)bytes[1] << 16) | ((int32_t)bytes[2] << 8) | ((int32_t)bytes[3]);
}
float swap_float(float val) {
    union {
        float f;
        uint8_t b[4];
    } in, out;

    in.f = val;
    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];

    return out.f;
}
/*
 *  Read a grid file
 */
grid_t *read_grid(char *fname) {
    int x, z, i;
    FILE *fp;
    float ftemp;
    grid_t *g;

    if ((fp = fopen(fname, "rb")) == NULL) {
        fprintf(stderr, "can't open \"%s\"\n", fname);
        perror("fopen");
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fprintf(stderr, "File size: %ld bytes\n", file_size);
    g = (grid_t *)malloc(sizeof(grid_t));
    if (g == NULL) {
        fprintf(stderr, "malloc failed for grid_t\n");
        fclose(fp);
        return NULL;
    }
    g->xsize = 0;
    g->zsize = 0;
    int32_t temp = 0;
    if (fread(&temp, sizeof(int32_t), 1, fp) != 1) {
        fprintf(stderr, "read_grid: error reading xsize\n");
        free(g);
        fclose(fp);
        return NULL;
    }
    g->xsize = swap_int32(temp);
    fread(&temp, sizeof(int32_t), 1, fp);
    g->zsize = swap_int32(temp);

    g->elv = (float **)malloc(sizeof(float *) * (g->xsize + 1));
    if (g->elv == NULL) {
        fprintf(stderr, "malloc failed for grid elevation array\n");
        free(g);
        fclose(fp);
        return NULL;
    }

    for (x = 0; x <= g->xsize; x++) {
        g->elv[x] = (float *)malloc(sizeof(float) * (g->zsize + 1));
        if (g->elv[x] == NULL) {
            fprintf(stderr, "malloc failed for grid elevation row\n");
            for (i = 0; i < x; i++)
                free(g->elv[i]);
            free(g->elv);
            free(g);
            fclose(fp);
            return NULL;
        }
    }

    for (z = 0; z <= g->zsize; z++) {
        for (x = 0; x <= g->xsize; x++) {
            fread(&ftemp, sizeof(float), 1, fp);
            g->elv[x][z] = swap_float(ftemp) * 2000.0;;
        }
    }
    fclose(fp);
    g->stepsize = 2000.0;
    g->xmin = g->zmin = 0.0;
    g->xmax = g->xsize * 2000.0;
    g->zmax = g->zsize * 2000.0;
    return (g);
}
