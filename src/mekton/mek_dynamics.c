/*
 * mek_dynamics.c - movement of the mechs: input, walking, jump jets,
 * falling, the Ex-Centinel's plane mode, and collisions with the colony.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mekton.h"

pfMatrix mek_mat, old_mek_mat;

/* g_acc: gravity in units per tick per tick (set_tps) */
float g_acc;

/* plane mode (the Ex-Centinel transformed) */
static float s_wing, weight, b_span, ef, Fmax, Smax, pitch_rate, roll_rate;
static float Lmax, Lmin, gefy, ipi_AR, ie_pi_AR, lift, max_cl, min_cl;
static float tilt_factor, Splf, Spdf, mach, Cdp;
static int max_thrust_vector, min_thrust_vector, thrust_vector, flaps, spoilers;
static short g_limit, wing_stall;

/* corners of the collision box of each type */
static pfVec3 mek_box[2][8] = {
    { { -3, -3, -5.84f }, { -3, 3, -5.84f }, { 3, -3, -5.84f }, { 3, 3, -5.84f },
      { -3, -3, 7.2f }, { -3, 3, 7.2f }, { 3, -3, 7.2f }, { 3, 3, 7.2f } },
    { { -4, -3, -7.3f }, { -4, 3, -7.3f }, { 4, -3, -7.3f }, { 4, 6, -7.3f },
      { -4, -3, 4.9f }, { -4, 3, 4.9f }, { 4, -3, 4.9f }, { 4, 3, 4.9f } }
};

MekType mek_types[2] = {
    { 78.4f, 0.0078f, 200, 1.0f, { 0, 2.5f, 0 }, 7.84f, 7.2f, 30, -10, 30, -30, 90, 10,
      200, 100, 150, 150, 100, 80, 80, 90 },
    { 78.4f, 0.0078f, 200, 1.2f, { 0, 0, 4.0f }, 9.3f, 4.9f, 30, -10, 30, -30, 90, 10,
      200, 100, 150, 150, 100, 80, 80, 90 }
};

static void adjust_flaps(void);

/* --- setup -------------------------------------------------------------- */

static short walk_mode(const Mek *m)
{
    return (short)((m->status ? 0 : MODE_WALK | MODE_FOOT) | MODE_GROUND);
}

static short jump_mode(const Mek *m)
{
    return (short)((m->status ? 0 : MODE_AIR | MODE_FOOT) | MODE_JETS);
}

void set_mek_tps(Mek *m)
{
    const MekType *t = &mek_types[m->type];
    float f = m->tps / tps;
    m->vel[0] *= f;
    m->vel[1] *= f;
    m->vel[2] *= f;
    m->ang[0] *= f;
    m->ang[1] *= f;
    m->ang[2] *= f;
    m->t.max_fwd = t->max_fwd / tps;
    m->t.max_back = t->max_back / tps;
    m->t.max_acc = t->max_acc / tps;
    m->t.max_dec = t->max_dec / tps;
    m->t.turn_rate = t->turn_rate / tps;
    m->t.strafe = t->strafe / tps;
    m->t.jet_up = t->jet_up / tps;
    m->t.jet_down = t->jet_down / tps;
    m->t.jet_fwd = t->jet_fwd / tps;
    m->t.jet_back = t->jet_back / tps;
    m->t.jet_side = t->jet_side / tps;
    m->t.max_pitch = t->max_pitch / tps;
    m->t.max_roll = t->max_roll / tps;
    m->t.air_turn = t->air_turn / tps;
    m->tps = tps;
}

void init_mek(Mek *m, int type)
{
    if (type != m->type || !m->obj) {
        if (m->obj) {
            free_mek_objs(m);
            pfRemoveChild(isect_group_w_self, m->cg_obj->dcs);
        }
        m->type = type;
        find_mek_objs(m);
        m->travmask = pfGetNodeTravMask(m->cg_obj->model, PFTRAV_ISECT);
        pfSwitchVal(m->cg_obj->sw, PFSWITCH_OFF);
        pfAddChild(isect_group_w_self, m->cg_obj->dcs);
    }
    m->status = 0;
    switch_dead(m);
    m->flags = 0;
    m->hit_by = -1;
    m->f264 = m->f252 = m->f256 = m->f260 = 0;
    m->f276 = m->f280 = 1;
    m->team = (unsigned char)user_team;
    m->s124 = 4;
    memcpy(&m->t, &mek_types[type], sizeof(MekType));
    m->tps = tps;
    set_mek_tps(m);
    m->anim = 0;
    m->anim89 = 0;
    m->frame = 0;
    if (type == EX_CENTINEL) {
        m->flying = 0;
        m->obj = m->walk_obj;
        pfSwitchVal(m->trans_obj->sw, PFSWITCH_OFF);
        init_mek_fly(m);
    }
    m->can_fly = 1;
}

/* the ten starting places in the colony: position and orientation */
static const float start_places[10][6] = {
    { 256.789124f, 4592.1626f, 1649.48242f, 79.4701157f, -26.9657269f, 247.699997f },
    { -327.765015f, -928.056946f, -4798.80908f, 328.812653f, -9.39598942f, 354.399994f },
    { -300.231598f, -1208.79346f, -4701.24268f, 339.518127f, -13.5427179f, 355.0f },
    { -325.621063f, 1049.99451f, -4737.78467f, 216.759705f, -10.0283661f, 7.4000001f },
    { 317.315094f, -1197.26379f, -4703.14502f, 21.4583035f, -13.3314457f, 5.0999999f },
    { 343.17691f, 1199.85046f, -4700.80566f, 167.408386f, -13.9879122f, 356.899994f },
    { 139.019775f, -4221.11426f, -1814.66028f, 44.6475945f, -58.8568459f, 40.0999985f },
    { -245.989655f, -3766.14307f, 3150.47974f, 235.063324f, -34.3948326f, 219.0f },
    { 41.8166542f, -3415.94556f, 3513.573f, 61.555439f, 24.8476143f, 142.199997f },
    { -16.3807697f, 2851.33105f, 3636.48584f, 182.938065f, 38.0630493f, 178.300003f }
};

void mek_start_pos(Mek *m, int place)
{
    if (place == -1)
        place = rand();
    place %= 10;
    if (place >= 0) {
        m->mode = walk_mode(m);
        pfSetVec3(m->pos, start_places[place][0], start_places[place][1], start_places[place][2]);
        pfSetVec3(m->hpr, start_places[place][3], start_places[place][4], start_places[place][5]);
    }
    pfCopyVec3(m->old_pos, m->pos);
    pfCopyVec3(m->old_hpr, m->hpr);
    pfSetVec3(m->vel, 0, 0, 0);
    pfSetVec3(m->ang, 0, 0, 0);
    world2colony(m->pos, m->hpr, NULL, m->cpos, m->chpr, NULL);
    pfCopyVec3(m->cpos2, m->cpos);
    init_camera();
}

/* --- input -------------------------------------------------------------- */

static float val_to_input(int v, int dead, int span)
{
    float f;
    if (v > dead) {
        f = (float)(v - dead) / (float)span;
        return f < 1 ? f : 1;
    }
    if (v < -dead) {
        f = (float)(v + dead) / (float)span;
        return f > -1 ? f : -1;
    }
    return 0;
}

extern int center_x, center_y;

void mek_input(Mek *m)
{
    int dx = inp.x - center_x, dy = inp.y - center_y;

    if (m->mode & MODE_WALK) {
        if (inp.tab)
            return;
        m->in_fwd = val_to_input(dy, 40, 200);
        if (inp.shift) {
            m->in_side = val_to_input(dx, 40, 200);
            m->in_turn = inp.left ? -0.5f : inp.right ? 0.5f : 0;
        } else {
            m->in_turn = val_to_input(dx, 40, 200);
            m->in_side = inp.left ? -0.3f : inp.right ? 0.3f : 0;
        }
    } else if (m->mode & MODE_AIR) {
        if (inp.d)
            m->in_throttle += 0.01f;
        if (inp.a)
            m->in_throttle -= 0.01f;
        if (inp.s)
            m->in_throttle = 0;
        else if (m->in_throttle > 1)
            m->in_throttle = 1;
        else if (m->in_throttle < -1)
            m->in_throttle = -1;
        if (inp.tab)
            return;
        m->in_fwd = val_to_input(dy, 10, 200);
        m->in_side = val_to_input(dx, 10, 200);
        m->in_roll = m->in_side;
        m->in_pitch = -m->in_fwd;
        m->in_brake = inp.middle == 0;
        m->in_turn = inp.left ? -1.0f : inp.right ? 1.0f : 0;
    } else if (m->mode & MODE_FLY) {
        m->in_throttle = 0;
        if (inp.d)
            m->throttle += 0.05f;
        if (inp.a)
            m->throttle -= 0.05f;
        if (inp.s)
            m->throttle = 0;
        else if (m->f280 < m->throttle)
            m->throttle = m->f280;
        else if (m->throttle < m->f280 / -3.0f)
            m->throttle = m->f280 / -3.0f;
        if (inp.tab)
            return;
        m->in_pitch = -val_to_input(dy, 5, 400);
        m->in_roll = val_to_input(dx, 5, 400);
    }
}

/* F: light the jump jets */
void fkey_down(void)
{
    if ((my_mek->mode & MODE_WALK) && !shadow) {
        my_mek->mode = jump_mode(my_mek);
        my_mek->in_throttle = 0.01f;
    }
}

/* --- dynamics ----------------------------------------------------------- */

static float clampf(float v, float lo, float hi)
{
    if (hi < v) return hi;
    if (v < lo) return lo;
    return v;
}

static void mek_ground_dynamics(Mek *m)
{
    float f14 = tps / 5.0f + 1.0f, f10 = 1.0f + tps / 15.0f, acc, t;

    acc = (m->in_fwd * (0 <= m->in_fwd ? m->t.max_fwd : -m->t.max_back) - m->vel[1]) / f10;
    if (m->t.max_acc < acc)
        acc = m->t.max_acc;
    else if (acc < m->t.max_dec)
        acc = m->t.max_dec;
    m->vel[1] += acc;
    m->vel[0] += (m->t.strafe * m->in_side - m->vel[0]) / f14;
    m->ang[0] += (-m->t.turn_rate * m->in_turn - m->ang[0]) / f14;
    /* stay level with the colony */
    t = clampf(-m->chpr[1], -m->t.max_pitch, m->t.max_pitch);
    m->ang[1] += (t - m->ang[1]) / f14;
    t = -m->chpr[2];
    if (t < -180)
        t += 360;
    t = clampf(t, -m->t.max_roll, m->t.max_roll);
    m->ang[2] += (t - m->ang[2]) / f14;
}

/* up of the colony in the frame of the mek (column 2 of its rotation) */
static void colony_up(const Mek *m, pfMatrix r)
{
    pfMakeEulerMat(r, m->chpr[0], m->chpr[1], m->chpr[2]);
}

static void mek_fall_dynamics(Mek *m)
{
    pfMatrix r;
    pfVec3 v, d;
    float mi = g_acc / m->t.mass, speed, k;

    colony_up(m, r);
    pfScaleVec3(v, tps, m->vel);
    speed = pfSqrt(pfDotVec3(v, v));
    k = speed * m->t.drag;
    d[0] = -(v[0] * k) * mi - r[0][2] * g_acc;
    d[1] = -(v[1] * k) * mi - r[1][2] * g_acc;
    d[2] = -(v[2] * k) * mi - r[2][2] * g_acc;
    pfAddVec3(m->vel, m->vel, d);

    if (m->mode & MODE_GROUND) {
        /* on the ground and not walking: settle down */
        float damp = 1.0f - 1.0f / tps, rate = tps / 10.0f + 1.0f, p = m->chpr[2], t;
        m->ang[0] = 0;
        if (90 < p && p < 270)
            p = 0 < m->chpr[1] ? 180 - m->chpr[1] : -180 - m->chpr[1];
        else
            p = m->chpr[1];
        if (0 < p) {
            t = 1 < p ? p : 1;
            p = t < 90 - p ? t : 90 - p;
        } else {
            t = p < -1 ? p : -1;
            p = -90 - p < t ? t : -90 - p;
        }
        m->ang[1] += (p - m->ang[1]) / rate;
        m->ang[2] = 0;
        m->vel[2] = 0;
        m->ang[1] *= damp;
        m->vel[0] *= damp;
        m->vel[1] *= damp;
    }
}

static void mek_air_dynamics(Mek *m)
{
    pfMatrix r;
    pfVec3 v, drag, acc;
    float mi = g_acc / m->t.mass, speed, k, thrust, upv, rate, t, h;

    colony_up(m, r);
    pfScaleVec3(v, tps, m->vel);
    speed = pfSqrt(pfDotVec3(v, v));
    k = speed * m->t.drag;
    pfScaleVec3(drag, k, v);

    thrust = 0 < m->in_throttle ? m->t.jet_up : m->t.jet_down;
    upv = m->vel[0] * r[0][2] + m->vel[1] * r[1][2] + r[2][2] * m->vel[2];
    acc[2] = drag[2] + ((thrust * m->in_throttle - upv) + g_acc) / mi;
    if (acc[2] < -m->t.max_jet * 0.25f)
        acc[2] = -m->t.max_jet * 0.25f;
    else if (m->t.max_jet < acc[2])
        acc[2] = m->t.max_jet;

    t = 0 < m->in_fwd ? m->t.jet_fwd : m->t.jet_back;
    acc[1] = drag[1] + (t * m->in_fwd - m->vel[1]) / mi;
    if (acc[1] < -m->t.max_jet * 0.5f)
        acc[1] = -m->t.max_jet * 0.5f;
    else if (m->t.max_jet * 0.5f < acc[1])
        acc[1] = m->t.max_jet * 0.5f;

    acc[0] = drag[0] + (m->t.jet_side * m->in_side - m->vel[0]) / mi;
    if (acc[0] < -m->t.max_jet / 3.0f)
        acc[0] = -m->t.max_jet / 3.0f;
    else if (m->t.max_jet / 3.0f < acc[0])
        acc[0] = m->t.max_jet / 3.0f;

    m->vel[0] += (acc[0] - drag[0]) * mi - r[0][2] * g_acc;
    m->vel[1] += (acc[1] - drag[1]) * mi - r[1][2] * g_acc;
    m->vel[2] += (acc[2] - drag[2]) * mi - r[2][2] * g_acc;

    rate = tps / 5.0f + 1.0f;
    h = m->chpr[2] <= 180 ? m->chpr[2] : m->chpr[2] - 360;
    t = clampf((m->in_roll * 10.0f - h) * 0.25f, -m->t.max_roll, m->t.max_roll);
    m->ang[2] += (t - m->ang[2]) / rate;
    t = clampf((m->in_pitch * 10.0f - m->chpr[1]) * 0.25f, -m->t.max_pitch, m->t.max_pitch);
    m->ang[1] += (t - m->ang[1]) / rate;
    t = m->in_turn * -m->t.air_turn;
    if (m->in_brake)
        t += -3.0f * m->vel[0];
    m->ang[0] += (t - m->ang[0]) / rate;
    m->throttle = (m->throttle + acc[2] / m->t.max_jet) * 0.5f;
}

void init_mek_fly(Mek *m)
{
    (void)m;
    s_wing = 60;
    weight = 28000;
    max_thrust_vector = 0;
    min_thrust_vector = 0;
    b_span = 15;
    ef = 0.87f;
    Lmax = 15;
    Lmin = -15;
    Fmax = 30;
    Smax = 60;
    pitch_rate = 40;
    roll_rate = 150;
    Cdp = 0.015f;
    ipi_AR = 1.0f / ((3.14159274f * b_span * b_span) / s_wing);
    ie_pi_AR = ipi_AR / ef;
    roll_rate *= 0.01f;
    pitch_rate *= 0.01f;
    gefy = 0.7f * b_span;
    wing_stall = 0;
    thrust_vector = min_thrust_vector;
    lift = 0;
    mach = 0;
    spoilers = 0;
    flaps = 0;
    adjust_flaps();
}

static void adjust_flaps(void)
{
    max_cl = flaps / 62.5f + 1.5f;
    min_cl = flaps / 62.5f - 1.5f;
    tilt_factor = flaps * 0.005f + 0.017f;
}

static void mek_fly_dynamics(Mek *m)
{
    pfMatrix r;
    float gm = 9.75361919f / m->t.mass, kl, cl, alpha, cd_mach, sound, q, drag, f14, s, c;
    float fz, fy, k, rate, thrust;

    colony_up(m, r);
    /* ground effect */
    if (gefy < m->cpos[2]) {
        kl = 1;
    } else {
        float h = m->cpos[2] / b_span;
        kl = 0.225f < h ? 0.484f * h + 0.661f : 2.533f * h + 0.2f;
    }
    if (0 <= m->vel[1]) {
        alpha = 0;
        cl = 0;
        wing_stall = (short)(-m->vel[1]);
    } else {
        alpha = (m->vel[2] / -m->vel[1] + tilt_factor) / (kl * ipi_AR + 0.17f);
        cl = alpha;
        if (max_cl < alpha) {
            wing_stall = 1;
            cl = 3.0f * max_cl - (alpha + alpha);
            if (cl < 0) {
                wing_stall = (short)(wing_stall - (int)(cl / max_cl) + 1);
                cl = 0;
            }
            if (5.0f < alpha)
                alpha = 5.0f;
        } else if (alpha < min_cl) {
            wing_stall = 1;
            cl = 3.0f * min_cl - (alpha + alpha);
            if (0 < cl) {
                wing_stall = (short)(wing_stall - (int)(cl / min_cl) + 1);
                cl = 0;
            }
            if (alpha < -5.0f)
                alpha = -5.0f;
        } else {
            wing_stall = 0;
        }
    }
    if (64 < wing_stall)
        wing_stall = 64;
    sound = 340.16f / tps;
    cd_mach = cl < 0.2f ? 0.1666667f * cl + 0.7166666f : 0.7833333f - 0.1666667f * cl;
    mach = m->vel[1] / sound;
    if (mach / cd_mach < 1.034f) {
        cd_mach = 0;
    } else {
        cd_mach = 0.082f * (mach / cd_mach) - 0.084788f;
        if (0.03f < cd_mach)
            cd_mach = 0.03f;
    }
    if (0 < spoilers)
        cd_mach += Spdf;
    q = -m->vel[1] * 0.00115405f * s_wing;
    g_limit = 0;
    lift = cl * q;
    if (0 < spoilers)
        lift *= Splf;
    for (;;) {
        float l = -m->vel[1] * lift * gm;
        if (Lmax * g_acc < l) {
            g_limit = 1;
            lift = ((0.99f * Lmax * g_acc) / gm) / -m->vel[1];
        } else if (l < Lmin * g_acc) {
            g_limit = 1;
            lift = ((0.99f * Lmin * g_acc) / gm) / -m->vel[1];
        } else {
            break;
        }
    }
    fz = -m->vel[1] * lift;
    fy = -m->vel[2] * lift;
    pfSinCos((float)thrust_vector, &s, &c);
    thrust = mek_types[m->type].max_jet * 1.5f * m->throttle / tps / tps;
    fy += c * thrust;
    fz += s * thrust;
    drag = ((kl * alpha * alpha * ie_pi_AR) + Cdp + cd_mach) * q;
    k = -m->vel[1] * drag;
    if (0 <= m->vel[1]) {
        fy -= k;
        fz -= m->vel[2] * drag;
    } else {
        fy += k;
        fz += m->vel[2] * drag;
    }
    fy *= gm;
    fz *= gm;
    m->vel[0] += 0 - r[0][2] * g_acc;
    m->vel[1] += fy - r[1][2] * g_acc;
    m->vel[2] += fz - r[2][2] * g_acc;
    f14 = m->vel[1] < 5.0f ? m->vel[1] : 5.0f;
    rate = tps / 5.0f + 1.0f;
    m->ang[2] += (roll_rate * m->in_roll * f14 - m->ang[2]) / rate;
    m->ang[1] += (pitch_rate * m->in_pitch * f14 + m->vel[2] * 0.05f - m->ang[1]) / rate;
    m->ang[0] += (-2.0f * m->vel[0] - m->ang[0]) / rate;
    (void)Fmax; (void)Smax; (void)weight;
}

/* --- position ----------------------------------------------------------- */

static void update_position(Mek *m)
{
    pfMatrix r;
    pfVec3 v;
    int k;

    for (k = 0; k < 3; k++)
        v[k] = clampf(m->vel[k], m->vmin[k], m->vmax[k]);
    pfCopyVec3(m->old_pos, m->pos);
    pfCopyVec3(m->old_hpr, m->hpr);
    matrix_translate(mek_mat, v[0], v[1], v[2]);
    if (m->ang[0] != 0)
        matrix_rotate(mek_mat, m->ang[0], 'z');
    if (m->ang[1] != 0)
        matrix_rotate(mek_mat, m->ang[1], 'x');
    if (m->ang[2] != 0)
        matrix_rotate(mek_mat, m->ang[2], 'y');
    mat2hpr(mek_mat, m->hpr);
    m->pos[0] = mek_mat[3][0];
    m->pos[1] = mek_mat[3][1];
    m->pos[2] = mek_mat[3][2];
    if (m->mode & MODE_WALK)
        return;
    /* keep the momentum: turn the velocity into the new frame */
    identify_matrix(r);
    if (m->ang[2] != 0)
        matrix_rotate(r, -m->ang[2], 'y');
    if (m->ang[1] != 0)
        matrix_rotate(r, -m->ang[1], 'x');
    if (m->ang[0] != 0)
        matrix_rotate(r, -m->ang[0], 'z');
    matrix_translate(r, m->vel[0], m->vel[1], m->vel[2]);
    m->vel[0] = r[3][0];
    m->vel[1] = r[3][1];
    m->vel[2] = r[3][2];
}

static int push_away(pfVec3 pos, pfVec3 *box, pfMatrix mat)
{
    pfSegSet ss;
    pfHit **hits[PFIS_MAX_SEGS];
    pfVec3 pt[8], nm[8], p;
    float dist[8], ratio[8];
    int nplanes = 0, hit = 0, k, j, flags;

    for (k = 0; k < 8; k++) {
        pfXformPt3(p, box[k], mat);
        pfMakePtsSeg(&ss.segs[k], pos, p);
        ss.segs[k].length += ss.segs[k].length;
    }
    ss.isectMask = 0xffffffffU;
    ss.mode = PFTRAV_IS_PRIM | PFTRAV_IS_NORM | PFTRAV_IS_CULL_BACK;
    ss.userData = NULL;
    ss.activeMask = 0xff;
    ss.discFunc = NULL;
    ss.bound = NULL;
    if (!pfNodeIsectSegs(isect_group, &ss, hits))
        return 0;
    for (k = 0; k < 8; k++) {
        float r, dp;
        int unique = 1;
        pfQueryHit(*hits[k], PFQHIT_FLAGS, &flags);
        if (!flags)
            continue;
        pfQueryHit(*hits[k], PFQHIT_POINT, pt[nplanes]);
        pfQueryHit(*hits[k], PFQHIT_NORM, nm[nplanes]);
        r = pfDistancePt3(ss.segs[k].pos, pt[nplanes]) / (ss.segs[k].length * 0.5f);
        if (r < 1)
            hit = 1;
        dp = dist_to_plane(pos, nm[nplanes], pt[nplanes]);
        dist[nplanes] = dp;
        ratio[nplanes] = dp / r;
        /* one entry per plane, the closest corner wins */
        for (j = 0; j < nplanes && unique; j++) {
            if (nm[nplanes][0] == nm[j][0] && nm[nplanes][1] == nm[j][1] && nm[nplanes][2] == nm[j][2]) {
                unique = 0;
                if (dp < dist[j]) {
                    ratio[j] = ratio[nplanes];
                    dist[j] = dp;
                    pfCopyVec3(pt[j], pt[nplanes]);
                }
            }
        }
        if (unique)
            nplanes++;
    }
    if (!hit)
        return 0;
    if (nplanes == 1) {
        float d = ratio[0] - dist[0];
        pos[0] += nm[0][0] * d;
        pos[1] += nm[0][1] * d;
        pos[2] += nm[0][2] * d;
    } else if (nplanes == 2) {
        float dp, d;
        if (dist[0] < ratio[0]) {
            d = ratio[0] - dist[0];
            pos[0] += nm[0][0] * d;
            pos[1] += nm[0][1] * d;
            pos[2] += nm[0][2] * d;
        }
        if (0 <= pfDotVec3(nm[1], nm[0])) {
            dp = dist_to_plane(pos, nm[1], pt[1]);
            if (dp < ratio[1]) {
                d = ratio[1] - dp;
                pos[0] += nm[1][0] * d;
                pos[1] += nm[1][1] * d;
                pos[2] += nm[1][2] * d;
            }
        } else {
            pfVec3 c, dir, q, isect;
            float len;
            dp = sdist_to_plane(pos, nm[1], pt[1]);
            if (!(dp < ratio[1]))
                return hit;
            /* slide along the crease between the two planes */
            pfCrossVec3(c, nm[0], nm[1]);
            pfCrossVec3(dir, c, nm[0]);
            pfNormalizeVec3(dir);
            if (pfDotVec3(dir, nm[1]) < 0)
                pfScaleVec3(dir, -1, dir);
            q[0] = dir[0] * -100.0f + pos[0];
            q[1] = dir[1] * -100.0f + pos[1];
            q[2] = dir[2] * -100.0f + pos[2];
            line_isect_plane(pos, q, nm[1], pt[1], isect);
            len = pfDistancePt3(pos, isect);
            if (dp < 0)
                len = -len;
            d = (ratio[1] - dp) * (len / dp);
            pos[0] += dir[0] * d;
            pos[1] += dir[1] * d;
            pos[2] += dir[2] * d;
        }
    } else {
        int pass, moved;
        for (pass = 0; pass < 3; pass++) {
            moved = 0;
            for (j = 0; j < nplanes; j++) {
                float dp = sdist_to_plane(pos, nm[j], pt[j]);
                if (dp < ratio[j]) {
                    float d = ratio[j] - dp;
                    pos[0] += nm[j][0] * d;
                    pos[1] += nm[j][1] * d;
                    pos[2] += nm[j][2] * d;
                    moved = 1;
                }
            }
            if (!moved)
                break;
        }
    }
    return hit;
}

static void test_position(Mek *m)
{
    pfSegSet ss;
    pfHit **hits[PFIS_MAX_SEGS];
    pfVec3 p[9], a, b, wnorm, up, p0, n0, p1, n1;
    float best_r = 0, best_d = 0, d0 = 0, d1 = 0;
    int lost_ground = 0, hit = 0, prev_wall = m->hit_wall, best = -1, k, f0, f1;
    int flags;

    m->hit_wall = 0;
    m->i268 = 0;
    pfSetVec3(wnorm, 0, 0, 1);

    /* segments from the old to the new position of the center and the
     * corners of the collision box */
    pfMakePtsSeg(&ss.segs[0], m->old_pos, m->pos);
    ss.segs[0].length += 3.1f;
    for (k = 0; k < 8; k++) {
        pfXformPt3(a, mek_box[m->type][k], old_mek_mat);
        pfXformPt3(b, mek_box[m->type][k], mek_mat);
        pfMakePtsSeg(&ss.segs[k + 1], a, b);
        ss.segs[k + 1].length += 0.2f;
    }
    ss.mode = PFTRAV_IS_PRIM | PFTRAV_IS_NORM | PFTRAV_IS_CULL_BACK;
    ss.userData = NULL;
    ss.activeMask = 0x1ff;
    ss.isectMask = 0xffffffffU;
    ss.discFunc = NULL;
    ss.bound = NULL;
    if (pfNodeIsectSegs(isect_group, &ss, hits)) {
        hit = 1;
        for (k = 0; k < 9; k++) {
            float d, r;
            pfQueryHit(*hits[k], PFQHIT_FLAGS, &flags);
            if (!flags)
                continue;
            pfQueryHit(*hits[k], PFQHIT_POINT, p[k]);
            d = pfDistancePt3(ss.segs[k].pos, p[k]);
            r = d / ss.segs[k].length;
            if (best == -1 || r < best_r) {
                best = k;
                best_r = r;
                best_d = d;
            }
        }
        pfQueryHit(*hits[best], PFQHIT_NORM, wnorm);
        if (best_d <= 0.2f) {
            pfCopyVec3(m->pos, m->old_pos);
        } else {
            float f = (best_d - 0.2f) / (ss.segs[best].length - 0.2f);
            for (k = 0; k < 3; k++)
                m->pos[k] = (m->pos[k] - m->old_pos[k]) * f + m->old_pos[k];
        }
        mek_mat[3][0] = m->pos[0];
        mek_mat[3][1] = m->pos[1];
        mek_mat[3][2] = m->pos[2];
    }
    if (hit || prev_wall) {
        hit |= push_away(m->pos, mek_box[m->type], mek_mat);
        mek_mat[3][0] = m->pos[0];
        mek_mat[3][1] = m->pos[1];
        mek_mat[3][2] = m->pos[2];
    }

    /* probes down to the ground and up to the ceiling */
    pfCopyVec3(ss.segs[0].pos, m->pos);
    pfSetVec3(ss.segs[0].dir, 0, 0, -1);
    pfXformVec3(ss.segs[0].dir, ss.segs[0].dir, mek_mat);
    ss.segs[0].length = m->t.height + 30.0f;
    pfCopyVec3(ss.segs[1].pos, m->pos);
    pfScaleVec3(ss.segs[1].dir, -1, ss.segs[0].dir);
    ss.segs[1].length = m->t.radius + 10.0f;
    ss.activeMask = 3;
    if (!pfNodeIsectSegs(isect_group, &ss, hits)) {
        if (m->mode & MODE_GROUND)
            lost_ground = 1;
        goto done;
    }
    pfQueryHit(*hits[1], PFQHIT_FLAGS, &f1);
    if (f1) {
        pfQueryHit(*hits[1], PFQHIT_POINT, p1);
        pfQueryHit(*hits[1], PFQHIT_NORM, n1);
        d1 = range(p1, m->pos);
    }
    pfQueryHit(*hits[0], PFQHIT_FLAGS, &f0);
    if (!f0) {
        if (m->mode & MODE_GROUND)
            lost_ground = 1;
        if (f1 && d1 < m->t.radius) {
            hit = 1;
            pfCopyVec3(wnorm, n1);
            for (k = 0; k < 3; k++)
                m->pos[k] = p1[k] - ss.segs[1].dir[k] * m->t.radius;
        }
        goto done;
    }
    pfQueryHit(*hits[0], PFQHIT_POINT, p0);
    pfQueryHit(*hits[0], PFQHIT_NORM, n0);
    /* up points to the axis of the colony */
    up[0] = 0;
    up[1] = -m->pos[1];
    up[2] = -m->pos[2];
    pfNormalizeVec2(up + 1);
    d0 = range(p0, m->pos);
    if ((m->mode & MODE_GROUND) || d0 <= m->t.height) {
        if (m->t.height + 2.0f < d0) {
            lost_ground = 1;
            goto done;
        }
        if (f1 && d1 < m->t.radius) {
            float push = m->t.radius - d1;
            hit = 1;
            pfCopyVec3(wnorm, n1);
            if (d0 - 0.5f < push)
                push = d0 - 0.5f;
            for (k = 0; k < 3; k++)
                m->pos[k] -= ss.segs[1].dir[k] * push;
        } else if ((m->mode & MODE_FOOT) || d0 < m->t.height) {
            for (k = 0; k < 3; k++)
                m->pos[k] = p0[k] + ss.segs[0].dir[k] * -m->t.height;
        }
        if (0.707f < pfDotVec3(n0, up)) {
            if (m->mode & MODE_JETS) {
                /* landing */
                hit = 0;
                m->mode = walk_mode(m);
                m->throttle = 0;
                m->vel[2] = 0;
                both_feet_sound(m);
            }
        } else {
            hit = 1;
            pfCopyVec3(wnorm, n0);
        }
    } else if (d0 < (m->cpos2[2] - m->cpos[2]) * tps + m->t.height) {
        if (0.707f < pfDotVec3(n0, up))
            m->i268 = 1;
    }

done:
    if (lost_ground && (m->mode & MODE_FOOT)) {
        m->mode = jump_mode(m);
        m->in_throttle = 0.01f;
    }
    if (hit) {
        pfVec3 v, rv;
        pfMatrix inv;
        float f;
        m->hit_wall = 1;
        pfXformVec3(v, m->vel, mek_mat);
        pfScaleVec3(v, -1, v);
        reflect(v, wnorm, rv);
        pfNormalizeVec3(v);
        f = 1.0f - pfDotVec3(wnorm, v);
        pfScaleVec3(rv, f, rv);
        make_inv_euler_mat(m->hpr, inv);
        pfXformVec3(m->vel, rv, inv);
    }
    if (m->pos[0] != m->pos[0] || m->pos[1] != m->pos[1] || m->pos[2] != m->pos[2] ||
        m->hpr[0] != m->hpr[0] || m->hpr[2] != m->hpr[2] || m->hpr[1] != m->hpr[1]) {
        fprintf(stderr, "Fixed NAN pos 2\n");
        pfCopyVec3(m->pos, m->old_pos);
        pfCopyVec3(m->hpr, m->old_hpr);
    }
    if (m->hit_wall == 1 && !prev_wall)
        clank_sound(m->pos);
    if (m->type == EX_CENTINEL && m->flying && (m->hit_wall || (m->mode & MODE_WALK))) {
        m->flying = 0;
        if (m->mode & MODE_FLY) {
            m->mode = jump_mode(m);
            m->in_throttle = 0;
        }
    }
}

void mek_dynamics(Mek *m)
{
    m->s124 = 4;
    if (tps != m->tps)
        set_mek_tps(m);
    pfMakeEulerMat(mek_mat, m->hpr[0], m->hpr[1], m->hpr[2]);
    mek_mat[3][0] = m->pos[0];
    mek_mat[3][1] = m->pos[1];
    mek_mat[3][2] = m->pos[2];
    pfCopyMat(old_mek_mat, mek_mat);
    if (cd_app->state == STATE_INTRO)
        return;
    if (m->mode & MODE_AIR)
        mek_air_dynamics(m);
    else if (m->mode & MODE_WALK)
        mek_ground_dynamics(m);
    else if (m->mode & MODE_FLY)
        mek_fly_dynamics(m);
    else if (!(m->mode & MODE_FOOT))
        mek_fall_dynamics(m);
    animate_mek(m);
    update_position(m);
    test_position(m);
    pfCopyVec3(m->cpos2, m->cpos);
    world2colony(m->pos, m->hpr, NULL, m->cpos, m->chpr, NULL);
}
