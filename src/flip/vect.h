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
 *	Definition for vector math.  Vectors are just arrays of 3 floats.
 */

#ifndef VECTDEF
#define VECTDEF

#ifndef _POLY9
#include <math.h>
#endif

float *vnew();
float *vclone();
void vcopy(float *, float *);
void vprint(float *);
void vset(float *, float, float, float);
void vzero(float *);
void vnormal(float *);
float vlength(float *);
void vscale(float *, float);
void vmult(float *, float *, float *);
void vadd(float *, float *, float *);
void vsub(float *, float *, float *);
void vhalf(float *, float *, float *);
float vdot(float *, float *);
void vcross(float *, float *, float *);
void vdirection(float *, float *);
void vreflect(float *, float *, float *);
void vmultmatrix(float [4][4], float [4][4], float [4][4]);
#endif VECTDEF
