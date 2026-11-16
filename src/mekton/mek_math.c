/*
 * mek_math.c - vector and matrix helpers of Mekton.
 *
 * Matrices use Performer's convention: row vectors, translation in row 3.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "mekton.h"

/* arc sine in tenths of a degree; the original used three tables */
short xasin(float x)
{
    if (x > 1) x = 1;
    if (x < -1) x = -1;
    return (short)floor(PF_RAD2DEG(asin(x)) * 10.0 + 0.5);
}

/* sine and cosine of an angle in degrees (sincos_table with 0.1 degree
 * steps and linear interpolation in the original) */
void fsincos(float deg, float *s, float *c)
{
    pfSinCos(deg, s, c);
}

/* random number in [-n, n] */
int frandom(int n)
{
    static unsigned int seed = 1;
    int bits = 1, r;
    if (n <= 0)
        return 0;
    while (bits < 31 && n >= (1 << bits))
        bits++;
    do {
        seed = seed * 1103515245u + 12345u;
        r = (int)((seed & 0x7fffffff) >> (31 - bits));
    } while (r > n);
    seed = seed * 1103515245u + 12345u;
    return (seed & 0x40000000u) ? -r : r;
}

float range(const pfVec3 a, const pfVec3 b)
{
    return pfSqrt(range_sq(a, b));
}

float range_sq(const pfVec3 a, const pfVec3 b)
{
    float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return x * x + y * y + z * z;
}

/* point at distance d from a towards b */
void vec_dist(const pfVec3 a, const pfVec3 b, float d, pfVec3 out)
{
    float f = d / range(a, b);
    if (f != f) {
        pfCopyVec3(out, a);
        fprintf(stderr, "vec_dist NAN\n");
        return;
    }
    out[0] = (b[0] - a[0]) * f + a[0];
    out[1] = (b[1] - a[1]) * f + a[1];
    out[2] = (b[2] - a[2]) * f + a[2];
}

/* heading, pitch and roll of a rotation matrix */
void mat2hpr(pfMatrix m, pfVec3 hpr)
{
    float sp = -m[1][2], p, cp;
    if (sp >= 1)
        p = 1.57079633f;
    else if (sp <= -1)
        p = -1.57079633f;
    else
        p = (float)asin(sp);
    hpr[1] = (float)(-p * 57.2957795);
    cp = (float)cos(-p);
    if (cp == 0)
        return;
    sp = -m[1][0] / cp;
    if (sp >= 1)
        hpr[0] = 90;
    else if (sp <= -1)
        hpr[0] = -90;
    else
        hpr[0] = (float)(asin(sp) * 57.2957795);
    if (m[1][1] < 0)
        hpr[0] = 180 - hpr[0];
    if (hpr[0] < 0)
        hpr[0] += 360;
    hpr[2] = xasin(-m[0][2] / cp) / 10.0f;
    if (m[2][2] < 0)
        hpr[2] = 180 - hpr[2];
    if (hpr[2] < 0)
        hpr[2] += 360;
}

/* hpr of the frame m once its up vector is turned onto up */
void orient(pfMatrix m, pfVec3 hpr, pfVec3 up)
{
    pfMatrix r;
    pfVec3 v;
    pfSetVec3(v, 0, 0, 1);
    pfXformVec3(v, v, m);
    pfMakeVecRotVecMat(r, v, up);
    pfSetVec3(v, 0, 1, 0);
    pfXformVec3(v, v, m);
    pfXformVec3(v, v, r);
    path_norm2hpr(v, up, hpr);
}

/* hpr of a frame looking along path with norm as its up vector */
void path_norm2hpr(const pfVec3 path, const pfVec3 norm, pfVec3 hpr)
{
    pfVec3 h, side;
    float d;

    h[0] = path[0];
    h[1] = path[1];
    h[2] = 0;
    pfNormalizeVec2(h);
    hpr[0] = 0 < h[0] ? -pfArcCos(h[1]) : pfArcCos(h[1]);
    if (hpr[0] < 0)
        hpr[0] += 360;
    d = path[2] * h[2] + h[0] * path[0] + h[1] * path[1];
    if (d > 1)
        hpr[1] = 0;
    else if (d < -1)
        hpr[1] = 180;
    else
        hpr[1] = pfArcCos(d);
    if (path[2] < 0)
        hpr[1] = -hpr[1];
    side[0] = -h[1];
    side[1] = h[0];
    side[2] = 0;
    d = norm[2] * side[2] + side[0] * norm[0] + side[1] * norm[1];
    if (d > 1)
        hpr[2] = 0;
    else if (d < -1)
        hpr[2] = 180;
    else
        hpr[2] = pfArcCos(d);
    if (norm[2] < 0)
        hpr[2] = -hpr[2];
    hpr[2] -= 90;
    if (hpr[2] < 0)
        hpr[2] += 360;
}

/* heading and pitch of a direction */
void path2hp(const pfVec3 path, pfVec3 hp)
{
    pfVec3 h;
    float d;
    h[0] = path[0];
    h[1] = path[1];
    h[2] = 0;
    pfNormalizeVec2(h);
    hp[0] = 0 < h[0] ? -pfArcCos(h[1]) : pfArcCos(h[1]);
    if (hp[0] < 0)
        hp[0] += 360;
    d = path[2] * h[2] + h[0] * path[0] + h[1] * path[1];
    if (d > 1)
        hp[1] = 0;
    else if (d < -1)
        hp[1] = 180;
    else
        hp[1] = pfArcCos(d);
    if (path[2] < 0)
        hp[1] = -hp[1];
}

void hpr2path_norm(const pfVec3 hpr, pfVec3 path, pfVec3 norm)
{
    pfMatrix m;
    pfMakeEulerMat(m, hpr[0], hpr[1], hpr[2]);
    pfSetVec3(path, 0, 1, 0);
    pfXformVec3(path, path, m);
    pfSetVec3(norm, 0, 0, 1);
    pfXformVec3(norm, norm, m);
}

void make_inv_euler_mat(const pfVec3 hpr, pfMatrix m)
{
    identify_matrix(m);
    matrix_rotate(m, -hpr[2], 'y');
    matrix_rotate(m, -hpr[1], 'x');
    matrix_rotate(m, -hpr[0], 'z');
}

void xyz_hpr2inv_mat(const pfVec3 xyz, const pfVec3 hpr, pfMatrix m)
{
    make_inv_euler_mat(hpr, m);
    matrix_translate(m, -xyz[0], -xyz[1], -xyz[2]);
}

void nearest_point_on_plane(const pfVec3 p, const pfVec3 n, const pfVec3 pt, pfVec3 out)
{
    float t = (pfDotVec3(n, p) - pfDotVec3(n, pt)) / pfDotVec3(n, n);
    out[0] = p[0] - n[0] * t;
    out[1] = p[1] - n[1] * t;
    out[2] = p[2] - n[2] * t;
}

float dist_to_plane(const pfVec3 p, const pfVec3 n, const pfVec3 pt)
{
    pfVec3 q;
    nearest_point_on_plane(p, n, pt, q);
    return pfDistancePt3(p, q);
}

/* signed distance, positive on the side n points to */
float sdist_to_plane(const pfVec3 p, const pfVec3 n, const pfVec3 pt)
{
    pfVec3 q, d;
    nearest_point_on_plane(p, n, pt, q);
    pfSubVec3(d, p, q);
    pfNormalizeVec3(d);
    if (pfDotVec3(d, n) < 0)
        return -pfDistancePt3(p, q);
    return pfDistancePt3(p, q);
}

/* intersection of the segment p0 p1 with a plane */
int line_isect_plane(const pfVec3 p0, const pfVec3 p1, const pfVec3 n, const pfVec3 pt, pfVec3 out)
{
    pfVec3 d;
    float t;
    pfSubVec3(d, p1, p0);
    t = -(pfDotVec3(p0, n) - pfDotVec3(n, pt)) / pfDotVec3(d, n);
    if (!(0 <= t) || !(t <= 1))
        return 0;
    out[0] = d[0] * t + p0[0];
    out[1] = d[1] * t + p0[1];
    out[2] = d[2] * t + p0[2];
    return 1;
}

/* mirror of v about n: 2 (v.n) n - v */
void reflect(const pfVec3 v, const pfVec3 n, pfVec3 out)
{
    float d = v[2] * n[2] + n[0] * v[0] + n[1] * v[1];
    pfVec3 p;
    pfScaleVec3(p, d, n);
    out[0] = p[0] - v[0] + p[0];
    out[1] = p[1] - v[1] + p[1];
    out[2] = p[2] - v[2] + p[2];
}

void identify_matrix(pfMatrix m)
{
    pfMakeIdentMat(m);
}

/* b = a b */
void matrix_multiply(pfMatrix a, pfMatrix b, pfMatrix out)
{
    pfMatrix t;
    pfMultMat(t, a, b);
    pfCopyMat(out, t);
}

static void rot_mat(pfMatrix r, float deg, int axis)
{
    float s, c;
    pfSinCos(deg, &s, &c);
    pfMakeIdentMat(r);
    switch (axis) {
    case 'x': case 'X':
        r[1][1] = c; r[1][2] = s; r[2][1] = -s; r[2][2] = c;
        break;
    case 'y': case 'Y':
        r[0][0] = c; r[0][2] = -s; r[2][0] = s; r[2][2] = c;
        break;
    case 'z': case 'Z':
        r[0][0] = c; r[0][1] = s; r[1][0] = -s; r[1][1] = c;
        break;
    }
}

/* m = R m: rotation in the local frame of m */
void matrix_rotate(pfMatrix m, float deg, int axis)
{
    pfMatrix r;
    rot_mat(r, deg, axis);
    pfMultMat(m, r, m);
}

/* m = m R */
void matrix_post_rotate(pfMatrix m, float deg, int axis)
{
    pfMatrix r;
    rot_mat(r, deg, axis);
    pfMultMat(m, m, r);
}

/* m = T m: translation in the local frame of m */
void matrix_translate(pfMatrix m, float x, float y, float z)
{
    int j;
    for (j = 0; j < 4; j++)
        m[3][j] += x * m[0][j] + y * m[1][j] + z * m[2][j];
}

void mult_vec(const pfVec3 v, pfMatrix m, pfVec3 out)
{
    pfXformPt3(out, v, m);
}

float vec_len(const pfVec3 v)
{
    return pfLengthVec3(v);
}
