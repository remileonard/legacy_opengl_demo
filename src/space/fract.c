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
#define _USE_MATH_DEFINES
#include "space.h"
#include <GL/gl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define LOWER_TRI 1
#define UPPER_TRI 2
#define BOTHH_TRI 3

#define LEVEL_SEA 1.00
#define LEVEL_SNOW 1.02

#define MAT_SEABLUE 0
#define MAT_DIRT 1
#define MAT_WHITE 2
#define SUM_WHITE 6
#define SUM_SEABLUE 0

#define SUBDIVIDE(aa, bb, cc)                                                                                          \
    x = (aa)->plan.x + (bb)->plan.x;                                                                                   \
    y = (aa)->plan.y + (bb)->plan.y;                                                                                   \
    z = (aa)->plan.z + (bb)->plan.z;                                                                                   \
    w = fsqrt(x * x + y * y + z * z);                                                                                  \
                                                                                                                       \
    if (Counter.flags & FRACT_FLAG) {                                                                                  \
        (cc)->seed = (aa)->seed + (bb)->seed;                                                                          \
        srand((cc)->seed);                                                                                             \
        s = rand() - 16384;                                                                                            \
        (cc)->delta = 0.5 * ((aa)->delta + (bb)->delta) + rg * (flot32)s;                                              \
        w = (cc)->delta / w;                                                                                           \
    } else {                                                                                                           \
        p.x = x;                                                                                                       \
        p.y = y;                                                                                                       \
        p.z = z;                                                                                                       \
        (cc)->seed = convert(&p);                                                                                      \
        (cc)->delta = p.w;                                                                                             \
        w = (cc)->delta / w;                                                                                           \
    }                                                                                                                  \
                                                                                                                       \
    (cc)->plan.x = w * x;                                                                                              \
    (cc)->plan.y = w * y;                                                                                              \
    (cc)->plan.z = w * z;

typedef struct {
    sint32 ar, ag, ab;
    flot32 dr, dg, db;
} C6;

static C6 a_dirt, a_seab, a_whit;

typedef struct { /* 284 BYTES */
    P3 p;
    uint32 seed;
    sint16 valid;
    sint16 quadw;
    V4 v0[5];
    V4 v1[5];
    P3 pp[8];
    TrigPoint *sur;
} P5;

typedef struct {
    V4 v1[5];
    V4 v2[5];
} Bounder;

extern t_stopwatch Counter;
extern P4 feye;
extern V3 lit;
extern flot32 vids;
extern t_body *planet;
extern V4 plum[NUM_CLIP_PLANES];

static sint32 dbg[8];
static sint32 levels, sidsz;
static sint32 A, B, C;
static flot32 roughness = 0.05;
static P5 dt[33][33];
static struct {
    sint32 status, xi, yi, flag;
} status;

static Bounder checkcrash[33][33];

static void fract_mid(sint32, sint32, sint32, sint32, flot32);
static void create_continent(P5 *, sint32, sint32);
static void destroy_continent(P5 *);
static void bound_normals(P3 *, P3 *, P3 *, V4 *);
static sint32 clipp(P5 *);
static void land_generation(P5 *, sint32);
static void water_conversion(P5 *, sint32);
static void generate_shades(P5 *, sint32);
static void cpack_continent(P5 *, uint32);
static void intersect_continent(void);
static void enhanced_bound_normals(P5 *, sint32);
static void p_trigpoint(TrigPoint *);
static void p_continent(P5 *);
static uint32 convert(P4 *);

/**********************************************************************
 *  converu()  - Unsigned version of convert for special texture formats
 **********************************************************************/
uint32 converu(P4 *p)
{
    /* For now, just call the regular convert function */
    /* This is a placeholder - the original SGI implementation may have differed */
    return convert(p);
}

/**********************************************************************
 *  convert()  -
 **********************************************************************/
static uint32 convert(P4 *p)

{
    flot32 the, phi, s, t;
    sint32 i, data, q;
    sint16 *r;
    unsigned char c;

    if ((planet->ptr->lformat >> 4) & 0x1)
        return (converu(p));

    if (p->x == 0.0 && p->z == 0.0)
        p->x += 0.000001;

    the = fatan2(p->z, p->x);
    phi = fatan2(p->y, fsqrt(p->x * p->x + p->z * p->z));

    switch (planet->ptr->lformat & 0xf) {
    case FM_MERC_1B:
        t = (flot32)planet->ptr->lysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->lxsize * (0.5 - 0.5 * the / M_PI);
        i = t;
        i *= planet->ptr->lxsize;
        i += s;
        p->w = 1.0 + planet->ptr->scale * (flot32)planet->land[i];
        break;
    case FM_MERC_2B:
        t = (flot32)planet->ptr->lysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->lxsize * (0.5 - 0.5 * the / M_PI);
        i = t;
        i *= 2 * planet->ptr->lxsize;
        i += 2 * (sint32)s;
        r = (sint16 *)&planet->land[i];
        p->w = 1.0 + planet->ptr->scale * (flot32)*r;
        break;
    case FM_MERC_4B:
        t = (flot32)planet->ptr->lysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->lxsize * (0.5 - 0.5 * the / M_PI);
        i = t;
        i *= 4 * planet->ptr->lxsize;
        i += 4 * (sint32)s;
        q = *(sint32 *)&planet->land[i];
        data = (q & 0x00ffffff) | 0xff000000;
        i = (q >> 24) & 0x000000ff;
        p->w = 1.0 + planet->ptr->scale * (flot32)i;
        return (data);
        break;
    case FM_SINU_1B:
        t = (flot32)planet->ptr->lysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->lxsize * (0.5 - 0.5 * the * fcos(phi) / M_PI);
        i = t;
        i *= planet->ptr->lxsize;
        i += s;
        p->w = 1.0 + planet->ptr->scale * (flot32)planet->land[i];
        break;
    case FM_SINU_2B:
        t = (flot32)planet->ptr->lysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->lxsize * (0.5 - 0.5 * the * fcos(phi) / M_PI);
        i = t;
        i *= 2 * planet->ptr->lxsize;
        i += 2 * (sint32)s;
        r = (sint16 *)&planet->land[i];
        p->w = 1.0 + planet->ptr->scale * (flot32)*r;
        break;
    case FM_SINU_4B:
        t = (flot32)planet->ptr->lysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->lxsize * (0.5 - 0.5 * the * fcos(phi) / M_PI);
        i = t;
        i *= 4 * planet->ptr->lxsize;
        i += 4 * (sint32)s;
        q = *(sint32 *)&planet->land[i];
        data = (q & 0x00ffffff) | 0xff000000;
        i = (q >> 24) & 0x000000ff;
        p->w = 1.0 + planet->ptr->scale * (flot32)i;
        return (data);
        break;
    default:
        exit(0);
        break;
    }

    switch (planet->ptr->cformat & 0xf) {
    case FM_MERC_1B:
        t = (flot32)planet->ptr->cysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->cxsize * (0.5 - 0.5 * the / M_PI);
        i = t;
        i *= planet->ptr->cxsize;
        i += s;
        c = planet->colr[i];
        data = 0xff000000 | ((c << 14) & 0xff0000) | ((c << 7) & 0x00ff00) | c;
        break;
    case FM_MERC_3B:
        t = (flot32)planet->ptr->cysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->cxsize * (0.5 - 0.5 * the / M_PI);
        i = t;
        i *= 4 * planet->ptr->cxsize;
        i += 4 * (sint32)s;
        data = *(sint32 *)&planet->colr[i];
        break;
    case FM_SINU_1B:
        t = (flot32)planet->ptr->cysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->cxsize * (0.5 - 0.5 * the * fcos(phi) / M_PI);
        i = t;
        i *= planet->ptr->cxsize;
        i += s;
        c = planet->colr[i];
        data = 0xff000000 | ((c << 14) & 0xff0000) | ((c << 7) & 0x00ff00) | c;
        break;
    case FM_SINU_3B:
        t = (flot32)planet->ptr->cysize * (0.5 + phi / M_PI);
        s = (flot32)planet->ptr->cxsize * (0.5 - 0.5 * the * fcos(phi) / M_PI);
        i = t;
        i *= 3 * planet->ptr->cxsize;
        i += 3 * (sint32)s;
        data = *(sint32 *)&planet->colr[i];
        break;
    default:
        break;
    }

    return (data);
}

/**********************************************************************
 *  fract_mid()  -
 **********************************************************************/
static void fract_mid(sint32 x1, sint32 y1, sint32 x2, sint32 y2, flot32 rough)

{
    sint32 xi, yi, s, seed;
    flot32 x, y, z, w[4];
    P5 *d, *d1, *d2;
    P4 p;

    xi = (x1 + x2) >> 1;
    yi = (y1 + y2) >> 1;

    d = &dt[yi][xi];
    d1 = &dt[y1][x1];
    d2 = &dt[y2][x2];

    x = d1->p.x + d2->p.x;
    y = d1->p.y + d2->p.y;
    z = d1->p.z + d2->p.z;

    w[0] = fsqrt(x * x + y * y + z * z);

    if (Counter.flags & FRACT_FLAG) {
        w[1] = fsqrt(d1->p.x * d1->p.x + d1->p.y * d1->p.y + d1->p.z * d1->p.z);
        w[2] = fsqrt(d2->p.x * d2->p.x + d2->p.y * d2->p.y + d2->p.z * d2->p.z);
        d->seed = d1->seed + d2->seed;
        srand(d->seed);
        s = rand() - 16384;
        w[3] = (0.5 * (w[1] + w[2]) + rough * (float)s / 16384.0) / w[0];
    } else {
        p.x = x;
        p.y = y;
        p.z = z;
        d->seed = convert(&p);
        w[3] = p.w / w[0];
    }

    d->p.x = w[3] * x;
    d->p.y = w[3] * y;
    d->p.z = w[3] * z;
}

/**********************************************************************
 *  generate_fractsphere()  -
 **********************************************************************/
void generate_fractsphere(sint32 level, sint32 in_seed)

{
    flot32 x, y, z, w, e[6], delta0, delta1, delta2, delta3;
    sint32 i, j, stage, step, dstep, even;
    P4 q;
    P5 *d, *p0, *p1, *p2, *p3;
    P3 *pp;

    if (Counter.flags & FRACT_FLAG) {
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
    }

    if (level < 0 || level > 4)
        exit(0);

    levels = -1;

    srand(in_seed);

    for (C = level, B = 2, A = 1, i = 0; i < level; A = B, B <<= 1, i++)
        ;

    if (Counter.flags & FRACT_FLAG) {
        for (i = 0; i < 6; i += 2) {
            j = rand() - 16384;

            e[i] = -1.0 + 0.2 * roughness * (flot32)j / 16384.0;
            e[i + 1] = 1.0 + 0.2 * roughness * (flot32)j / 16384.0;
        }

        d = &dt[0][0];
        d->p.x = 0;
        d->p.y = e[0];
        d->p.z = 0;
        d->seed = rand();
        d = &dt[0][A];
        d->p.x = 0;
        d->p.y = 0;
        d->p.z = e[4];
        d->seed = rand();
        dt[0][B] = dt[0][0];
        d = &dt[A][0];
        d->p.x = e[2];
        d->p.y = 0;
        d->p.z = 0;
        d->seed = rand();
        d = &dt[A][A];
        d->p.x = 0;
        d->p.y = e[1];
        d->p.z = 0;
        d->seed = rand();
        d = &dt[A][B];
        d->p.x = e[3];
        d->p.y = 0;
        d->p.z = 0;
        d->seed = rand();
        dt[B][0] = dt[0][0];
        d = &dt[B][A];
        d->p.x = 0;
        d->p.y = 0;
        d->p.z = e[5];
        d->seed = rand();
        dt[B][B] = dt[0][0];
    } else {
        d = &dt[0][0];
        q.x = 0;
        q.y = -1.0;
        q.z = 0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
        d = &dt[0][A];
        q.x = 0;
        q.y = 0;
        q.z = -1.0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
        d = &dt[0][B];
        q.x = 0;
        q.y = -1.0;
        q.z = 0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
        d = &dt[A][0];
        q.x = -1.0;
        q.y = 0;
        q.z = 0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
        d = &dt[A][A];
        q.x = 0;
        q.y = 1.0;
        q.z = 0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
        d = &dt[A][B];
        q.x = 1.0;
        q.y = 0;
        q.z = 0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
        d = &dt[B][0];
        q.x = 0;
        q.y = -1.0;
        q.z = 0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
        d = &dt[B][A];
        q.x = 0;
        q.y = 0;
        q.z = 1.0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
        d = &dt[B][B];
        q.x = 0;
        q.y = -1.0;
        q.z = 0;
        d->seed = convert(&q);
        d->p.x = q.x * q.w;
        d->p.y = q.y * q.w;
        d->p.z = q.z * q.w;
    }

    for (step = A / 2, dstep = A, stage = 1; stage <= level; dstep = step, step >>= 1, stage++)
        for (i = dstep; i <= B; i += dstep)
            for (j = step; j <= B; j += dstep) {
                fract_mid(j - step, i, j + step, i, (flot32)roughness / stage);
                fract_mid(j - step, i - dstep, j + step, i - dstep, (flot32)roughness / stage);
                fract_mid(j - step, i, j - step, i - dstep, (flot32)roughness / stage);
                fract_mid(j + step, i, j + step, i - dstep, (flot32)roughness / stage);

                if ((((i - dstep) & A) + ((j - step) & A)) & A)
                    fract_mid(j - step, i - dstep, j + step, i, (flot32)roughness / stage);
                else
                    fract_mid(j - step, i, j + step, i - dstep, (flot32)roughness / stage);
            }

    for (i = 0; i < B; i++)
        for (j = 0; j < B; j++) {
            dt[i][j].valid = 0;
            dt[i][j].quadw = 0;
            dt[i][j].sur = 0;

            if (((i & A) + (j & A)) & A) {
                bound_normals(&dt[i][j].p, &dt[i][j + 1].p, &dt[i + 1][j + 1].p, &dt[i][j].v0[0]);
                bound_normals(&dt[i][j].p, &dt[i + 1][j].p, &dt[i + 1][j + 1].p, &dt[i][j].v1[0]);
            } else {
                bound_normals(&dt[i][j].p, &dt[i][j + 1].p, &dt[i + 1][j].p, &dt[i][j].v0[0]);
                bound_normals(&dt[i + 1][j + 1].p, &dt[i][j + 1].p, &dt[i + 1][j].p, &dt[i][j].v1[0]);
            }

            even = ((((j & A) + (i & A)) & A) ? 1 : 0);

            if (!even) {
                p0 = &dt[i + 0][j + 0];
                p1 = &dt[i + 0][j + 1];
                p2 = &dt[i + 1][j + 0];
                p3 = &dt[i + 1][j + 1];
            } else {
                p0 = &dt[i + 0][j + 1];
                p1 = &dt[i + 1][j + 1];
                p2 = &dt[i + 0][j + 0];
                p3 = &dt[i + 1][j + 0];
            }

            delta0 = fsqrt(p0->p.x * p0->p.x + p0->p.y * p0->p.y + p0->p.z * p0->p.z);
            delta1 = fsqrt(p1->p.x * p1->p.x + p1->p.y * p1->p.y + p1->p.z * p1->p.z);
            delta2 = fsqrt(p2->p.x * p2->p.x + p2->p.y * p2->p.y + p2->p.z * p2->p.z);
            delta3 = fsqrt(p3->p.x * p3->p.x + p3->p.y * p3->p.y + p3->p.z * p3->p.z);

            pp = dt[i][j].pp;
            pp[0].x = p0->p.x / delta0;
            pp[0].y = p0->p.y / delta0;
            pp[0].z = p0->p.z / delta0;
            pp[2].x = p1->p.x / delta1;
            pp[2].y = p1->p.y / delta1;
            pp[2].z = p1->p.z / delta1;
            pp[4].x = p2->p.x / delta2;
            pp[4].y = p2->p.y / delta2;
            pp[4].z = p2->p.z / delta2;
            pp[6].x = p3->p.x / delta3;
            pp[6].y = p3->p.y / delta3;
            pp[6].z = p3->p.z / delta3;

            pp[1].x = 1.25 * pp[0].x;
            pp[1].y = 1.25 * pp[0].y;
            pp[1].z = 1.25 * pp[0].z;
            pp[3].x = 1.25 * pp[2].x;
            pp[3].y = 1.25 * pp[2].y;
            pp[3].z = 1.25 * pp[2].z;
            pp[5].x = 1.25 * pp[4].x;
            pp[5].y = 1.25 * pp[4].y;
            pp[5].z = 1.25 * pp[4].z;
            pp[7].x = 1.25 * pp[6].x;
            pp[7].y = 1.25 * pp[6].y;
            pp[7].z = 1.25 * pp[6].z;
        }

    status.status = -1;
    status.xi = 0;
    status.yi = 0;
    status.flag = 0;
}

/**********************************************************************
 *  display_fractsphere()  -
 **********************************************************************/
void display_fractsphere(void)

{
    sint32 i, j, flag;
    uint32 hey;
    flot32 vian;
    P5 *p;
    V4 *v;
    V3 fevt;

    for (i = 0; i < 8; i++)
        dbg[i] = 0;

    if (Counter.flags & FRACT_FLAG)
        vian = 1.0 / (vids + 1.0) - 0.040;
    else
        vian = 1.0 / (vids + 1.0) - 0.025;

    fevt.x = feye.x / feye.w;
    fevt.y = feye.y / feye.w;
    fevt.z = feye.z / feye.w;

    if (vids > 64.00)
        i = 0 + Counter.hw_graphics;
    else if (vids > 16.00)
        i = 1 + Counter.hw_graphics;
    else if (vids > 4.00)
        i = 2 + Counter.hw_graphics;
    else if (vids > 1.00)
        i = 3 + Counter.hw_graphics;
    else if (vids > 0.25)
        i = 4 + Counter.hw_graphics;
    else
        i = 5 + Counter.hw_graphics;

    if (i < 0)
        i = 0;
    if (i > 6)
        i = 6;

    if (i != levels) {
        Counter.flags |= FREEZ_FLAG;
        levels = i;
        for (j = 1, i = 0; i < levels; j <<= 2, i++)
            ;
        printf("New Level: %d, Triangles: 2048*%d\n", i, j);
        status.status = -1;

        for (i = 0; i < B; i++)
            for (p = dt[i], j = 0; j < B; p++, j++)
                destroy_continent(p);
    }

    for (sidsz = 1, i = 0; i < levels; sidsz <<= 1, i++)
        ;

    /* are we still on the home plate */
    if (status.status == 1) {
        status.status = 0;
        v = (status.flag ? &dt[status.yi][status.xi].v1[0] : &dt[status.yi][status.xi].v0[0]);

        if (v[0].x * feye.x + v[0].y * feye.y + v[0].z * feye.z >= 0.0)
            if (v[1].x * feye.x + v[1].y * feye.y + v[1].z * feye.z >= 0.0)
                if (v[2].x * feye.x + v[2].y * feye.y + v[2].z * feye.z >= 0.0)
                    status.status = 1;
    }

    /* find and create new new home plate */
    if (status.status != 1)
        for (i = 0; i < B; i++)
            for (p = dt[i], j = 0; j < B; p++, j++) {
                if (p->v0[0].x * feye.x + p->v0[0].y * feye.y + p->v0[0].z * feye.z >= 0.0)
                    if (p->v0[1].x * feye.x + p->v0[1].y * feye.y + p->v0[1].z * feye.z >= 0.0)
                        if (p->v0[2].x * feye.x + p->v0[2].y * feye.y + p->v0[2].z * feye.z >= 0.0) {
                            status.xi = j;
                            status.yi = i;
                            status.flag = 0;

                            if (flag = (LOWER_TRI & ~p->valid) & BOTHH_TRI)
                                create_continent(p, flag, ((((j & A) + (i & A)) & A) ? 1 : 0));

                            status.status = 1;
                            enhanced_bound_normals(p, LOWER_TRI);

                            dbg[7]++;
                            goto d_FOUND;
                        }

                if (p->v1[0].x * feye.x + p->v1[0].y * feye.y + p->v1[0].z * feye.z >= 0.0)
                    if (p->v1[1].x * feye.x + p->v1[1].y * feye.y + p->v1[1].z * feye.z >= 0.0)
                        if (p->v1[2].x * feye.x + p->v1[2].y * feye.y + p->v1[2].z * feye.z >= 0.0) {
                            status.xi = j;
                            status.yi = i;
                            status.flag = 1;

                            if (flag = (UPPER_TRI & ~p->valid) & BOTHH_TRI)
                                create_continent(p, flag, ((((j & A) + (i & A)) & A) ? 1 : 0));

                            status.status = 1;
                            enhanced_bound_normals(p, UPPER_TRI);

                            dbg[7]++;
                            goto d_FOUND;
                        }
            }

d_FOUND:
    i = i;

    /* generate new plates, destroy old plates, draw valid plates */
    for (i = 0; i < B; i++)
        for (p = dt[i], j = 0; j < B; p++, j++) {
            flag = 0;

            v = &p->v0[4];
            if (v->x * fevt.x + v->y * fevt.y + v->z * fevt.z > vian)
                flag |= LOWER_TRI;

            v = &p->v1[4];
            if (v->x * fevt.x + v->y * fevt.y + v->z * fevt.z > vian)
                flag |= UPPER_TRI;

            if (flag) {
                if (flag = (flag & ~p->valid) & BOTHH_TRI)
                    create_continent(p, flag, ((((j & A) + (i & A)) & A) ? 1 : 0));

                if (p->sur) {
                    flag = p->valid & clipp(p);
                    if (Counter.flags & SHADE_FLAG)
                        generate_shades(p, flag);
                    cpack_continent(p, flag);
                }
            } else if (p->sur) {
                hey = (uintptr_t)p->sur;
                free((void *)(hey - ((p->quadw) ? 0 : 8)));

                p->valid = 0;
                p->quadw = 0;
                p->sur = 0;
            }
        }

    /* intersect home plate */
    intersect_continent();

    if ((Counter.flags & PRINT_FLAG) && (dbg[0] + dbg[1] + dbg[7]))
        printf("Create: %d/%d Display: %02d/%02d Enhance: %d\n", dbg[0], dbg[1], dbg[2], dbg[3], dbg[7]);
}

/**********************************************************************
 *  destroy_fractsphere()  -
 **********************************************************************/
void destroy_fractsphere(void)

{
    sint32 i, j;
    P5 *p;

    for (i = 0; i < B; i++)
        for (p = dt[i], j = 0; j < B; p++, j++)
            destroy_continent(p);
}

/**********************************************************************
 *  create_continent()  -
 **********************************************************************/
static void create_continent(P5 *d, sint32 create, sint32 even)

{
    sint32 cstat, size;
    sint32 hey;
    TrigPoint *sur, *p0, *p1, *p2, *p3;
    P3 *p;
    flot32 w;

    cstat = clipp(d);

    if (status.status == 1)
        create &= cstat;

    if (!create)
        return;

    if (!d->sur) {
        size = 8 + (sidsz + 1) * (sidsz + 1) * sizeof(TrigPoint);

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
        p1 = p0 + sidsz;
        p2 = sur + (sidsz + 1) * sidsz;
        p3 = p2 + sidsz;

        if (!even) {
            p0->plan = (d + 0)->p;
            p0->seed = (d + 0)->seed;
            p1->plan = (d + 1)->p;
            p1->seed = (d + 1)->seed;
            p2->plan = (d + 33)->p;
            p2->seed = (d + 33)->seed;
            p3->plan = (d + 34)->p;
            p3->seed = (d + 34)->seed;
        } else {
            p0->plan = (d + 1)->p;
            p0->seed = (d + 1)->seed;
            p1->plan = (d + 34)->p;
            p1->seed = (d + 34)->seed;
            p2->plan = (d + 0)->p;
            p2->seed = (d + 0)->seed;
            p3->plan = (d + 33)->p;
            p3->seed = (d + 33)->seed;
        }

        p0->delta = fsqrt(p0->plan.x * p0->plan.x + p0->plan.y * p0->plan.y + p0->plan.z * p0->plan.z);
        p1->delta = fsqrt(p1->plan.x * p1->plan.x + p1->plan.y * p1->plan.y + p1->plan.z * p1->plan.z);
        p2->delta = fsqrt(p2->plan.x * p2->plan.x + p2->plan.y * p2->plan.y + p2->plan.z * p2->plan.z);
        p3->delta = fsqrt(p3->plan.x * p3->plan.x + p3->plan.y * p3->plan.y + p3->plan.z * p3->plan.z);
    }

    if (create) {
        if (create == BOTHH_TRI)
            dbg[0] += 2;
        else
            dbg[1]++;

        if (Counter.flags & FRACT_FLAG)
            if (d->sur) {
                p1 = d->sur + sidsz;
                p2 = d->sur + (sidsz + 1) * sidsz;

                if (!even) {
                    p1->plan = (d + 1)->p;
                    p2->plan = (d + 33)->p;
                } else {
                    p1->plan = (d + 34)->p;
                    p2->plan = (d + 0)->p;
                }
            }

        if (!d->sur)
            d->sur = sur;

        land_generation(d, create);

        if (Counter.flags & FRACT_FLAG)
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
        p->quadw = 0;
        p->sur = 0;
    }
} /**********************************************************************
   *  land_generation()  -
   **********************************************************************/
static void land_generation(P5 *d, sint32 triflag)

{
    sint32 stage, step, dstep, i, j, s, q, sp;
    flot32 x, y, z, w, rg;
    TrigPoint *sur, *c, *c1, *c2, *c3, *c4;
    P4 p;

    sur = d->sur;

    step = 1 << (levels - 1);
    rg = roughness / ((1.0 + C) * 16384.0);

    for (dstep = step + step, stage = 1; stage <= levels; rg *= 0.50, dstep = step, step >>= 1, stage++) {
        sp = step * (sidsz + 1);

        for (i = dstep; i <= sidsz; i += dstep) {
            c1 = sur + i - dstep;
            c2 = sur + i;
            c3 = sur + dstep * (sidsz + 1) + i - dstep;
            c4 = sur + dstep * (sidsz + 1) + i;

            for (j = step; j <= sidsz; j += dstep) {
                q = sidsz + step - i - j;

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
    flot32 a, b, w, m;
    TrigPoint *sur, *trg;

    sur = d->sur;

    a = LEVEL_SEA;
    b = LEVEL_SNOW;

    for (i = 0; i <= sidsz; i++)
        for (trg = sur + (sidsz + 1) * i, j = 0; j <= sidsz; trg++, j++) {
            q = sidsz - i - j;

            if (((triflag & LOWER_TRI) && (q >= 0)) || ((triflag & UPPER_TRI) && (q <= 0))) {
                w = trg->delta;

                if (w <= a) {
                    trg->plan.x /= w;
                    trg->plan.y /= w;
                    trg->plan.z /= w;
                }

                if (w > 1.0 + (1.0 - 2.0 * trg->plan.y * trg->plan.y) * roughness)
                    trg->label = MAT_WHITE;
                else if (w <= a)
                    trg->label = MAT_SEABLUE;
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
    flot32 x, y, z, w;
    V3 v1, v2;
    TrigPoint *sur, *p00, *p10;
    C6 *cc, cl;

    sur = d->sur;

    for (i = 0; i < sidsz; i++) {
        p00 = sur + (sidsz + 1) * (i);
        p10 = sur + (sidsz + 1) * (i + 1);

        for (j = 0; j < sidsz; p00++, p10++, j++) {
            q = sidsz - i - j;

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
                        w /= fsqrt(x * x + y * y + z * z);
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
                        w /= (3.0 * fsqrt(x * x + y * y + z * z));
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
                        w /= fsqrt(x * x + y * y + z * z);
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
                        w /= (3.0 * fsqrt(x * x + y * y + z * z));
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
 *  bound_normals()  -
 **********************************************************************/
static void bound_normals(P3 *p0, P3 *p1, P3 *p2, V4 *v)

{
    flot32 x, y, z, w;
    V4 v1, v2;

    x = p1->y * p0->z - p0->y * p1->z;
    y = p0->x * p1->z - p1->x * p0->z;
    z = p1->x * p0->y - p0->x * p1->y;
    w = -(x * p0->x + y * p0->y + z * p0->z);
    if (x * p2->x + y * p2->y + z * p2->z + w < 0.0) {
        x = -x;
        y = -y;
        z = -z;
        w = -w;
    }
    v[0].x = x;
    v[0].y = y;
    v[0].z = z;
    v[0].w = w;

    x = p2->y * p1->z - p1->y * p2->z;
    y = p1->x * p2->z - p2->x * p1->z;
    z = p2->x * p1->y - p1->x * p2->y;
    w = -(x * p1->x + y * p1->y + z * p1->z);
    if (x * p0->x + y * p0->y + z * p0->z + w < 0.0) {
        x = -x;
        y = -y;
        z = -z;
        w = -w;
    }
    v[1].x = x;
    v[1].y = y;
    v[1].z = z;
    v[1].w = w;

    x = p0->y * p2->z - p2->y * p0->z;
    y = p2->x * p0->z - p0->x * p2->z;
    z = p0->x * p2->y - p2->x * p0->y;
    w = -(x * p2->x + y * p2->y + z * p2->z);
    if (x * p1->x + y * p1->y + z * p1->z + w < 0.0) {
        x = -x;
        y = -y;
        z = -z;
        w = -w;
    }
    v[2].x = x;
    v[2].y = y;
    v[2].z = z;
    v[2].w = w;

    v1.x = p2->x - p0->x;
    v1.y = p2->y - p0->y;
    v1.z = p2->z - p0->z;

    v2.x = p1->x - p0->x;
    v2.y = p1->y - p0->y;
    v2.z = p1->z - p0->z;

    x = v2.y * v1.z - v1.y * v2.z;
    y = v1.x * v2.z - v2.x * v1.z;
    z = v2.x * v1.y - v1.x * v2.y;
    w = fsqrt(x * x + y * y + z * z);
    x /= w;
    y /= w;
    z /= w;
    w = -(x * p2->x + y * p2->y + z * p2->z);
    v[3].x = x;
    v[3].y = y;
    v[3].z = z;
    v[3].w = w;

    x = p0->x + p1->x + p2->x;
    y = p0->y + p1->y + p2->y;
    z = p0->z + p1->z + p2->z;
    w = fsqrt(x * x + y * y + z * z);
    v[4].x = x / w;
    v[4].y = y / w;
    v[4].z = z / w;
    v[4].w = 0.0;
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

    return (~resu & BOTHH_TRI);
}

/**********************************************************************
 *  cpack_continent()  -
 **********************************************************************/
static void cpack_continent(P5 *d, uint32 flag)

{
    sint32 i, j;
    TrigPoint *trp, *trq, *srp, *srq;

    if ((uintptr_t)d->sur & 0x0f) {
        printf("Error: data is not quad-word aligned: %p!\\n", (void *)d->sur);
        exit(1);
    }

    switch (flag) {
    case 0:
        return;
        break;

    case BOTHH_TRI:
        for (srp = d->sur, srq = srp + sidsz + 1, j = sidsz; j > 0; srp = srq, srq += sidsz + 1, j--) {
            glBegin(GL_TRIANGLE_STRIP);
            glVertex3fv(&srp->plan.x);
            glVertex3fv(&srq->plan.x);

            for (trp = srp, trq = srq + 1, i = sidsz; i > 0; trp++, trq++, i--) {
                glColor4ubv((unsigned char *)&trp->cpack1);
                glVertex3fv(&(trp + 1)->plan.x);

                glColor4ubv((unsigned char *)&trp->cpack2);
                glVertex3fv(&trq->plan.x);
            }
            glEnd();
        }

        dbg[2] += 2;
        break;

    case LOWER_TRI:
        for (srp = d->sur, j = sidsz; j > 0; srp += sidsz + 1, j--) {
            glBegin(GL_TRIANGLE_STRIP);
            glVertex3fv(&srp->plan.x);
            glVertex3fv(&(srp + sidsz + 1)->plan.x);

            for (trp = srp, i = j - 1; i > 0; i--) {
                glColor4ubv((unsigned char *)&trp->cpack1);
                glVertex3fv(&(trp + 1)->plan.x);

                glColor4ubv((unsigned char *)&trp->cpack2);
                glVertex3fv(&(trp + sidsz + 2)->plan.x);
                trp++;
            }

            glColor4ubv((unsigned char *)&trp->cpack1);
            glVertex3fv(&(trp + 1)->plan.x);
            glEnd();
        }

        dbg[3]++;
        break;

    case UPPER_TRI:
        for (srp = (d->sur + (sidsz + 1) * sidsz + sidsz), j = sidsz; j > 0; srp -= sidsz + 1, j--) {
            glBegin(GL_TRIANGLE_STRIP);
            glVertex3fv(&srp->plan.x);
            glVertex3fv(&(srp - sidsz - 1)->plan.x);

            for (trp = srp, i = j - 1; i > 0; i--) {
                glColor4ubv((unsigned char *)&(trp - sidsz - 2)->cpack2);
                glVertex3fv(&(trp - 1)->plan.x);

                glColor4ubv((unsigned char *)&(trp - sidsz - 2)->cpack1);
                glVertex3fv(&(trp - sidsz - 2)->plan.x);
                trp--;
            }

            glColor4ubv((unsigned char *)&(trp - sidsz - 2)->cpack2);
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
 *  enhanced_bound_normals()  -
 **********************************************************************/
static void enhanced_bound_normals(P5 *d, sint32 triflag)

{
    sint32 i, j;
    TrigPoint *sur, *c0;
    Bounder *z0;

    sur = d->sur;

    if (triflag == LOWER_TRI)
        for (i = 0; i < sidsz; i++)
            for (j = 0; j < sidsz - i; j++) {
                c0 = sur + i * (sidsz + 1) + j;
                z0 = &checkcrash[i][j];

                bound_normals(&c0->plan, &(c0 + sidsz + 1)->plan, &(c0 + 1)->plan, &z0->v1[0]);

                if (i + j < sidsz - 1)
                    bound_normals(&(c0 + sidsz + 2)->plan, &(c0 + 1)->plan, &(c0 + sidsz + 1)->plan, &z0->v2[0]);
            }
    else {
        for (i = 0; i < sidsz; i++)
            for (j = 0; j < sidsz - i; j++) {
                c0 = sur + (sidsz - i) * (sidsz + 1) + sidsz - j;
                z0 = &checkcrash[i][j];

                bound_normals(&c0->plan, &(c0 - sidsz - 1)->plan, &(c0 - 1)->plan, &z0->v1[0]);

                if (i + j < sidsz - 1)
                    bound_normals(&(c0 - sidsz - 2)->plan, &(c0 - 1)->plan, &(c0 - sidsz - 1)->plan, &z0->v2[0]);
            }
    }
}

/**********************************************************************
 *  intersect_continent()  -
 **********************************************************************/
static void intersect_continent(void)

{
    sint32 i, j, crash;
    Bounder *bnd;
    flot32 x, y, z;
    static sint32 flag;

    if (feye.w > 1.5)
        return;

    x = feye.x;
    y = feye.y;
    z = feye.z;

    for (crash = i = 0; i < sidsz; i++) {
        bnd = checkcrash[i];

        for (j = 0; j < sidsz - i; bnd++, j++) {
            if (bnd->v1[0].x * x + bnd->v1[0].y * y + bnd->v1[0].z * z + bnd->v1[0].w >= 0.0)
                if (bnd->v1[1].x * x + bnd->v1[1].y * y + bnd->v1[1].z * z + bnd->v1[1].w >= 0.0)
                    if (bnd->v1[2].x * x + bnd->v1[2].y * y + bnd->v1[2].z * z + bnd->v1[2].w >= 0.0) {
                        if (bnd->v1[3].x * x + bnd->v1[3].y * y + bnd->v1[3].z * z + bnd->v1[3].w <= 0.0)
                            crash = 1;
                        goto INTERSECT_END;
                    }

            if (i + j < sidsz - 1) {
                if (bnd->v2[0].x * x + bnd->v2[0].y * y + bnd->v2[0].z * z + bnd->v2[0].w >= 0.0)
                    if (bnd->v2[1].x * x + bnd->v2[1].y * y + bnd->v2[1].z * z + bnd->v2[1].w >= 0.0)
                        if (bnd->v2[2].x * x + bnd->v2[2].y * y + bnd->v2[2].z * z + bnd->v2[2].w >= 0.0) {
                            if (bnd->v2[3].x * x + bnd->v2[3].y * y + bnd->v2[3].z * z + bnd->v2[3].w <= 0.0)
                                crash = 2;
                            goto INTERSECT_END;
                        }
            }
        }
    }

INTERSECT_END:
    if (crash) {
        t_body *tb;
        extern t_boss flaggs;

        spRingBell();

        tb = (t_body *)flaggs.star[flaggs.suun_current].next[flaggs.plan_current];
        x = fsqrt(tb->posit.x * tb->posit.x + tb->posit.y * tb->posit.y + tb->posit.z * tb->posit.z);

        Counter.eye.x -= (tb->posit.x + 16.0) / x;
        Counter.eye.y -= (tb->posit.y + 16.0) / x;
        Counter.eye.z -= (tb->posit.z + 16.0) / x;
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

    printf("Continent: %p\n", d->sur);

    for (i = 0; i <= sidsz; i++)
        for (j = 0; j <= sidsz; j++) {
            printf("%d %d:", i, j);
            p_trigpoint(d->sur + i * (sidsz + 1) + j);
        }
}
