/*
 * mek_anim.c - walking animation of the mechs.
 *
 * The keyframes of the models are indexed by the distance walked: each
 * tick the distance covered by the mech advances the animation through a
 * small state machine (stand, start walking, walk, run, stop, jump, land,
 * walk backwards). The state machine also limits the speed while a
 * transition is played.
 */
#include <math.h>

#include "mekton.h"

/* Vantage states and their frame ranges */
enum {
    V_STAND,        /* 0 */
    V_START,        /* 0..4 */
    V_WALK,         /* 4..24 */
    V_RUN_START,    /* 18..36 */
    V_RUN,          /* 36..44 */
    V_RUN_STOP,     /* 45..51 */
    V_WALK_STOP,    /* 25..29 */
    V_JUMP,         /* 52..61 */
    V_LAND,         /* 61..52 */
    V_BACK_START,   /* 29..25 */
    V_BACK,         /* 23..4 */
    V_BACK_STOP     /* 4..0 */
};

/* what the mech is asked to do */
enum { GO_BACK, GO_STAND, GO_WALK, GO_RUN, GO_AIR };

static float clampv(float v, float lo, float hi)
{
    if (hi < v) return hi;
    if (v < lo) return lo;
    return v;
}

/* moves frame forward by d / step, stopping at limit; returns the part of
 * d left once the limit is reached, or 0 */
static float advance(Mek *m, float *d, float step, float limit)
{
    float f = *d / step;
    if (limit <= m->frame + f) {
        float left = *d * ((m->frame + f - limit) / f);
        *d = left;
        return 1;
    }
    m->frame += f;
    *d = 0;
    return 0;
}

static float back_up(Mek *m, float *d, float step, float limit)
{
    float f = *d / step;
    if (m->frame - f <= limit) {
        *d = *d * (-((m->frame - f) - limit) / f);
        return 1;
    }
    m->frame -= f;
    *d = 0;
    return 0;
}

static void animate_vantage(Mek *m)
{
    float speed = m->vel[1] * tps, d, f;
    int go;

    if (m->vmax[1] < m->vel[1])
        d = m->vmax[1];
    else
        d = m->vel[1] < m->vmin[1] ? m->vmin[1] : m->vel[1];
    if (m->t.max_fwd < d)
        d = m->t.max_fwd;
    else if (d < 0)
        d = -d;
    else if (d == 0)
        d = 1e-6f;

    pfSetVec3(m->vmin, -100000, -100000, -100000);
    pfSetVec3(m->vmax, 100000, 100000, 100000);
    if (!(m->mode & MODE_FOOT))
        return;

    if ((m->mode & MODE_AIR) && !m->i268)
        go = GO_AIR;
    else if (16 <= speed)
        go = GO_RUN;
    else if (0.1f <= speed)
        go = GO_WALK;
    else if (-0.1f <= speed)
        go = GO_STAND;
    else
        go = GO_BACK;

    while (0 < d) {
        switch (m->anim) {
        case V_STAND:
            m->frame = 0;
            switch (go) {
            case GO_BACK: m->anim = V_BACK_START; m->frame = 29; break;
            case GO_STAND: d = 0; break;
            case GO_WALK: case GO_RUN: m->anim = V_START; break;
            case GO_AIR: m->anim = V_JUMP; m->frame = 52; break;
            }
            break;
        case V_START:
            if (advance(m, &d, 1.2f, 4)) {
                m->anim = V_WALK;
                m->frame = 4;
            }
            break;
        case V_WALK:
            f = d / 1.2f;
            switch (go) {
            case GO_BACK:
                m->anim = V_BACK;
                break;
            case GO_STAND: case GO_AIR:
                if (m->frame <= 23 && 23 <= m->frame + f) {
                    d *= (m->frame + f - 23) / f;
                    m->anim = V_WALK_STOP;
                    m->frame = 25;
                } else {
                    m->frame += f;
                    d = 0;
                }
                break;
            case GO_WALK:
                if (advance(m, &d, 1.2f, 24))
                    m->frame = 4;
                break;
            case GO_RUN:
                if (m->frame <= 18 && 18 <= m->frame + f) {
                    d *= (m->frame + f - 18) / f;
                    m->anim = V_RUN_START;
                    m->frame = 30;
                } else if (advance(m, &d, 1.2f, 24)) {
                    m->frame = 4;
                }
                break;
            }
            break;
        case V_RUN_START:
            if (advance(m, &d, 1.2f, 36)) {
                m->anim = V_RUN;
                m->frame = 36;
            }
            break;
        case V_RUN:
            f = d / 3.8f;
            if (go == GO_RUN) {
                if (advance(m, &d, 3.8f, 44))
                    m->frame = 36;
            } else if (40 < m->frame && 44 <= m->frame + f) {
                d *= (m->frame + f - 44) / f;
                m->frame = 36;
            } else if (m->frame <= 40 && 40 <= m->frame + f) {
                d *= (m->frame + f - 40) / f;
                m->anim = V_RUN_STOP;
                m->frame = 45;
            } else {
                m->frame += f;
                d = 0;
            }
            break;
        case V_RUN_STOP:
            if (advance(m, &d, 1.2f, 51)) {
                m->anim = V_WALK;
                m->frame = 10;
            }
            break;
        case V_WALK_STOP:
            f = d / 1.2f;
            if (29 <= m->frame + f) {
                m->anim = V_STAND;
                m->frame = 0;
            } else {
                m->frame += f;
            }
            d = 0;
            break;
        case V_JUMP:
            if (go != GO_AIR) {
                m->anim = V_LAND;
            } else {
                m->frame += 30.0f / tps;
                if (61 < m->frame)
                    m->frame = 61;
                d = 0;
            }
            break;
        case V_LAND:
            f = 30.0f / tps;
            if (go == GO_AIR) {
                m->anim = V_JUMP;
            } else if (m->frame - f <= 52) {
                m->anim = V_STAND;
                m->frame = 0;
                d *= (f + 52) / f;
            } else {
                m->frame -= f;
                d = 0;
            }
            break;
        case V_BACK_START:
            if (back_up(m, &d, 1.2f, 25)) {
                m->anim = V_BACK;
                m->frame = 23;
            }
            break;
        case V_BACK:
            switch (go) {
            case GO_BACK:
                if (back_up(m, &d, 1.2f, 4))
                    m->frame = 24;
                break;
            case GO_STAND: case GO_AIR:
                if (back_up(m, &d, 1.2f, 4)) {
                    m->anim = V_BACK_STOP;
                    m->frame = 4;
                }
                break;
            default:
                m->anim = V_WALK;
                break;
            }
            break;
        case V_BACK_STOP:
            f = d / 1.2f;
            if (m->frame <= f) {
                m->anim = V_STAND;
                m->frame = 0;
                left_foot_step_sound(m);
            } else {
                m->frame -= f;
            }
            d = 0;
            break;
        default:
            m->anim = V_STAND;
            d = 0;
            break;
        }
    }

    /* the transitions limit the speed */
    switch (m->anim) {
    case V_START: case V_WALK: case V_WALK_STOP:
        m->vmin[1] = 5.0f / tps;
        if (m->mode & MODE_WALK) {
            m->vmax[1] = 21.0f / tps;
            if (m->vmax[1] < m->vel[1])
                m->vel[1] = m->vmax[1];
        }
        break;
    case V_RUN_START: case V_RUN:
        m->vmin[1] = 15.0f / tps;
        if (m->vel[1] < m->vmin[1])
            m->vel[1] = m->vmin[1];
        break;
    case V_RUN_STOP:
        m->vmin[1] = 13.0f / tps;
        if (m->vel[1] < m->vmin[1])
            m->vel[1] = m->vmin[1];
        break;
    case V_BACK_START: case V_BACK: case V_BACK_STOP:
        m->vmax[1] = -5.0f / tps;
        break;
    }
    (void)clampv;
}

void animate_ex_centinel(Mek *m);

void animate_mek(Mek *m)
{
    if (m->type == VANTAGE)
        animate_vantage(m);
    else if (m->type == EX_CENTINEL)
        animate_ex_centinel(m);
}

/* shows the right model of each mech and poses its joints */
void animation_control(void)
{
    int i;
    for (i = 0; i < number_mektons; i++) {
        Mek *m = mektons[i];
        if (m->type == EX_CENTINEL && m != my_mek && !shadow) {
            Obj *show, *hide;
            if (m->anim == 19 || m->anim == 17) {
                show = m->trans_obj;
                hide = m->walk_obj;
            } else {
                show = m->walk_obj;
                hide = m->trans_obj;
            }
            m->obj = show;
            if (m != my_mek || camera_mode)
                pfSwitchVal(show->sw, PFSWITCH_ON);
            pfSwitchVal(hide->sw, PFSWITCH_OFF);
        }
        if (m->type == VANTAGE || m->type == EX_CENTINEL)
            da_frame(m->obj->dcs, m->frame);
    }
}
