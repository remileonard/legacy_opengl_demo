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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "flip.h"
#include "hash.h"
#include "porting/iris2ogl.h"

int32_t swap_int32(int32_t val) {
    uint8_t *bytes = (uint8_t *)&val;
    return ((int32_t)bytes[0] << 24) | ((int32_t)bytes[1] << 16) | ((int32_t)bytes[2] << 8) | ((int32_t)bytes[3]);
}

flipobj *readflipobj(char *name) {
    FILE *inf;
    flipobj *obj;
    int32_t i;
    int32_t nlongs;
    int32_t magic; // champ "colors"/magic ignoré
    int32_t *ip;

    inf = fopen(name, "rb");
    if (!inf) {
        fprintf(stderr, "readflipobj: can't open input file %s\n", name);
        exit(1);
    }

    /* Lecture du magic global (FASTMAGIC) déjà gérée chez toi avant,
       si tu gardes ça, ne relis pas ici. Je pars de ta version : */
    int32_t fileMagic;
    fread(&fileMagic, sizeof(int32_t), 1, inf);
    fileMagic = swap_int32(fileMagic);
    if (fileMagic != FASTMAGIC) {
        fprintf(stderr, "readflipobj: bad magic in object file\n");
        fclose(inf);
        exit(1);
    }

    obj = (flipobj *)malloc(sizeof(flipobj));
    if (!obj) {
        fprintf(stderr, "readflipobj: malloc flipobj FAILED\n");
        fclose(inf);
        exit(1);
    }

    /* npoints et "colors" viennent du fichier en 32 bits big-endian */
    fread(&obj->npoints, sizeof(int32_t), 1, inf);
    obj->npoints = swap_int32(obj->npoints);

    /* IGNORE COLORS FIELD: on le lit juste pour avancer dans le fichier */
    fread(&magic, sizeof(int32_t), 1, inf);
    magic = swap_int32(magic);
    obj->colors = magic; /* ou ignore, selon ce que tu veux */

    nlongs = 8 * obj->npoints; /* nombre d'int au total */

    /* Buffer brut en int32_t */
    int32_t *raw = (int32_t *)malloc(nlongs * sizeof(int32_t));
    if (!raw) {
        fprintf(stderr, "readflipobj: malloc raw FAILED (nlongs=%d)\n", nlongs);
        fclose(inf);
        exit(1);
    }
    long pos_after_header = ftell(inf); // après magic, npoints, colors
    fseek(inf, 0, SEEK_END);
    long end = ftell(inf);
    long bytes_available = end - pos_after_header;
    fseek(inf, pos_after_header, SEEK_SET);
    /* Reproduire exactement la logique originale: 3 int lus sur 4 */
    ip = raw;
    for (i = 0; i < nlongs / 4; i++, ip += 4) {
        int len = fread(ip, 3 * sizeof(int32_t), 1, inf);
        if (len != 1) {
            fprintf(stderr, "readflipobj: read error on data (i=%d)\n", i);
            free(raw);
            fclose(inf);
            exit(1);
        }
        ip[0] = swap_int32(ip[0]);
        ip[1] = swap_int32(ip[1]);
        ip[2] = swap_int32(ip[2]);
        /* ip[3] reste à 0, comme dans l’original */
    }

    fclose(inf);
    long bytes_needed = (nlongs / 4) * 3 * sizeof(int32_t); // = 6 * npoints * 4
    printf("available=%ld needed=%ld\n", bytes_available, bytes_needed);
    /* Conversion vers float* exploitable par le reste du code */
    obj->data = (float *)malloc(nlongs * sizeof(float));
    if (!obj->data) {
        fprintf(stderr, "readflipobj: malloc data FAILED (nlongs=%d)\n", nlongs);
        free(raw);
        exit(1);
    }

    for (i = 0; i < nlongs; i++) {
        union {
            int32_t i;
            float f;
        } u;
        u.i = raw[i];
        obj->data[i] = u.f;
    }

    free(raw);

    printf("readflipobj: npoints=%d\n", obj->npoints);
    printf("readflipobj: data[0]=%g data[last]=%g\n", obj->data[0], obj->data[8 * obj->npoints - 1]);
    /* Ensuite tu peux réactiver */
    swirl_randomize(obj);
    fflush(stdout);
    find_edges(obj);

    return obj;
}

void drawflipobj(flipobj *obj) {
    register float *p, *end;
    enum DrawType lflag;

    p = obj->data;
    end = p + 8 * obj->npoints;
    lflag = obj->type;

    if (obj->type == POLYGONS) {
        while (p < end) {
            bgnpolygon();
            n3f(p);
            v3f(p + 4);
            n3f(p + 8);
            v3f(p + 12);
            n3f(p + 16);
            v3f(p + 20);
            n3f(p + 24);
            v3f(p + 28);
            endpolygon();
            p += 32;
        }
    } else {
        int i;

        for (i = 0; i < obj->nedges; i++) {
            bgnline();
            n3f(obj->edge[i].v0 - 4);
            v3f(obj->edge[i].v0);
            n3f(obj->edge[i].v1 - 4);
            v3f(obj->edge[i].v1);
            endline();
        }
    }
}

/*
 * objmaxpoint
 *
 * find the vertex farthest from the origin,
 * so we can set the near and far clipping planes tightly.
 */

#define MAXVERT(v)                                                                                                     \
    if ((len = sqrt((*(v)) * (*(v)) + (*(v + 1)) * (*(v + 1)) + (*(v + 2)) * (*(v + 2)))) > max)                       \
        max = len;

float objmaxpoint(flipobj *obj) {
    register float *p, *end;
    register int npolys;
    register float len;
    register float max = 0.0;

    p = obj->data;

    end = p + 8 * obj->npoints;
    while (p < end) {
        MAXVERT(p + 4);
        MAXVERT(p + 12);
        MAXVERT(p + 20);
        MAXVERT(p + 28);
        p += 32;
    }

    return max;
}

/*
 *	Use hash functions to find all unique edges
 */
int find_edges(flipobj *obj) {
    int i, j, v0, v1, n;
    float *p, *end;
    printf("about to call find_edges: npoints=%d\n", obj->npoints);
    fflush(stdout);
    h_init_vertex(obj->npoints * 2);
    h_init_edge(obj->npoints * 4);

    /* First run through, to figure out how many there are */
    p = obj->data;
    end = p + 8 * obj->npoints;
    while (p < end) {
        v0 = h_find_vertex(p + 4);
        v1 = h_find_vertex(p + 12);
        h_find_edge(v0, v1);
        v0 = h_find_vertex(p + 12);
        v1 = h_find_vertex(p + 20);
        h_find_edge(v0, v1);
        v0 = h_find_vertex(p + 20);
        v1 = h_find_vertex(p + 28);
        h_find_edge(v0, v1);
        v0 = h_find_vertex(p + 28);
        v1 = h_find_vertex(p + 4);
        h_find_edge(v0, v1);
        p += 32;
    }
    /* Now malloc enough space */
    obj->nedges = h_get_ne();
    obj->edge = (flipedge *)malloc(sizeof(flipedge) * obj->nedges);

    /* And now run through, filling up structure */
    p = obj->data;
    end = p + 8 * obj->npoints;
    while (p < end) {
        v0 = h_find_vertex(p + 4);
        v1 = h_find_vertex(p + 12);
        n = h_find_edge(v0, v1);
        obj->edge[n].v0 = p + 4;
        obj->edge[n].v1 = p + 12;

        v0 = h_find_vertex(p + 12);
        v1 = h_find_vertex(p + 20);
        n = h_find_edge(v0, v1);
        obj->edge[n].v0 = p + 12;
        obj->edge[n].v1 = p + 20;

        v0 = h_find_vertex(p + 20);
        v1 = h_find_vertex(p + 28);
        n = h_find_edge(v0, v1);
        obj->edge[n].v0 = p + 20;
        obj->edge[n].v1 = p + 28;

        v0 = h_find_vertex(p + 28);
        v1 = h_find_vertex(p + 4);
        n = h_find_edge(v0, v1);
        obj->edge[n].v0 = p + 28;
        obj->edge[n].v1 = p + 4;

        p += 32;
    }
    h_destroy_vertex();
    h_destroy_edge();
    return 0;
}
