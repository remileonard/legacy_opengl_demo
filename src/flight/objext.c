/*
 * Copyright 1992, 1993, 1994, Silicon Graphics, Inc.
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
 *  flight/objext.c $Revision: 1.1 $
 *
 *  Extensions to gobj library routines for flight
 */

#include "flight.h"

/*
 *  avg_verts() averages vertices, to get the "center" of an object.
 */
void avg_verts(object_t *obj, float *cx, float *cy, float *cz) {
    geometry_t *g;
    int i, j, k = 0;
    float *v;
    float tx = 0.0, ty = 0.0, tz = 0.0;

    for (i = 0; i < obj->gcount; i++) {
        g = &obj->glist[i];
        for (j = 0; j < g->vcount; j++, k++) {
            v = g->vlist[j];
            tx += v[0];
            ty += v[1];
            tz += v[2];
        }
    }
    *cx = tx / (float)k;
    *cy = ty / (float)k;
    *cz = tz / (float)k;
}

/*
 *  remap_obj() remaps the colors in a color indexed object.
 *  You shouldn't call this with an object that has non color indexed
 *  geometry in it.
 */
void remap_obj(object_t *obj) {
    int i;

    for (i = 0; i < obj->gcount; i++)
        remap_geom(&obj->glist[i]);
}

/*
 *  remap_geom() remaps the colors in a color indexed geometry node.
 *  You shouldn't call this with a geometry node that is not colorindex.
 */
void remap_geom(geometry_t *g) {
    int i;

    switch (g->type) {
    case IMV_GEOM:
    case IPV_GEOM:
        for (i = 0; i < g->vcount; i++)
            g->clist[i] = ci_table[g->clist[i]];
        break;
    case IMU_GEOM:
    case IPU_GEOM:
        for (i = 0; i < g->pcount; i++)
            g->plist[i].color = ci_table[g->plist[i].color];
        break;
    default:
        fprintf(stderr, "Warning: remap_geom() told to remap non ci geometry\n");
        break;
    }
}
