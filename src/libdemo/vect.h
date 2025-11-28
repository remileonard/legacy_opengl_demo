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
 * vect:
 *	Functions to support operations on vectors and matrices.
 *
 * Original code from:
 * David M. Ciemiewicz, Mark Grossman, Henry Moreton, and Paul Haeberli
 *
 * Much mucking with by:
 * Gavin Bell
 */

#ifndef VECTDEF
#define VECTDEF

#include <math.h>
#include "porting/iris2ogl.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880   /* sqrt(2) */
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440 /* 1/sqrt(2) */
#endif

float *vnew();
float *vclone(const float *);
void vcopy(const float *, float *);
void vprint(const float *);
void vset(float *, float, float, float);
void vzero(float *);
void vnormal(float *);
float vlength(const float *);
void vscale(float *, float);
void vmult(const float *, const float *, float *);
void vadd(const float *, const float *, float *);
void vsub(const float *, const float *, float *);
void vhalf(const float *, const float *, float *);
float vdot(const float *, const float *);
void vcross(const float *, const float *, float *);
void vdirection(const float *, float *);
void vreflect(const float *, const float *, float *);
void vmultmatrix(const Matrix, const Matrix, Matrix);
void vtransform(const float *, const Matrix, float *);
void vtransform4(const float *, const Matrix, float *);

extern Matrix idmatrix;

void mcopy(const Matrix, Matrix);
void minvert(const Matrix, Matrix);
void vgetmatrix(Matrix m);
void linsolve(const float *[], int, float *);

#endif /* VECTDEF */
