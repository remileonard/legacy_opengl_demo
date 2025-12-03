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

/*
 *  flight/cull.c $Revision: 1.1 $
 *
 *  culling routines
 */

#include "flight.h"
#include <math.h>

/*
 *  cull_sphere() returns TRUE if the sphere is outside the viewing frustom,
 *  FALSE other wise.
 */
cull_sphere(float *center, float radius) {
    int i;

    if (clip_planes[0][3] + DOT(clip_planes[0], center) > radius)
        return (TRUE);
    if (clip_planes[1][3] + DOT(clip_planes[1], center) > radius)
        return (TRUE);
    if (clip_planes[2][3] + DOT(clip_planes[2], center) > radius)
        return (TRUE);
    if (clip_planes[3][3] + DOT(clip_planes[3], center) > radius)
        return (TRUE);

    return (FALSE);
}

/*
 *  cull_shadow() returns TRUE if the shadow of plane pp is outside the
 *  viewing frustom, FALSE other wise.
 */
cull_shadow(Plane pp, float xf, float zf) {
    float v1[3], v2[3];
    float radius;

    radius = planeobj[pp->type]->radius;
    v1[X] = pp->x + (pp->y + radius) * xf;
    v1[Y] = 0;
    v1[Z] = pp->z + (pp->y + radius) * zf;
    v2[X] = pp->x + (pp->y - radius) * xf;
    v2[Y] = 0;
    v2[Z] = pp->z + (pp->y - radius) * zf;

    if (clip_planes[0][3] + DOT(clip_planes[0], v1) > radius && clip_planes[0][3] + DOT(clip_planes[0], v2) > radius)
        return (TRUE);
    if (clip_planes[1][3] + DOT(clip_planes[1], v1) > radius && clip_planes[1][3] + DOT(clip_planes[1], v2) > radius)
        return (TRUE);
    if (clip_planes[2][3] + DOT(clip_planes[2], v1) > radius && clip_planes[2][3] + DOT(clip_planes[2], v2) > radius)
        return (TRUE);
    if (clip_planes[3][3] + DOT(clip_planes[3], v1) > radius && clip_planes[3][3] + DOT(clip_planes[3], v2) > radius)
        return (TRUE);

    return (FALSE);
}
