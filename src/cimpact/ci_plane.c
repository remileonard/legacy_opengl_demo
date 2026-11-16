/*
 * ci_plane.c - Certain Impact: the airplane of the player (vgAirPlane and its
 * flight model), ballistic ammunition (vgBallistAmmoBank), the pair of
 * machine guns (vgMachGunPair) and the bombs (vgBombs).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

#include "ci.h"

/* rotation of deg degrees about an axis, for row vectors (Performer) */
static void rot_mat(pfMatrix m, float deg, float x, float y, float z)
{
    float s, c, t;
    pfSinCos(deg, &s, &c);
    t = 1 - c;
    pfMakeIdentMat(m);
    m[0][0] = t * x * x + c;     m[1][0] = t * x * y - s * z; m[2][0] = t * x * z + s * y;
    m[0][1] = t * x * y + s * z; m[1][1] = t * y * y + c;     m[2][1] = t * y * z - s * x;
    m[0][2] = t * x * z - s * y; m[1][2] = t * y * z + s * x; m[2][2] = t * z * z + c;
}

static void transpose(pfMatrix d, pfMatrix m)
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            d[i][j] = m[j][i];
}

static void pos_mat(pfMatrix m, const float *pos)
{
    pfCoord c;
    pfSetVec3(c.xyz, pos[0], pos[1], pos[2]);
    pfSetVec3(c.hpr, pos[3], pos[4], pos[5]);
    pfMakeCoordMat(m, &c);
}

/* angle of a body axis (row of the matrix) with the world up axis */
static float axis_up_angle(pfMatrix m, int row)
{
    float z = m[row][2];
    if (z > 1) z = 1;
    if (z < -1) z = -1;
    return pfArcCos(z);
}

/* --- creation ---------------------------------------------------------------------- */

static void type_of_code(int code, const char **dcs, const char **shadow, int *index)
{
    switch (code) {
    case 17: *dcs = "FokkerDCS"; *shadow = "FokkerShadow"; *index = 0; break;
    case 49: *dcs = "RedBaronDCS"; *shadow = "RedBaronShadow"; *index = 1; break;
    case 33: *dcs = "AlbatrosDCS"; *shadow = "AlbatrosShadow"; *index = 2; break;
    case 513: *dcs = "SopwithDCS"; *shadow = "SopwithShadow"; *index = 3; break;
    case 256: *dcs = "Fe2bDCS"; *shadow = "Fe2bShadow"; *index = 4; break;
    default: *dcs = "AmericanDCS"; *shadow = "AmericanShadow"; *index = 5; break;
    }
}

Plane *planeNew(int code, const float *init)
{
    Plane *p = (Plane *)calloc(1, sizeof(Plane));
    const char *dcs, *shadow;
    int index;
    type_of_code(code, &dcs, &shadow, &index);
    memcpy(p->init, init, sizeof(p->init));
    p->obj = vgFindObj(dcs);
    p->shadow = vgFindObj(shadow);
    vgObjModel(p->obj);
    vgObjModel(p->shadow);
    p->t = gTypes[index];
    p->code = code;
    p->side = code >= 256;
    switch (code) {
    case 17: p->gnd_pitch = 15; p->gear = 1.65f; break;
    case 33: p->gnd_pitch = 13; p->gear = 1.65f; break;
    case 49: p->gnd_pitch = 13; p->gear = 1.35f; break;
    case 256: p->gnd_pitch = 15; p->gear = 1.1f; break;
    case 513: p->gnd_pitch = 17; p->gear = 1.35f; break;
    default: p->gnd_pitch = 17; p->gear = 1.35f; break;
    }
    pfSetVec3(p->eye, 0, 0.5f, 0.7f);
    p->lwing = vgFindPart(p->obj, "lwing", 0);
    p->rwing = vgFindPart(p->obj, "rwing", 0);
    p->tailv = vgFindPart(p->obj, "tailv", 0);
    p->tailh = vgFindPart(p->obj, "tailh", 0);
    vgObjClass(p->obj, CL_USER);
    vgObjClass(p->shadow, CL_USER);
    p->obj->user = p;
    vgAddSceneObj(gMainScene, p->obj);
    planeReset(p);
    planeResetMotion(p);
    return p;
}

void planeDelete(Plane *p)
{
    vgRemSceneObj(gMainScene, p->obj);
    vgRemSceneObj(gMainScene, p->shadow);
    free(p);
}

static void init_parameters(Plane *p)
{
    p->state = 0;
    p->fuel = 1;
    p->damage = 0;
    p->damagef = 0;
    p->bombs = p->t.bombs;
    p->ammo = p->t.ammo;
    p->speedfrac = p->prev_speedfrac = p->fwdfrac = p->speed = 0;
    pfSetVec3(p->dir, 0, 0, 0);
    pfSetVec3(p->vel, 0, 0, 0);
    p->stall_pitch = p->stall_spin = 0;
    p->throttle = p->thrust_eff = p->stick_y = p->yaw = p->stick_x = 0;
    p->score = 0;
    /* motion model parameters */
    p->vmax = p->t.vmax;
    p->vstall = 0.2f * p->vmax;
    p->lift = 1.6666666f;
    p->landh = 0.1f;
    p->invvmax = 1 / p->vmax;
    p->gturn = 2.5f * p->t.agility;
    p->roll_rate = 0.8f * p->t.agility;
    p->pitch_rate = 0.35f * p->t.agility;
    p->stall_p = 0.3f;
    p->stall_s = 0.5f;
}

void planeReset(Plane *p)
{
    vgDISAppearance(p->obj, 0x400018, 0x400000);
    p->dead = 0;
    p->screamed = 0;
    p->engine_on = 0;
    p->dive_t = 0;
    p->start_time = gNow;
    init_parameters(p);
}

/* places the airplane on the ground at its initial position */
void planeResetMotion(Plane *p)
{
    float z, gp = 0, gr = 0;
    planeReset(p);
    p->state = 0;
    p->throttle = 0;
    if (vgIsectZ(gMainScene, CL_HOT, p->init[0], p->init[1], p->init[3], &z, &gp, &gr, NULL))
        p->ground = z + p->gear;
    else
        p->ground = -100;
    if (p->code & 1)
        gp += p->gnd_pitch;   /* tail dragger */
    p->pos[0] = p->init[0];
    p->pos[1] = p->init[1];
    p->pos[2] = p->ground;
    p->pos[3] = p->init[3];
    p->pos[4] = gp;
    p->pos[5] = gr;
    pos_mat(p->mat, p->pos);
    vgObjPosVec(p->obj, p->pos);
    pfSetVec3(p->vel, 0, 0, 0);
    p->start_time = gNow;
}

/* --- scoring and damages ---------------------------------------------------------- */

void planeAddScore(Plane *p, int n)
{
    if (gDifficulty == 1)
        p->score += n;
    else if (gDifficulty == 2)
        p->score += 2 * n;
}

void planeHit(Plane *p, int n, int bullet)
{
    (void)bullet;
    if (p->dead)
        return;
#ifdef CI_DEBUG
    fprintf(stderr, "hit %d (bullet %d) damage %d\n", n, bullet, p->damage + n);
#endif
#ifdef CI_DEBUG
    fprintf(stderr, "hit %d (bullet %d) damage %d\n", n, bullet, p->damage + n);
#endif
    p->damage += n;
    if (p->damage < 0)
        p->damage = 0;
    p->damagef = p->damage / 25.0f;
    if (p->damagef > 1) p->damagef = 1;
    if (p->damagef < 0) p->damagef = 0;
}

void planeExplode(Plane *p)
{
#ifdef CI_DEBUG
    fprintf(stderr, "explode state %d z %.1f ground %.1f\n", p->state, p->pos[2], p->ground);
#endif
#ifdef CI_DEBUG
    fprintf(stderr, "explode state %d z %.1f ground %.1f\n", p->state, p->pos[2], p->ground);
#endif
    p->state = 4;
    p->dead = 1;
    ciCreateAirExplosion(p->pos);
}

void planeSink(Plane *p)
{
    p->dead = 1;
    p->state = 3;
}

void planeDiveAndExplode(Plane *p)
{
#ifdef CI_DEBUG
    fprintf(stderr, "dive and explode\n");
#endif
#ifdef CI_DEBUG
    fprintf(stderr, "dive and explode\n");
#endif
    if (p->state == 0) {
        planeExplode(p);
        return;
    }
    ciChangeLayout(3);
    p->state = 5;
    p->dive_t = 0;
    p->dive_z = p->ground + 100 * p->speedfrac;
    vgDISAppearance(p->obj, 0x400018, 0x400018);
}

/* --- flight model (updateMotionModel__10vgAirPlane) -------------------------------- */

/*
 * The original works per frame at 30 Hz: the speed follows the throttle with
 * v = (0.005 throttle + 0.995 speedfrac) vmax each frame. The coefficients
 * are scaled here so that the behavior does not depend on the frame rate.
 */
static float get_thrust(Plane *p)
{
    float a = p->vmax / gDt * 0.999f, t = p->throttle, f30 = gDt * 30;
    float k = 1 - (float)pow(0.995, f30);
    p->thr_a = k * a;
    p->thr_b = (1 - k) * a;
    /* the engine gains power slowly near the ground */
    if (p->pos[2] - p->ground < 10 && p->thrust_eff + 0.005f * f30 < t)
        t = p->thrust_eff + 0.005f * f30;
    p->thrust_eff = t;
    return t * p->thr_a + p->speedfrac * p->thr_b;
}

static unsigned int hit_class(vgHit *h) { return h->hit ? h->cls : 0; }

static void update_landing(Plane *p, vgHit *ground)
{
    unsigned int c = hit_class(ground);
    pfVec3 v;
    if (c == CL_WATER)
        p->state = 3;
    else if (c == CL_GROUND)
        p->state = 0;
    else
        p->state = 4;
    /* too much bank or a too steep descent */
    if (axis_up_angle(p->mat, 2) > 40)
        planeExplode(p);
    pfCopyVec3(v, p->vel);
    if (pfNormalizeVec3(v) > 0 && 180 - pfArcCos(v[2]) < 60)
        planeExplode(p);
}

void planeUpdate(Plane *p, float sx, float sy, int up, int down)
{
    pfVec3 acc, body, tv;
    pfMatrix rm, t;
    float dt = gDt, thrust, prev;
    vgHit ground;
    int ground_hit = 0;
    float gz = 0, gp = 0, gr = 0;

    /* controls */
    p->stick_y = sy;
    p->stick_x = sx;
    p->yaw = sx * 0.5f;
    if (up)
        p->throttle += 0.03f * gDt * 30;
    else if (down)
        p->throttle -= 0.03f * gDt * 30;
    if (p->throttle > 1) p->throttle = 1;
    if (p->throttle < 0) p->throttle = 0;
    if (p->state == 5)
        p->throttle = 0;
    vgPartPos(p->tailh, 0, 0, 0, 0, p->stick_y * 30, 0);
    vgPartPos(p->tailv, 0, 0, 0, p->yaw * 30, 0, 0);
    vgPartPos(p->lwing, 0, 0, 0, 0, p->stick_x * 30, 0);
    vgPartPos(p->rwing, 0, 0, 0, 0, p->stick_x * -30, 0);

    /* fuel */
    if (p->fuel > 0) {
        float use = p->throttle * p->t.invrange * gDt90;
        if (gDifficulty == 1)
            p->fuel -= use * 0.0025f;
        else if (gDifficulty == 2)
            p->fuel -= use * 0.005f;
        if (p->fuel <= 0) {
            p->fuel = 0;
            p->throttle = 0;
            vgDISAppearance(p->obj, 0x400000, 0);
        }
    } else {
        p->throttle = 0;
    }

    pos_mat(p->mat, p->pos);
    transpose(t, p->mat);

    /* acceleration */
    if (p->state == 2) {
        thrust = get_thrust(p);
        acc[0] = p->dir[0] * thrust;
        acc[1] = p->dir[1] * thrust;
        acc[2] = p->dir[2] * thrust - 9.8f;
    } else if (p->state == 5) {
        p->dive_t += 0.1f * gDt * 30;
        thrust = get_thrust(p);
        pfSetVec3(tv, 0, thrust + p->dive_t, 0);
        pfXformVec3(acc, tv, p->mat);
        acc[2] -= 9.8f;
    } else {
        thrust = get_thrust(p);
        pfSetVec3(tv, 0, thrust, 9.8f * p->speedfrac * p->lift);
        pfXformVec3(acc, tv, p->mat);
        acc[2] -= 9.8f;
    }
    if (p->state == 0 && p->fwdfrac < 0.3f)
        acc[2] = 0;
    p->vel[0] = acc[0] * dt;
    p->vel[1] = acc[1] * dt;
    p->vel[2] = acc[2] * dt;
    pfXformVec3(body, p->vel, t);
    if (p->throttle > 0.001f || p->speed > 0.001f) {
        pfCopyVec3(p->dir, p->vel);
        p->speed = pfNormalizeVec3(p->dir);
    } else {
        pfSetVec3(p->dir, 0, 0, 0);
        p->speed = 0;
    }
    /* the speed fraction changes by 0.007 at most per frame */
    p->speedfrac = p->speed * p->invvmax;
    prev = p->prev_speedfrac;
    if (p->speedfrac > prev + 0.007f * gDt * 30)
        p->speedfrac = prev + 0.007f * gDt * 30;
    else if (p->speedfrac < prev - 0.007f * gDt * 30)
        p->speedfrac = prev - 0.007f * gDt * 30;
    if (p->speedfrac > 1)
        p->speedfrac = 1;
    p->prev_speedfrac = p->speedfrac;
    p->fwdfrac = body[1] * p->invvmax;

    if (!p->dead) {
        p->pos[0] += p->vel[0] * dt + 0.5f * acc[0] * dt * dt;
        p->pos[1] += p->vel[1] * dt + 0.5f * acc[1] * dt * dt;
        p->pos[2] += p->vel[2] * dt + 0.5f * acc[2] * dt * dt;
    }
    if (p->pos[0] > 4400) p->pos[0] = 4400;
    if (p->pos[0] < -4400) p->pos[0] = -4400;
    if (p->pos[1] > 4400) p->pos[1] = 4400;
    if (p->pos[1] < -4400) p->pos[1] = -4400;
    if (p->pos[2] > 2000) p->pos[2] = 2000;

    /* line of sight: collisions once the engine has started */
    if (!p->engine_on && p->start_time + 10.0 < gNow)
        p->engine_on = 1;
    if (p->engine_on) {
        pfVec3 sp, fwd;
        vgHit h;
        pfMatrix m;
        float lp[6];
        memcpy(lp, p->pos, sizeof(lp));
        lp[2] += p->gear;
        pos_mat(m, lp);
        pfSetVec3(sp, lp[0], lp[1], lp[2]);
        pfSetVec3(fwd, m[1][0], m[1][1], m[1][2]);
        if (vgIsectSeg(gMainScene, ~CL_USER, sp, fwd, 10, &h))
            worldCollision(h.obj, h.cls);
    }

    /* ground below and shadow */
    if (p->pos[2] < 600) {
        if (!p->shadow_on) {
            p->shadow_on = 1;
            vgAddSceneObj(gMainScene, p->shadow);
        }
        ground_hit = vgIsectZ(gMainScene, CL_HOT, p->pos[0], p->pos[1], p->pos[3], &gz, &gp, &gr, &ground);
        if (!ground_hit) {
#ifdef CI_DEBUG
            {
                vgHit h2;
                float z2;
                int k;
                if (vgIsectZ(gMainScene, ~0u, p->pos[0], p->pos[1], 0, &z2, NULL, NULL, &h2)) {
                    fprintf(stderr, "no ground at %.0f %.0f: hit %s class %#x z %.1f path", p->pos[0], p->pos[1],
                            h2.obj->name, h2.cls, z2);
                    for (k = 0; k < h2.pathlen; k++)
                        fprintf(stderr, " %s", h2.path[k]->name ? h2.path[k]->name : "-");
                    fprintf(stderr, "\n");
                } else {
                    fprintf(stderr, "no ground at %.0f %.0f: nothing\n", p->pos[0], p->pos[1]);
                }
            }
#endif
            p->pos[2] = 0;
            gz = 0;
            gp = p->pos[4];
            gr = p->pos[5];
            p->ground = 0;
            memset(&ground, 0, sizeof(ground));
        } else {
            unsigned int c = hit_class(&ground);
            float sc;
            pfMatrix sm;
            int j;
            p->ground = gz;
            if (p->state == 0) {
                if (c == CL_WATER)
                    p->state = 3;
                else if (c != CL_GROUND)
                    p->state = 4;
            }
            /* the shadow is squeezed along x as the airplane banks */
            vgObjPos(p->shadow, p->pos[0], p->pos[1], gz + 0.1f, p->pos[3], gp, gr);
            sc = fabsf((float)((int)fabsf(p->pos[5]) % 180) - 90) / 90;
            pfCopyMat(sm, p->shadow->mat);
            for (j = 0; j < 3; j++)
                sm[0][j] *= sc;
            vgObjMat(p->shadow, sm);
            p->ground = gz + p->gear;
        }
    } else {
        if (p->shadow_on) {
            p->shadow_on = 0;
            vgRemSceneObj(gMainScene, p->shadow);
        }
        p->ground = 600;
        memset(&ground, 0, sizeof(ground));
    }

    switch (p->state) {
    case 1:
        if (p->speed < p->vstall) {
            /* stall: spin according to the bank, nose down */
            float a;
            p->state = 2;
            p->vel[2] = -4;
            a = axis_up_angle(p->mat, 0);
            p->stall_spin = (90 - a) / 90;
            if (axis_up_angle(p->mat, 2) > 90)
                p->stall_pitch = 1 - fabsf(p->stall_spin);
            else
                p->stall_pitch = -(1 - fabsf(p->stall_spin));
        } else if (p->pos[2] - p->ground < p->landh * gDt * 30) {
            update_landing(p, &ground);
            p->pos[2] = p->ground;
        } else {
            float roll = axis_up_angle(p->mat, 0) - 90;
            if (fabsf(roll) < 90) {
                p->yaw = roll * p->fwdfrac * 0.005f;
                p->pos[3] -= p->yaw * gDt90 * p->t.agility;
                p->pos[4] -= fabsf(roll / 90) * 0.2f * gDt * 30;
            }
            pos_mat(p->mat, p->pos);
            rot_mat(rm, gDt90 * (-p->stick_y * p->pitch_rate), 1, 0, 0);
            pfMultMat(p->mat, rm, p->mat);
            rot_mat(rm, gDt90 * (p->stick_x * p->roll_rate), 0, 1, 0);
            pfMultMat(p->mat, rm, p->mat);
            ciCopyMatHPR(p->mat, p->pos + 3);
        }
        break;
    case 2:
        if (p->pos[2] - p->ground < p->landh * gDt * 30) {
            update_landing(p, &ground);
            p->pos[2] = p->ground;
        } else {
            float tilt = axis_up_angle(p->mat, 2), roll = axis_up_angle(p->mat, 0) - 90;
            if (fabsf(tilt - 90) > 10 || fabsf(roll) > 10) {
                rot_mat(rm, gDt90 * (p->stall_spin * p->stall_s), 0, 0, 1);
                pfMultMat(p->mat, rm, p->mat);
                rot_mat(rm, gDt90 * (p->stall_pitch * p->stall_p) + (-p->stick_y * p->pitch_rate) * p->fwdfrac * gDt * 30, 1, 0, 0);
                pfMultMat(p->mat, rm, p->mat);
            } else {
                rot_mat(rm, (-p->stick_y * p->pitch_rate) * p->fwdfrac * gDt * 30, 1, 0, 0);
                pfMultMat(p->mat, rm, p->mat);
            }
            rot_mat(rm, p->stick_x * p->roll_rate * p->fwdfrac * gDt * 30, 0, 1, 0);
            pfMultMat(p->mat, rm, p->mat);
            ciCopyMatHPR(p->mat, p->pos + 3);
            /* recovered when the nose points down and the wings are level */
            if (axis_up_angle(p->mat, 1) > 135 && fabsf(roll) < 60)
                p->state = 1;
        }
        break;
    case 0:
        if (p->fwdfrac > 0.33f && p->pos[2] - p->ground > p->landh * gDt * 30) {
            p->state = 1;
        } else {
            p->pos[2] = p->ground;
            p->yaw = p->stick_x * p->gturn * p->fwdfrac;
            p->pos[3] -= p->yaw * gDt90 * 0.5f;
            if ((p->code & 1) && p->fwdfrac < 0.33f) {
                /* tail dragger: the tail lifts with the speed */
                p->pos[4] = gp + p->gnd_pitch * (0.33f - p->fwdfrac) * 3;
                p->pos[4] -= p->stick_y * p->pitch_rate * p->fwdfrac * gDt90;
            } else if (p->fwdfrac < 0.33f && (p->code >> 1)) {
                /* keeps its pitch */
            } else {
                p->pos[4] -= p->stick_y * p->pitch_rate * p->fwdfrac * gDt90;
                if (p->pos[4] < gp)
                    p->pos[4] = gp;
                else if (p->pos[4] > gp + 5)
                    p->pos[4] = gp + 5;
            }
            p->pos[5] = gr;
        }
        break;
    case 5:
        {
            float roll = axis_up_angle(p->mat, 0) - 90;
            if (roll > -90 && roll < 90) {
                p->pos[4] -= gDt90;
                if (p->pos[4] < -30)
                    p->pos[4] = -30;
            } else {
                p->pos[4] += gDt90;
                if (p->pos[4] > 30)
                    p->pos[4] = 30;
            }
            p->pos[5] += 1.5f * gDt90;
            if (p->pos[2] < p->dive_z && !p->screamed)
                p->screamed = 1;
            else if (p->pos[2] < gz)
                planeExplode(p);
        }
        break;
    default:
        break;
    }
    pos_mat(p->mat, p->pos);
    vgObjPosVec(p->obj, p->pos);
}

/* --- ballistic ammunition --------------------------------------------------------- */

AmmoBank *ammoBankNew(int n, float range, float accel, unsigned int mask, int tracers,
                      float r, float g, float b, vgObject *proto)
{
    AmmoBank *bank = (AmmoBank *)calloc(1, sizeof(AmmoBank));
    int i;
    bank->n = n;
    bank->ammo = (Ammo *)calloc(n, sizeof(Ammo));
    bank->range = range;
    bank->accel = accel;
    bank->mask = mask;
    bank->tracers = tracers;
    bank->tracer_color[0] = r;
    bank->tracer_color[1] = g;
    bank->tracer_color[2] = b;
    bank->proto = proto;
    if (proto)
        for (i = 0; i < n; i++)
            bank->ammo[i].obj = vgCopyObj(proto);
    return bank;
}

static void vec_hpr(const pfVec3 v, pfVec3 hpr)
{
    float l = sqrtf(v[0] * v[0] + v[1] * v[1]);
    hpr[0] = (float)-PF_RAD2DEG(atan2(v[0], v[1]));
    hpr[1] = (float)PF_RAD2DEG(atan2(v[2], l));
    hpr[2] = 0;
}

Ammo *ammoFire(AmmoBank *b, const pfVec3 pos, const pfVec3 vel, AmmoHitCb cb, void *data)
{
    int i;
    for (i = 0; i < b->n; i++) {
        Ammo *a = &b->ammo[i];
        if (a->active)
            continue;
        a->active = 1;
        pfCopyVec3(a->start, pos);
        pfCopyVec3(a->pos, pos);
        pfCopyVec3(a->vel, vel);
        vec_hpr(vel, a->hpr);
        a->seglen = -1;
        a->range2 = b->range * b->range;
        a->accel = b->accel;
        a->mask = b->mask;
        a->tracer = b->tracers;
        memcpy(a->tracer_color, b->tracer_color, sizeof(a->tracer_color));
        a->cb = cb;
        a->data = data;
        if (a->obj) {
            vgObjPos(a->obj, pos[0], pos[1], pos[2], a->hpr[0], a->hpr[1], 0);
            vgAddSceneObj(gMainScene, a->obj);
        }
        return a;
    }
    return NULL;
}

void ammoKill(Ammo *a)
{
    a->active = 0;
    if (a->obj)
        vgRemSceneObj(gMainScene, a->obj);
}

static void ammo_move(Ammo *a, float dt)
{
    pfVec3 acc, tv, old;
    pfMatrix m;
    float p6[6];
    p6[0] = p6[1] = p6[2] = 0;
    p6[3] = a->hpr[0]; p6[4] = a->hpr[1]; p6[5] = 0;
    pos_mat(m, p6);
    pfSetVec3(tv, 0, a->accel, 0);
    pfXformVec3(acc, tv, m);
    acc[2] -= 9.8f;
    pfCopyVec3(old, a->pos);
    a->vel[0] += acc[0] * dt;
    a->vel[1] += acc[1] * dt;
    a->vel[2] += acc[2] * dt;
    a->pos[0] += a->vel[0] * dt + 0.5f * acc[0] * dt * dt;
    a->pos[1] += a->vel[1] * dt + 0.5f * acc[1] * dt * dt;
    a->pos[2] += a->vel[2] * dt + 0.5f * acc[2] * dt * dt;
    vec_hpr(a->vel, a->hpr);
    if (a->seglen < 0)
        a->seglen = 5 * pfDistancePt3(old, a->pos);
    if (a->obj)
        vgObjPos(a->obj, a->pos[0], a->pos[1], a->pos[2], a->hpr[0], a->hpr[1], 0);
}

/* 0 when the ammunition is gone */
int ammoUpdate(Ammo *a, float dt)
{
    pfVec3 d, dir;
    vgHit h;
    if (!a->active)
        return 0;
    ammo_move(a, dt);
    pfSubVec3(d, a->pos, a->start);
    if (pfDotVec3(d, d) > a->range2 || a->pos[2] < 0) {
        ammoKill(a);
        return 0;
    }
    if (a->mask) {
        pfCopyVec3(dir, a->vel);
        if (pfNormalizeVec3(dir) > 0 && vgIsectSeg(gMainScene, a->mask, a->pos, dir, a->seglen, &h)) {
            if (!a->cb || !a->cb(a, &h, a->data)) {
                ammoKill(a);
                return 0;
            }
        }
    }
    return 1;
}

/* flak shells: no intersection, gone after a distance */
int ammoUpdateNoIsect(Ammo *a, float dt, float maxdist2)
{
    pfVec3 d;
    if (!a->active)
        return 0;
    ammo_move(a, dt);
    pfSubVec3(d, a->pos, a->start);
    if (pfDotVec3(d, d) > a->range2 || pfDotVec3(d, d) > maxdist2 || a->pos[2] < 0)
        return 0;
    return 1;
}

void ammoDraw(AmmoBank *b)
{
    int i;
    if (!b->tracers)
        return;
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(2);
    glBegin(GL_LINES);
    for (i = 0; i < b->n; i++) {
        Ammo *a = &b->ammo[i];
        pfVec3 d;
        if (!a->active)
            continue;
        pfCopyVec3(d, a->vel);
        pfNormalizeVec3(d);
        glColor3fv(a->tracer_color);
        glVertex3fv(a->pos);
        glVertex3f(a->pos[0] - 6 * d[0], a->pos[1] - 6 * d[1], a->pos[2] - 6 * d[2]);
    }
    glEnd();
    glLineWidth(1);
}

/* --- machine guns (vgMachGunPair) ----------------------------------------------- */

static struct {
    Plane *plane;
    int gun;
    pfNode *seq_gun[2], *sw_on[2], *flashes, *seq_flash[2];
    int trigger, prev;
    double trig_time, last_fire;
} guns;

static const float gun_offset[2][3] = { { 0.5f, 0.5f, 0 }, { -0.5f, 0.5f, 0 } };

static void guns_idle(void)
{
    int k;
    for (k = 0; k < 2; k++) {
        pfSwitchVal(guns.sw_on[k], PFSWITCH_ON);
        pfSeqMode(guns.seq_gun[k], PFSEQ_STOP);
        pfSeqMode(guns.seq_flash[k], PFSEQ_STOP);
    }
    pfSwitchVal(guns.flashes, PFSWITCH_OFF);
}

void gunsNew(Plane *p)
{
    vgObject *cp = vgFindObj("CockPit");
    memset(&guns, 0, sizeof(guns));
    guns.plane = p;
    guns.seq_gun[0] = vgFindPart(cp, "l_gun", 0);
    guns.seq_gun[1] = vgFindPart(cp, "r_gun", 0);
    guns.sw_on[0] = vgFindPart(cp, "l_gunOn", 0);
    guns.sw_on[1] = vgFindPart(cp, "r_gunOn", 0);
    guns.flashes = vgFindPart(cp, "flashes", 0);
    guns.seq_flash[0] = vgFindPart(cp, "l_flash", 0);
    guns.seq_flash[1] = vgFindPart(cp, "r_flash", 0);
    pfSeqDuration(guns.seq_gun[0], 0.75f, 1);
    pfSeqDuration(guns.seq_gun[1], 0.75f, 1);
    pfSeqDuration(guns.seq_flash[0], 1, 1);
    pfSeqDuration(guns.seq_flash[1], 1, 1);
    guns_idle();
    guns.last_fire = -1;
}

void gunsDelete(void)
{
    int i;
    for (i = 0; i < gGunBank->n; i++)
        ammoKill(&gGunBank->ammo[i]);
    guns.plane = NULL;
}

void gunsTrigger(void)
{
    guns.trigger = 1;
    guns.trig_time = gNow;
}

static int gun_hit_cb(Ammo *a, vgHit *h, void *data)
{
    (void)a; (void)data;
    worldDetonate(1, h->point, h);
    return 0;
}

static void guns_fire(void)
{
    Plane *p = guns.plane;
    pfVec3 pos, off, vel, mv;
    int g = guns.gun, o = !g;
    if (p->ammo == 0) {
        guns_idle();
        return;
    }
    p->ammo--;
    pfSwitchVal(guns.sw_on[o], PFSWITCH_ON);
    pfSeqMode(guns.seq_gun[o], PFSEQ_STOP);
    pfSeqMode(guns.seq_flash[o], PFSEQ_STOP);
    pfSwitchVal(guns.sw_on[g], PFSWITCH_OFF);
    pfSeqMode(guns.seq_gun[g], PFSEQ_START);
    pfSwitchVal(guns.flashes, PFSWITCH_ON);
    pfSeqMode(guns.seq_flash[g], PFSEQ_START);
    guns.gun = o;
    pfXformVec3(off, gun_offset[g], p->mat);
    pos[0] = p->pos[0] + off[0];
    pos[1] = p->pos[1] + off[1];
    pos[2] = p->pos[2] + off[2];
    pfSetVec3(mv, 0, 800, 0);
    pfXformVec3(vel, mv, p->mat);
    vel[0] += p->vel[0];
    vel[1] += p->vel[1];
    vel[2] += p->vel[2];
    ammoFire(gGunBank, pos, vel, gun_hit_cb, NULL);
}

void gunsUpdate(double t, float dt)
{
    int i;
    if (!guns.plane)
        return;
    for (i = 0; i < gGunBank->n; i++)
        if (gGunBank->ammo[i].active)
            ammoUpdate(&gGunBank->ammo[i], dt);
    if (guns.trigger != guns.prev) {
        if (!guns.trigger)
            guns_idle();
        guns.prev = guns.trigger;
    } else if (guns.trig_time + 0.21 < t) {
        guns.trigger = 0;
    }
    if (guns.trigger && guns.last_fire + 0.2 < t) {
        guns_fire();
        guns.last_fire = t;
    }
}

/* --- bombs (vgBombs) ------------------------------------------------------------ */

static struct {
    int trigger;
    double trig_time, last_fire;
} bombs;

void bombsTrigger(void)
{
    bombs.trigger = 1;
    bombs.trig_time = gNow;
}

static int bomb_hit_cb(Ammo *a, vgHit *h, void *data)
{
    (void)a; (void)data;
    worldDetonate(2, h->point, h);
    return 0;
}

void bombsUpdate(double t, float dt)
{
    Plane *p = gUserPlane;
    int i;
    for (i = 0; i < gBombBank->n; i++)
        if (gBombBank->ammo[i].active)
            ammoUpdate(&gBombBank->ammo[i], dt);
    if (!p)
        return;
    if (bombs.trigger && bombs.trig_time + 1.0 < t)
        bombs.trigger = 0;
    if (bombs.trigger && bombs.last_fire + 1.5 < t && p->bombs > 0) {
        pfVec3 pos;
        p->bombs--;
        pfSetVec3(pos, p->pos[0], p->pos[1], p->pos[2]);
        ammoFire(gBombBank, pos, p->vel, bomb_hit_cb, NULL);
        bombs.last_fire = t;
        bombs.trigger = 0;
    }
}
