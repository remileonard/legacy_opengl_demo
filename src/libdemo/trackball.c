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
 *	Implementation of a virtual trackball.
 *	Implemented by Gavin Bell, lots of ideas from Thant Tessman and
 *		the August '88 issue of Siggraph's "Computer Graphics," pp. 121-129.
 *
 */
#include "trackball.h"

#define fsin(x)  (sinf(x))
#define fcos(x)  (cosf(x))
#define fsqrt(x) (sqrtf(x))

/*
 * This size should really be based on the distance from the center of
 * rotation to the point on the object underneath the mouse.  That
 * point would then track the mouse as closely as possible.  This is a
 * simple example, though, so that is left as an Exercise for the
 * Programmer.
 */
#define TRACKBALLSIZE  (0.8)

/*
 * Local function prototypes (not defined in trackball.h)
 */
float tb_project_to_sphere(float, float, float);
void normalize_quat(float [4]);

/*
 * Ok, simulate a track-ball.  Project the points onto the virtual
 * trackball, then figure out the axis of rotation, which is the cross
 * product of P1 P2 and O P1 (O is the center of the ball, 0,0,0)
 * Note:  This is a deformed trackball-- is a trackball in the center,
 * but is deformed into a hyperbolic sheet of rotation away from the
 * center.  This particular function was chosen after trying out
 * several variations.
 * 
 * It is assumed that the arguments to this routine are in the range
 * (-1.0 ... 1.0)
 */
void
trackball(float q[4], float p1x, float p1y, float p2x, float p2y)
{
	float a[3];	/* Axis of rotation */
	float phi;	/* how much to rotate about axis */
	float p1[3], p2[3], d[3];
	float t;

	if (p1x == p2x && p1y == p2y)
	{
		vzero(q); q[3] = 1.0; /* Zero rotation */
		return;
	}
/*
 * First, figure out z-coordinates for projection of P1 and P2 to
 * deformed sphere
 */
	vset(p1,p1x,p1y,tb_project_to_sphere(TRACKBALLSIZE,p1x,p1y));
	vset(p2,p2x,p2y,tb_project_to_sphere(TRACKBALLSIZE,p2x,p2y));

/*
 *	Now, we want the cross product of P1 and P2
 */
	vcross(p2,p1,a);

/*
 *	Figure out how much to rotate around that axis.
 */
	vsub(p1,p2,d);
	t = vlength(d) / (2.0*TRACKBALLSIZE);
	/*
	 * Avoid problems with out-of-control values...
	 */
	if (t > 1.0) t = 1.0;
	if (t < -1.0) t = -1.0;
	phi = 2.0 * asin(t);
	axis_to_quat(a,phi,q);
}

/*
 *	Given an axis and angle, compute quaternion.
 */
void
axis_to_quat(float a[3], float phi, float q[4])
{
    float mag;

    /* norme de l’axe */
    mag = a[0]*a[0] + a[1]*a[1] + a[2]*a[2];

    /* axe dégénéré ou numériquement pourri → pas de rotation */
    if (mag <= 1e-12f || mag != mag) { /* mag!=mag teste NaN */
        q[0] = q[1] = q[2] = 0.0f;
        q[3] = 1.0f;
        return;
    }

    mag = fsqrt(mag);
    q[0] = a[0] / mag;
    q[1] = a[1] / mag;
    q[2] = a[2] / mag;

    {
        float half = phi * 0.5f;
        float s = fsin(half);
        float c = fcos(half);
        q[0] *= s;
        q[1] *= s;
        q[2] *= s;
        q[3] = c;
    }
}

/*
 * Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
 * if we are away from the center of the sphere.
 */
static float
tb_project_to_sphere(float r, float x, float y)
{
    float d, t, z;

    d = fsqrt(x*x + y*y);
    if (d < r*M_SQRT1_2) {  /* Inside sphere */
        float arg = r*r - d*d;
        if (arg < 0.0f) arg = 0.0f;
        z = fsqrt(arg);
    } else {                        /* On hyperbola */
        t = r / M_SQRT2;
        z = t*t / d;
    }

    /* Petit plancher pour éviter un vecteur (0,0,0) exact */
    if (z == 0.0f) z = 1e-4f;

    return z;
}

/*
 * Given two rotations, e1 and e2, expressed as quaternion rotations,
 * figure out the equivalent single rotation and stuff it into dest.
 * 
 * This routine also normalizes the result every RENORMCOUNT times it is
 * called, to keep error from creeping in.
 *
 * NOTE: This routine is written so that q1 or q2 may be the same
 * as dest (or each other).
 */

#define RENORMCOUNT 97

void
add_quats(float q1[4], float q2[4], float dest[4])
{
	static int count=0;
	int i;
	float t1[4], t2[4], t3[4];
	float tf[4];

	vcopy(q1,t1); 
	vscale(t1,q2[3]);

	vcopy(q2,t2); 
	vscale(t2,q1[3]);

	vcross(q2,q1,t3);
	vadd(t1,t2,tf);
	vadd(t3,tf,tf);
	tf[3] = q1[3] * q2[3] - vdot(q1,q2);

	dest[0] = tf[0];
	dest[1] = tf[1];
	dest[2] = tf[2];
	dest[3] = tf[3];

	if (++count > RENORMCOUNT)
	{
		count = 0;
		normalize_quat(dest);
	}
}

/*
 * Quaternions always obey:  a^2 + b^2 + c^2 + d^2 = 1.0
 * If they don't add up to 1.0, dividing by their magnitued will
 * renormalize them.
 *
 * Note: See the following for more information on quaternions:
 * 
 * - Shoemake, K., Animating rotation with quaternion curves, Computer
 *   Graphics 19, No 3 (Proc. SIGGRAPH'85), 245-254, 1985.
 * - Pletinckx, D., Quaternion calculus as a basic tool in computer
 *   graphics, The Visual Computer 5, 2-13, 1989.
 */
static void
normalize_quat(float q[4])
{
    float mag = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
    if (mag <= 0.0f) {
        q[0] = q[1] = q[2] = 0.0f;
        q[3] = 1.0f;
        return;
    }
    mag = fsqrt(mag);
    q[0] /= mag;
    q[1] /= mag;
    q[2] /= mag;
    q[3] /= mag;
}

/*
 * Build a rotation matrix, given a quaternion rotation.
 *
 */
void
build_rotmatrix(float m[4][4], float q[4])
{
	m[0][0] = 1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]);
	m[0][1] = 2.0 * (q[0] * q[1] - q[2] * q[3]);
	m[0][2] = 2.0 * (q[2] * q[0] + q[1] * q[3]);
	m[0][3] = 0.0;

	m[1][0] = 2.0 * (q[0] * q[1] + q[2] * q[3]);
	m[1][1] = 1.0 - 2.0 * (q[2] * q[2] + q[0] * q[0]);
	m[1][2] = 2.0 * (q[1] * q[2] - q[0] * q[3]);
	m[1][3] = 0.0;

	m[2][0] = 2.0 * (q[2] * q[0] - q[1] * q[3]);
	m[2][1] = 2.0 * (q[1] * q[2] + q[0] * q[3]);
	m[2][2] = 1.0 - 2.0 * (q[1] * q[1] + q[0] * q[0]);
	m[2][3] = 0.0;

	m[3][0] = 0.0;
	m[3][1] = 0.0;
	m[3][2] = 0.0;
	m[3][3] = 1.0;
}
