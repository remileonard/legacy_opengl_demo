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
#include "space.h"
#include <GL/gl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#define LOWER_TRI 1
#define UPPER_TRI 2
#define BOTHH_TRI 3

#define LEVEL_SEA 1.0000
#define LEVEL_SNOW 0.9995

#define MAT_SEABLUE 0
#define MAT_DIRT 1
#define MAT_WHITE 2
#define SUM_WHITE 6
#define SUM_SEABLUE 0

#define SUBDIVIDE(aa, bb, cc)                                                                                          \
    x = (aa)->plan.x + (bb)->plan.x;                                                                                   \
    y = (aa)->plan.y + (bb)->plan.y;                                                                                   \
    z = (aa)->plan.z + (bb)->plan.z;                                                                                   \
    w = fsqrt(x * x + z * z);                                                                                          \
                                                                                                                       \
    (cc)->seed = (aa)->seed + (bb)->seed;                                                                              \
    srand((cc)->seed);                                                                                                 \
    s = rand() - 16384;                                                                                                \
    (cc)->delta = 0.5 * ((aa)->delta + (bb)->delta) + rg * (flot32)s;                                                  \
    w = (cc)->delta / w;                                                                                               \
                                                                                                                       \
    (cc)->plan.x = w * x;                                                                                              \
    (cc)->plan.y = 0.5 * y;                                                                                            \
    (cc)->plan.z = w * z;

typedef struct {
    sint32 ar, ag, ab;
    flot32 dr, dg, db;
} C6;

static C6 a_dirt, a_seab, a_whit;

typedef struct { /* 128 BYTES */
    P3 p;
    uint32 seed;
    sint16 valid;
    sint16 quadw;
    sint32 level;
    sint32 sidsz;
    TrigPoint *sur;
    P3 pp[8];
} P5;

extern P4 feye;
extern V4 plum[NUM_CLIP_PLANES];
extern t_stopwatch Counter;

static sint32 dbg[8];
static flot32 roughness = 0.0;
static sint32 C;
static P5 dt[2][1025];
static struct {
    sint32 status, xi, yi, flag;
} status;

static void fract_mid(sint32, sint32, flot32);
static void create_continent(P5 *, sint32, sint32);
static void destroy_continent(P5 *);
static sint32 clipp(P5 *);
static void land_generation(P5 *, sint32);
static void water_conversion(P5 *, sint32);
static void generate_shades(P5 *, sint32);
static void cpack_continent(P5 *, uint32);
static void p_trigpoint(TrigPoint *);
static void p_continent(P5 *);

/**********************************************************************
 *  fract_mid()  -
 **********************************************************************/
static void fract_mid(sint32 x1, sint32 x2, flot32 rough)

{
    sint32 xi, s, seed;
    flot32 x, y, z, w[4];
    P5 *d, *d1, *d2;
    P4 p;

    xi = (x1 + x2) >> 1;

    d = &dt[0][xi];
    d1 = &dt[0][x1];
    d2 = &dt[0][x2];

    x = d1->p.x + d2->p.x;
    y = d1->p.y + d2->p.y;
    z = d1->p.z + d2->p.z;
    w[0] = fsqrt(x * x + z * z);

    w[1] = fsqrt(d1->p.x * d1->p.x + d1->p.z * d1->p.z);
    w[2] = fsqrt(d2->p.x * d2->p.x + d2->p.z * d2->p.z);
    d->seed = d1->seed + d2->seed;
    srand(d->seed);
    s = rand() - 26000;

    w[3] = (0.5 * (w[1] + w[2]) + rough * (float)s / 16384.0) / w[0];

    d->p.x = w[3] * x;
    d->p.y = 0.5 * y;
    d->p.z = w[3] * z;

    d = &dt[1][xi];
    d1 = &dt[1][x1];
    d2 = &dt[1][x2];

    x = d1->p.x + d2->p.x;
    y = d1->p.y + d2->p.y;
    z = d1->p.z + d2->p.z;
    w[0] = fsqrt(x * x + z * z);

    w[1] = fsqrt(d1->p.x * d1->p.x + d1->p.z * d1->p.z);
    w[2] = fsqrt(d2->p.x * d2->p.x + d2->p.z * d2->p.z);
    d->seed = d1->seed + d2->seed;
    srand(d->seed);
    seed = rand() - 16384;
    s += seed >> 2;

    w[3] = (0.5 * (w[1] + w[2]) + rough * (float)s / 16384.0) / w[0];

    d->p.x = w[3] * x;
    d->p.y = 0.5 * y;
    d->p.z = w[3] * z;
}

/**********************************************************************
 *  generate_ringworld()  -
 **********************************************************************/
void generate_ringworld(sint32 in_level, sint32 in_seed)

{
    flot32 x, y, z, w, e, delta0, delta1, delta2, delta3;
    sint32 i, j, stage, step, dstep;
    P4 q;
    P5 *d, *p0, *p1, *p2, *p3;
    P3 *pp;

    roughness = 0.0003;

    a_dirt.ar = 5;
    a_dirt.ag = 4;
    a_dirt.ab = 2;
    a_dirt.dr = 155.0;
    a_dirt.dg = 140.0;
    a_dirt.db = 70.0;
    a_seab.ar = 0;
    a_seab.ag = 0;
    a_seab.ab = 5;
    a_seab.dr = 0.0;
    a_seab.dg = 0.0;
    a_seab.db = 224.0;
    a_whit.ar = 5;
    a_whit.ag = 5;
    a_whit.ab = 5;
    a_whit.dr = 250.0;
    a_whit.dg = 250.0;
    a_whit.db = 250.0;

    srand(in_seed);

    w = RINGWEDGE;

    d = &dt[0][0];
    d->p.y = w;
    d->p.x = -1.0;
    d->p.z = 0.0;
    d->seed = rand();
    d = &dt[1][0];
    d->p.y = -w;
    d->p.x = -1.0;
    d->p.z = 0.0;
    d->seed = rand();
    d = &dt[0][256];
    d->p.y = w;
    d->p.x = 0.0;
    d->p.z = -1.0;
    d->seed = rand();
    d = &dt[1][256];
    d->p.y = -w;
    d->p.x = 0.0;
    d->p.z = -1.0;
    d->seed = rand();
    d = &dt[0][512];
    d->p.y = w;
    d->p.x = 1.0;
    d->p.z = 0.0;
    d->seed = rand();
    d = &dt[1][512];
    d->p.y = -w;
    d->p.x = 1.0;
    d->p.z = 0.0;
    d->seed = rand();
    d = &dt[0][768];
    d->p.y = w;
    d->p.x = 0.0;
    d->p.z = 1.0;
    d->seed = rand();
    d = &dt[1][768];
    d->p.y = -w;
    d->p.x = 0.0;
    d->p.z = 1.0;
    d->seed = rand();
    d = &dt[0][1024];
    *d = dt[0][0];
    d = &dt[1][1024];
    *d = dt[1][0];

    for (step = 128, dstep = 256, stage = 1; stage <= 8; dstep = step, step >>= 1, stage++)
        for (i = step; i <= 1024; i += dstep)
            fract_mid(i - step, i + step, (flot32)4.0 * roughness / (stage * stage));

    for (i = 0; i < 1024; i++) {
        dt[0][i].valid = 0;
        dt[0][i].sur = 0;
        dt[0][i].level = -1;
        dt[0][i].quadw = -1;
        dt[0][i].sidsz = -1;

        p0 = &dt[0][i + 0];
        p1 = &dt[0][i + 1];
        p2 = &dt[1][i + 0];
        p3 = &dt[1][i + 1];

        delta0 = fsqrt(p0->p.x * p0->p.x + p0->p.z * p0->p.z);
        delta1 = fsqrt(p1->p.x * p1->p.x + p1->p.z * p1->p.z);
        delta2 = fsqrt(p2->p.x * p2->p.x + p2->p.z * p2->p.z);
        delta3 = fsqrt(p3->p.x * p3->p.x + p3->p.z * p3->p.z);

        pp = dt[0][i].pp;
        pp[0].x = p0->p.x / delta0;
        pp[0].y = p0->p.y;
        pp[0].z = p0->p.z / delta0;
        pp[2].x = p1->p.x / delta1;
        pp[2].y = p1->p.y;
        pp[2].z = p1->p.z / delta1;
        pp[4].x = p2->p.x / delta2;
        pp[4].y = p2->p.y;
        pp[4].z = p2->p.z / delta2;
        pp[6].x = p3->p.x / delta3;
        pp[6].y = p3->p.y;
        pp[6].z = p3->p.z / delta3;

        pp[1].x = 0.999 * pp[0].x;
        pp[1].y = pp[0].y;
        pp[1].z = 0.999 * pp[0].z;
        pp[3].x = 0.999 * pp[2].x;
        pp[3].y = pp[2].y;
        pp[3].z = 0.999 * pp[2].z;
        pp[5].x = 0.999 * pp[4].x;
        pp[5].y = pp[4].y;
        pp[5].z = 0.999 * pp[4].z;
        pp[7].x = 0.999 * pp[6].x;
        pp[7].y = pp[6].y;
        pp[7].z = 0.999 * pp[6].z;
    }

    status.status = 1;
    status.xi = 0;
    status.yi = 0;
    status.flag = 0;
}

/**********************************************************************
 *  display_ringworld()  -
 **********************************************************************/
void display_ringworld(void)

{
    sint32 i, j, flag, level;
    flot32 vian, w;
    P5 *p;
    V4 *v;

    for (i = 0; i < 8; i++)
        dbg[i] = 0;

    /* generate new plates, destroy old plates, draw valid plates */
    for (p = dt[0], j = 0; j < 1024; p++, j++) {
        w = (feye.x - p->p.x) * (feye.x - p->p.x) + (feye.y - p->p.y) * (feye.y - p->p.y) +
            (feye.z - p->p.z) * (feye.z - p->p.z);

        if (w > 1.00000000)
            level = 0 + Counter.hw_graphics;
        else if (w > 0.16000000)
            level = 1 + Counter.hw_graphics;
        else if (w > 0.02560000)
            level = 2 + Counter.hw_graphics;
        else if (w > 0.00409600)
            level = 3 + Counter.hw_graphics;
        else if (w > 0.00065536)
            level = 4 + Counter.hw_graphics;
        else if (w > 0.000065536)
            level = 5 + Counter.hw_graphics;
        else if (w > 0.0000065536)
            level = 6 + Counter.hw_graphics;
        else
            level = 7 + Counter.hw_graphics;

        if (level < 0)
            level = 0;
        if (level > 9)
            level = 9;

        if (level != p->level) {
            p->level = level;
            for (p->sidsz = 1, i = 0; i < p->level; p->sidsz <<= 1, i++)
                ;
            destroy_continent(p);
        }

        if (flag = (~p->valid & BOTHH_TRI))
            create_continent(p, flag, 0);

        if (p->sur) {
            flag = p->valid & clipp(p);
            if (Counter.flags & SHADE_FLAG)
                generate_shades(p, flag);
            cpack_continent(p, flag);
        }
    }

    if ((Counter.flags & DEBUG_FLAG) && (dbg[0] + dbg[1] + dbg[7]))
        printf("Create: %d/%d Display: %02d/%02d Enhance: %d\n", dbg[0], dbg[1], dbg[2], dbg[3], dbg[7]);
}

/**********************************************************************
 *  destroy_ringworld()  -
 **********************************************************************/
void destroy_ringworld(void)

{
    sint32 i, j;
    P5 *p;

    for (p = dt[0], j = 0; j < 1024; p++, j++)
        destroy_continent(p);
}

/**********************************************************************
 *  create_continent()  -
 **********************************************************************/
static void create_continent(P5 *d, sint32 create, sint32 even)

{
    sint32 cstat, i, size;
    uintptr_t hey;
    TrigPoint *sur, *p0, *p1, *p2, *p3;
    P3 *p;

    cstat = clipp(d);

    if (status.status == 1)
        create &= cstat;

    if (!create)
        return;

    if (!d->sur) {
        size = 8 + (d->sidsz + 1) * (d->sidsz + 1) * sizeof(TrigPoint);

        if (!(sur = (TrigPoint *)malloc(size))) {
            fprintf(stderr, "ERROR: malloc(%d) failed in create_continent()\n", size);
            exit(0);
        }

        hey = (uintptr_t)sur;
        if (hey & 8) {
            hey = (hey + 8) & 0xfffffff0;
            sur = (TrigPoint *)hey;
            d->quadw = 0;
        } else
            d->quadw = 1;
        p0 = sur;
        p1 = p0 + d->sidsz;
        p2 = sur + (d->sidsz + 1) * d->sidsz;
        p3 = p2 + d->sidsz;

        p0->plan = (d + 0)->p;
        p0->seed = (d + 0)->seed;
        p0->delta = fsqrt(p0->plan.x * p0->plan.x + p0->plan.z * p0->plan.z);

        p1->plan = (d + 1)->p;
        p1->seed = (d + 1)->seed;
        p1->delta = fsqrt(p1->plan.x * p1->plan.x + p1->plan.z * p1->plan.z);

        p2->plan = (d + 1025)->p;
        p2->seed = (d + 1025)->seed;
        p2->delta = fsqrt(p2->plan.x * p2->plan.x + p2->plan.z * p2->plan.z);

        p3->plan = (d + 1026)->p;
        p3->seed = (d + 1026)->seed;
        p3->delta = fsqrt(p3->plan.x * p3->plan.x + p3->plan.z * p3->plan.z);
    }

    if (create) {
        if (create == BOTHH_TRI)
            dbg[0] += 2;
        else
            dbg[1]++;

        if (!d->sur)
            d->sur = sur;

        land_generation(d, create);

        water_conversion(d, create);

        generate_shades(d, create);
        d->valid |= create;
    } else if (!d->sur) {
        hey = (uintptr_t)sur;
        free((void *)(hey - ((d->quadw) ? 0 : 8)));
    }
} /**********************************************************************
   *  destroy_continent()  -
   **********************************************************************/
static void destroy_continent(P5 *p)

{
    uintptr_t hey;

    if (p->sur) {
        hey = (uintptr_t)p->sur;

        free((void *)(hey - ((p->quadw) ? 0 : 8)));

        p->valid = 0;
        p->sur = 0;
        p->quadw = 0;
    }
}

/**********************************************************************
 *  land_generation()  -
 **********************************************************************/
static void land_generation(P5 *d, sint32 triflag)

{
    sint32 stage, step, dstep, i, j, s, q, sp;
    flot32 x, y, z, w, rg;
    TrigPoint *c, *c1, *c2, *c3, *c4;
    P4 p;
    TrigPoint *sur;

    sur = d->sur;

    step = 1 << (d->level - 1);
    rg = roughness / ((1.0 + C) * 16384.0);

    for (dstep = step + step, stage = 1; stage <= d->level; rg *= 0.50, dstep = step, step >>= 1, stage++) {
        sp = step * (d->sidsz + 1);

        for (i = dstep; i <= d->sidsz; i += dstep) {
            c1 = sur + i - dstep;
            c2 = sur + i;
            c3 = sur + dstep * (d->sidsz + 1) + i - dstep;
            c4 = sur + dstep * (d->sidsz + 1) + i;

            for (j = step; j <= d->sidsz; j += dstep) {
                q = d->sidsz + step - i - j;

                if ((triflag & LOWER_TRI) && (q >= 0)) {
                    c = c2 - step;
                    SUBDIVIDE(c1, c2, c);
                    c = c1 + sp;
                    SUBDIVIDE(c1, c3, c);
                    c = c1 + sp + step;
                    SUBDIVIDE(c2, c3, c);
                }

                if ((triflag & UPPER_TRI) && (q <= 0)) {
                    c = c4 - step;
                    SUBDIVIDE(c3, c4, c);
                    c = c2 + sp;
                    SUBDIVIDE(c2, c4, c);
                    c = c1 + sp + step;
                    SUBDIVIDE(c2, c3, c);
                }

                c1 += sp + sp;
                c2 += sp + sp;
                c3 += sp + sp;
                c4 += sp + sp;
            }
        }
    }
}

/**********************************************************************
 *  water_conversion()  -
 **********************************************************************/
static void water_conversion(P5 *d, sint32 triflag)

{
    sint32 i, j, q;
    flot32 w;
    TrigPoint *sur, *trg;

    sur = d->sur;

    for (i = 0; i <= d->sidsz; i++)
        for (trg = sur + (d->sidsz + 1) * i, j = 0; j <= d->sidsz; trg++, j++) {
            q = d->sidsz - i - j;

            if (((triflag & LOWER_TRI) && (q >= 0)) || ((triflag & UPPER_TRI) && (q <= 0))) {
                w = trg->delta;

                if (w >= LEVEL_SEA) {
                    trg->plan.x /= w;
                    trg->plan.z /= w;
                    trg->label = MAT_SEABLUE;
                } else if (w < LEVEL_SNOW)
                    trg->label = MAT_WHITE;
                else
                    trg->label = MAT_DIRT;
            }
        }
}

/**********************************************************************
 *  generate_shades()  -
 **********************************************************************/
static void generate_shades(P5 *d, sint32 triflag)

{
    sint32 i, j, r, g, b, q;
    flot32 x, y, z, w, daylight, theta, delta;
    V3 v1, v2, lit;
    TrigPoint *sur, *p00, *p10;
    C6 *cc, cl;
    P3 *p;

    sur = d->sur;

    lit.x = sur->plan.x;
    lit.y = 0.0;
    lit.z = sur->plan.z;
    w = -fsqrt(lit.x * lit.x + lit.z * lit.z);
    lit.x /= w;
    lit.z /= w;

    delta = 0.5 * Counter.D / 9.037;
    delta = 366.0 - 360.0 * (delta - (long)delta);

    for (i = 0; i < d->sidsz; i++) {
        p00 = sur + (d->sidsz + 1) * (i);
        p10 = sur + (d->sidsz + 1) * (i + 1);

        for (j = 0; j < d->sidsz; p00++, p10++, j++) {
            q = d->sidsz - i - j;

            theta = RTOD * (M_PI + atan2(p00->plan.x, p00->plan.z)) + delta;
            theta /= 12.0;
            r = theta;
            if (r & 1)
                daylight = 1.0;
            else
                daylight = 0.0;

            if (((triflag & LOWER_TRI) && (q >= 1)) || ((triflag & UPPER_TRI) && (q <= 0))) {
                v1.x = (p00 + 1)->plan.x - p00->plan.x;
                v1.y = (p00 + 1)->plan.y - p00->plan.y;
                v1.z = (p00 + 1)->plan.z - p00->plan.z;

                v2.x = p10->plan.x - p00->plan.x;
                v2.y = p10->plan.y - p00->plan.y;
                v2.z = p10->plan.z - p00->plan.z;

                x = v2.y * v1.z - v1.y * v2.z;
                y = v1.x * v2.z - v2.x * v1.z;
                z = v2.x * v1.y - v1.x * v2.y;
                w = x * lit.x + y * lit.y + z * lit.z;

                if (Counter.flags & FRACT_FLAG) {
                    r = p00->label + (p00 + 1)->label + p10->label;
                    if (r == SUM_SEABLUE)
                        cc = &a_seab;
                    else if (r == SUM_WHITE)
                        cc = &a_whit;
                    else
                        cc = &a_dirt;

                    r = cc->ar;
                    g = cc->ag;
                    b = cc->ab;

                    if (w > 0.0) {
                        w *= daylight / fsqrt(x * x + y * y + z * z);
                        r += cc->dr * w;
                        g += cc->dg * w;
                        b += cc->db * w;
                    }
                } else {
                    cl.dr = ((p00->seed) & 0xff) + (((p00 + 1)->seed) & 0xff) + ((p10->seed) & 0xff);
                    cl.dg = ((p00->seed >> 8) & 0xff) + (((p00 + 1)->seed >> 8) & 0xff) + ((p10->seed >> 8) & 0xff);
                    cl.db = ((p00->seed >> 16) & 0xff) + (((p00 + 1)->seed >> 16) & 0xff) + ((p10->seed >> 16) & 0xff);
                    r = 0.015 * cl.dr;
                    g = 0.015 * cl.dg;
                    b = 0.015 * cl.db;

                    if (w > 0.0) {
                        w *= daylight / (3.0 * fsqrt(x * x + y * y + z * z));
                        r += cl.dr * w;
                        g += cl.dg * w;
                        b += cl.db * w;
                    }
                }

                p00->cpack1 = (r << 24) | (g << 16) | (b << 8) | 0xff;
            }

            if (((triflag & LOWER_TRI) && (q > 1)) || ((triflag & UPPER_TRI) && (q <= 1))) {
                v1.x = p10->plan.x - (p10 + 1)->plan.x;
                v1.y = p10->plan.y - (p10 + 1)->plan.y;
                v1.z = p10->plan.z - (p10 + 1)->plan.z;

                v2.x = (p00 + 1)->plan.x - (p10 + 1)->plan.x;
                v2.y = (p00 + 1)->plan.y - (p10 + 1)->plan.y;
                v2.z = (p00 + 1)->plan.z - (p10 + 1)->plan.z;

                x = v2.y * v1.z - v1.y * v2.z;
                y = v1.x * v2.z - v2.x * v1.z;
                z = v2.x * v1.y - v1.x * v2.y;
                w = x * lit.x + y * lit.y + z * lit.z;

                if (Counter.flags & FRACT_FLAG) {
                    r = (p10 + 1)->label + (p00 + 1)->label + p10->label;
                    if (r == SUM_SEABLUE)
                        cc = &a_seab;
                    else if (r == SUM_WHITE)
                        cc = &a_whit;
                    else
                        cc = &a_dirt;

                    r = cc->ar;
                    g = cc->ag;
                    b = cc->ab;

                    if (w > 0.0) {
                        w *= daylight / fsqrt(x * x + y * y + z * z);
                        r += cc->dr * w;
                        g += cc->dg * w;
                        b += cc->db * w;
                    }
                } else {
                    cl.dr = (((p10 + 1)->seed) & 0xff) + (((p00 + 1)->seed) & 0xff) + ((p10->seed) & 0xff);
                    cl.dg =
                        (((p10 + 1)->seed >> 8) & 0xff) + (((p00 + 1)->seed >> 8) & 0xff) + ((p10->seed >> 8) & 0xff);
                    cl.db = (((p10 + 1)->seed >> 16) & 0xff) + (((p00 + 1)->seed >> 16) & 0xff) +
                            ((p10->seed >> 16) & 0xff);
                    r = 0.015 * cl.dr;
                    g = 0.015 * cl.dg;
                    b = 0.015 * cl.db;

                    if (w > 0.0) {
                        w *= daylight / (3.0 * fsqrt(x * x + y * y + z * z));
                        r += cl.dr * w;
                        g += cl.dg * w;
                        b += cl.db * w;
                    }
                }

                p00->cpack2 = (r << 24) | (g << 16) | (b << 8) | 0xff;
            }
        }
    }
}

/**********************************************************************
 *  clipp()  -
 **********************************************************************/
static sint32 clipp(P5 *d)

{
    sint32 i, j;
    uint32 flag, resu, mask;
    P3 *p;
    V4 *c;

    for (c = plum, resu = i = 0; i < NUM_CLIP_PLANES; c++, i++) {
        for (mask = 1, p = d->pp, flag = 0, j = 0; j < 8; mask <<= 1, p++, j++)
            if (c->x * p->x + c->y * p->y + c->z * p->z + c->w < 0.0)
                flag |= mask;

        if (flag == 0xff)
            return (0);
        if ((flag & 0x3f) == 0x3f)
            resu |= LOWER_TRI;
        if ((flag & 0xfc) == 0xfc)
            resu |= UPPER_TRI;
    }

    return (BOTHH_TRI);
}

/**********************************************************************
 *  cpack_continent()  -
 **********************************************************************/
static void cpack_continent(P5 *d, uint32 flag)

{
    sint32 i, j;
    TrigPoint *sur, *trp, *srp;

    sur = d->sur;

    if ((uintptr_t)sur & 0x0f) {
        printf("Error: data is not quad-word aligned!\n");
        exit(1);
    }

    switch (flag) {
    case 0:
        return;
        break;

    case BOTHH_TRI:
        for (srp = sur, j = d->sidsz; j > 0; srp += d->sidsz + 1, j--) {
            glBegin(GL_TRIANGLE_STRIP);
            glVertex3fv(&srp->plan.x);
            glVertex3fv(&(srp + d->sidsz + 1)->plan.x);

            for (trp = srp, i = d->sidsz; i > 0; trp++, i--) {
                glColor4ubv((unsigned char *)&trp->cpack1);
                glVertex3fv(&(trp + 1)->plan.x);

                glColor4ubv((unsigned char *)&trp->cpack2);
                glVertex3fv(&(trp + d->sidsz + 2)->plan.x);
            }

            glEnd();
        }

        dbg[2] += 2;
        break;

    case LOWER_TRI:
        for (srp = sur, j = d->sidsz; j > 0; srp += d->sidsz = 1, j--) {
            glBegin(GL_TRIANGLE_STRIP);
            glVertex3fv(&srp->plan.x);
            glVertex3fv(&(srp + d->sidsz + 1)->plan.x);

            for (trp = srp, i = j - 1; i > 0; i--) {
                glColor4ubv((unsigned char *)&trp->cpack1);
                glVertex3fv(&(trp + 1)->plan.x);

                glColor4ubv((unsigned char *)&trp->cpack2);
                glVertex3fv(&(trp + d->sidsz + 2)->plan.x);
                trp++;
            }

            glColor4ubv((unsigned char *)&trp->cpack1);
            glVertex3fv(&(trp + 1)->plan.x);
            glEnd();
        }

        dbg[3]++;
        break;

    case UPPER_TRI:
        for (srp = (sur + (d->sidsz + 1) * d->sidsz), j = d->sidsz; j > 0; srp -= d->sidsz + 1, j--) {
            glBegin(GL_TRIANGLE_STRIP);
            glVertex3fv(&srp->plan.x);
            glVertex3fv(&(srp - d->sidsz - 1)->plan.x);

            for (trp = srp, i = j - 1; i > 0; i--) {
                glColor4ubv((unsigned char *)&(trp - d->sidsz - 2)->cpack2);
                glVertex3fv(&(trp - 1)->plan.x);

                glColor4ubv((unsigned char *)&(trp - d->sidsz - 2)->cpack1);
                glVertex3fv(&(trp - d->sidsz - 2)->plan.x);
                trp--;
            }

            glColor4ubv((unsigned char *)&(trp - d->sidsz - 2)->cpack2);
            glVertex3fv(&(trp - 1)->plan.x);
            glEnd();
        }

        dbg[3]++;
        break;

    default:
        printf("Error: cpack_continent(%d)\n", flag);
        exit(0);
        break;
    }
}

/**********************************************************************
 *  p_trigpoint()  -
 **********************************************************************/
static void p_trigpoint(TrigPoint *t)

{
    printf("T: %7.4f %7.4f %7.4f %6.4f %06d %1d 0x%08x 0x%08x\n", t->plan.x, t->plan.y, t->plan.z, t->delta, t->seed,
           t->label, t->cpack1, t->cpack2);
}

/**********************************************************************
 *  p_continent()  -
 **********************************************************************/
static void p_continent(P5 *d)

{
    sint32 i, j;
    TrigPoint *sur;

    printf("Continent: %p\n", (void *)d->sur);

    for (i = 0; i <= d->sidsz; i++)
        for (j = 0; j <= d->sidsz; j++) {
            printf("%d %d:", i, j);
            p_trigpoint(d->sur + (d->sidsz + 1) * i + j);
        }
}
