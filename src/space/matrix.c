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

extern t_stopwatch Counter;

/**********************************************************************
 *  spLookMatrix()  -
 **********************************************************************/
void spLookMatrix(flot32 vx, flot32 vy, flot32 vz, flot32 px, flot32 py, flot32 pz, Matrix m)

{
    flot32 sine, cosi, hyp, hyp1, dx, dy, dz;
    Matrix fat, mat;

    dx = px - vx;
    dy = py - vy;
    dz = pz - vz;

    hyp = dx * dx + dz * dz; /* hyp squared  */
    hyp1 = fsqrt(dy * dy + hyp);
    hyp = fsqrt(hyp); /* the real hyp */

    if (hyp1 != 0.0) { /* rotate X     */
        sine = -dy / hyp1;
        cosi = hyp / hyp1;
    } else {
        sine = 0.0;
        cosi = 1.0;
    }

    mat[0][0] = 1.0;
    mat[0][1] = 0.0;
    mat[0][2] = 0.0;
    mat[0][3] = 0.0;
    mat[1][0] = 0.0;
    mat[1][1] = cosi;
    mat[1][2] = sine;
    mat[1][3] = 0.0;
    mat[2][0] = 0.0;
    mat[2][1] = -sine;
    mat[2][2] = cosi;
    mat[2][3] = 0.0;
    mat[3][0] = 0.0;
    mat[3][1] = 0.0;
    mat[3][2] = 0.0;
    mat[3][3] = 1.0;

    if (hyp != 0.0) { /* rotate Y     */
        sine = dx / hyp;
        cosi = -dz / hyp;
    } else {
        sine = 0.0;
        cosi = 1.0;
    }

    fat[0][0] = cosi;
    fat[0][1] = 0.0;
    fat[0][2] = -sine;
    fat[0][3] = 0.0;
    fat[1][0] = 0.0;
    fat[1][1] = 1.0;
    fat[1][2] = 0.0;
    fat[1][3] = 0.0;
    fat[2][0] = sine;
    fat[2][1] = 0.0;
    fat[2][2] = cosi;
    fat[2][3] = 0.0;
    fat[3][0] = 0.0;
    fat[3][1] = 0.0;
    fat[3][2] = 0.0;
    fat[3][3] = 1.0;

    spMultMatrix((flot32 *)m, (flot32 *)fat, (flot32 *)mat);
}

/**********************************************************************
 *  spTranMatrix()  -
 **********************************************************************/
void spTranMatrix(flot32 *mo, flot32 *mi)

{
    Matrix mm;
    flot32 *ms;
    sint32 flag = 0;

    if (mo == mi) {
        ms = mo;
        mo = &mm[0][0];
        flag = 1;
    }

    *(mo + 0) = *(mi + 0);
    *(mo + 1) = *(mi + 4);
    *(mo + 2) = *(mi + 8);
    *(mo + 3) = *(mi + 12);
    *(mo + 4) = *(mi + 1);
    *(mo + 5) = *(mi + 5);
    *(mo + 6) = *(mi + 9);
    *(mo + 7) = *(mi + 13);
    *(mo + 8) = *(mi + 2);
    *(mo + 9) = *(mi + 6);
    *(mo + 10) = *(mi + 10);
    *(mo + 11) = *(mi + 14);
    *(mo + 12) = *(mi + 3);
    *(mo + 13) = *(mi + 7);
    *(mo + 14) = *(mi + 11);
    *(mo + 15) = *(mi + 15);

    if (flag)
        spCopyMatrix(ms, mo);
}

/**********************************************************************
 *  spCopyMatrix()  -
 **********************************************************************/
void spCopyMatrix(flot32 *mo, flot32 *mi)

{
    *(mo + 0) = *(mi + 0);
    *(mo + 1) = *(mi + 1);
    *(mo + 2) = *(mi + 2);
    *(mo + 3) = *(mi + 3);
    *(mo + 4) = *(mi + 4);
    *(mo + 5) = *(mi + 5);
    *(mo + 6) = *(mi + 6);
    *(mo + 7) = *(mi + 7);
    *(mo + 8) = *(mi + 8);
    *(mo + 9) = *(mi + 9);
    *(mo + 10) = *(mi + 10);
    *(mo + 11) = *(mi + 11);
    *(mo + 12) = *(mi + 12);
    *(mo + 13) = *(mi + 13);
    *(mo + 14) = *(mi + 14);
    *(mo + 15) = *(mi + 15);
}

/**********************************************************************
 *  spMultMatrix()  -
 **********************************************************************/
void spMultMatrix(flot32 *mo, flot32 *m1, flot32 *m2)

{
    Matrix mm;
    flot32 *ms;
    sint32 flag = 0;

    if (mo == m1 || mo == m2) {
        ms = mo;
        mo = (float *)mm;
        flag = 1;
    }

    *(mo + 0) = *(m1 + 0) * *(m2 + 0) + *(m1 + 1) * *(m2 + 4) + *(m1 + 2) * *(m2 + 8);
    *(mo + 1) = *(m1 + 0) * *(m2 + 1) + *(m1 + 1) * *(m2 + 5) + *(m1 + 2) * *(m2 + 9);
    *(mo + 2) = *(m1 + 0) * *(m2 + 2) + *(m1 + 1) * *(m2 + 6) + *(m1 + 2) * *(m2 + 10);
    *(mo + 3) = 0.0;
    *(mo + 4) = *(m1 + 4) * *(m2 + 0) + *(m1 + 5) * *(m2 + 4) + *(m1 + 6) * *(m2 + 8);
    *(mo + 5) = *(m1 + 4) * *(m2 + 1) + *(m1 + 5) * *(m2 + 5) + *(m1 + 6) * *(m2 + 9);
    *(mo + 6) = *(m1 + 4) * *(m2 + 2) + *(m1 + 5) * *(m2 + 6) + *(m1 + 6) * *(m2 + 10);
    *(mo + 7) = 0.0;
    *(mo + 8) = *(m1 + 8) * *(m2 + 0) + *(m1 + 9) * *(m2 + 4) + *(m1 + 10) * *(m2 + 8);
    *(mo + 9) = *(m1 + 8) * *(m2 + 1) + *(m1 + 9) * *(m2 + 5) + *(m1 + 10) * *(m2 + 9);
    *(mo + 10) = *(m1 + 8) * *(m2 + 2) + *(m1 + 9) * *(m2 + 6) + *(m1 + 10) * *(m2 + 10);
    *(mo + 11) = 0.0;
    *(mo + 12) = 0.0;
    *(mo + 13) = 0.0;
    *(mo + 14) = 0.0;
    *(mo + 15) = 1.0;

    if (flag)
        spCopyMatrix(ms, mo);
}

/**********************************************************************
 *  spPerspMatrix()  -
 **********************************************************************/
void spPerspMatrix(flot32 min, flot32 max)

{
    float a, dz;
    Matrix mt;

    dz = max - min;
    a = Counter.fov / Counter.aspcratio;

    mt[0][0] = a;
    mt[0][1] = 0.0;
    mt[0][2] = 0.0;
    mt[0][3] = 0.0;
    mt[1][0] = 0.0;
    mt[1][1] = Counter.fov;
    mt[1][2] = 0.0;
    mt[1][3] = 0.0;
    mt[2][0] = 0.0;
    mt[2][1] = 0.0;
    mt[2][2] = -(max + min) / dz;
    mt[2][3] = -1.0;
    mt[3][0] = 0.0;
    mt[3][1] = 0.0;
    mt[3][2] = -2.0 * min * max / dz;
    mt[3][3] = 1.0;

    glLoadMatrixf((float *)mt);
}
