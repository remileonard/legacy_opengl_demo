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


/* Ex-Centinel states and their frame ranges */
enum {
    X_STAND,        /* 0 */
    X_START,        /* 0..4 */
    X_START2,       /* ..74 */
    X_WALK,         /* 4..24 */
    X_STOP_R,       /* 24..28 */
    X_STOP_L,       /* 29..33 */
    X_RUN_START_R,  /* 34..38 */
    X_RUN_START_L,  /* 39..43 */
    X_RUN,          /* 43..53 */
    X_RUN_STOP_R,   /* 53..58 */
    X_RUN_STOP_L,   /* 59..64 */
    X_BACK_START,   /* 28..24 */
    X_BACK_START2,  /* 33..29 */
    X_BACK,         /* 24..4 */
    X_BACK_STOP,    /* 4..0 */
    X_BACK_STOP2,   /* 74..70 */
    X_JUMP,         /* 65..68 */
    X_AIR,          /* transformation model, frame 89 */
    X_LAND,         /* 68..65 */
    X_TRANSFORM     /* 89..0: walker to plane, and back */
};

static void show_trans(Mek *m, int trans)
{
    Obj *show = trans ? m->trans_obj : m->walk_obj, *hide = trans ? m->walk_obj : m->trans_obj;
    m->obj = show;
    pfSwitchVal(hide->sw, PFSWITCH_OFF);
    if (camera_mode)
        pfSwitchVal(show->sw, PFSWITCH_ON);
}

void animate_ex_centinel(Mek *m)
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

    if ((m->mode & MODE_FLY) || ((m->mode & MODE_AIR) && !m->i268))
        go = GO_AIR;
    else if (20 <= speed)
        go = GO_RUN;
    else if (0.1f <= speed)
        go = GO_WALK;
    else if (-0.1f <= speed)
        go = GO_STAND;
    else
        go = GO_BACK;

    while (0 < d) {
        switch (m->anim) {
        case X_STAND:
            switch (go) {
            case GO_BACK: m->anim = X_BACK_START; m->frame = 28; break;
            case GO_STAND: m->frame = 0; d = 0; break;
            case GO_WALK: case GO_RUN: m->anim = X_START; m->frame = 0; break;
            case GO_AIR: m->anim = X_JUMP; m->frame = 65; break;
            }
            break;
        case X_START:
            if (advance(m, &d, 1, 4)) {
                m->anim = X_WALK;
                m->frame = 4;
            }
            break;
        case X_START2:
            if (advance(m, &d, 1, 74)) {
                m->anim = X_WALK;
                m->frame = 14;
            }
            break;
        case X_WALK:
            f = d;
            switch (go) {
            case GO_BACK:
                m->anim = X_BACK;
                break;
            case GO_STAND: case GO_AIR:
                if (24 <= m->frame + f) {
                    d *= (m->frame + f - 24) / f;
                    m->anim = X_STOP_R;
                    m->frame = 24;
                } else if (m->frame < 14 && 14 <= m->frame + f) {
                    d *= (m->frame + f - 14) / f;
                    m->anim = X_STOP_L;
                    m->frame = 29;
                } else {
                    m->frame += f;
                    d = 0;
                }
                break;
            case GO_WALK:
                if (advance(m, &d, 1, 24))
                    m->frame = 4;
                break;
            case GO_RUN:
                if (24 <= m->frame + f) {
                    d *= (m->frame + f - 24) / f;
                    m->anim = X_RUN_START_R;
                    m->frame = 34;
                } else if (m->frame < 14 && 14 <= m->frame + f) {
                    d *= (m->frame + f - 14) / f;
                    m->anim = X_RUN_START_L;
                    m->frame = 39;
                } else {
                    m->frame += f;
                    d = 0;
                }
                break;
            }
            break;
        case X_STOP_R:
            if (28 <= m->frame + d) {
                m->anim = X_STAND;
                m->frame = 0;
            } else {
                m->frame += d;
            }
            d = 0;
            break;
        case X_STOP_L:
            if (33 <= m->frame + d) {
                m->anim = X_STAND;
                m->frame = 0;
            } else {
                m->frame += d;
            }
            d = 0;
            break;
        case X_RUN_START_R:
            if (advance(m, &d, 2, 38)) {
                m->anim = X_RUN;
                m->frame = 48;
            }
            break;
        case X_RUN_START_L:
            if (advance(m, &d, 2, 43)) {
                m->anim = X_RUN;
                m->frame = 43;
            }
            break;
        case X_RUN:
            f = d * 0.5f;
            if (go == GO_RUN) {
                if (advance(m, &d, 2, 53))
                    m->frame = 43;
            } else if (53 <= m->frame + f) {
                d *= (m->frame + f - 53) / f;
                m->anim = X_RUN_STOP_R;
                m->frame = 53;
            } else if (m->frame < 48 && 48 <= m->frame + f) {
                d *= (m->frame + f - 48) / f;
                m->anim = X_RUN_STOP_L;
                m->frame = 59;
            } else {
                m->frame += f;
                d = 0;
            }
            break;
        case X_RUN_STOP_R:
            if (advance(m, &d, 2, 58)) {
                m->anim = X_WALK;
                m->frame = 14;
            }
            break;
        case X_RUN_STOP_L:
            if (advance(m, &d, 2, 64)) {
                m->anim = X_WALK;
                m->frame = 4;
            }
            break;
        case X_BACK_START:
            if (back_up(m, &d, 1, 24)) {
                m->anim = X_BACK;
                m->frame = 24;
            }
            break;
        case X_BACK_START2:
            if (back_up(m, &d, 1, 29)) {
                m->anim = X_BACK;
                m->frame = 14;
            }
            break;
        case X_BACK:
            f = d;
            switch (go) {
            case GO_BACK:
                if (m->frame - f < 4) {
                    d *= -((m->frame - f) - 4) / f;
                    m->frame = 24;
                } else {
                    m->frame -= f;
                    d = 0;
                }
                break;
            case GO_STAND: case GO_AIR:
                if (m->frame - f < 4) {
                    d *= -((m->frame - f) - 4) / f;
                    m->anim = X_BACK_STOP;
                    m->frame = 4;
                } else if (14 < m->frame && m->frame - f <= 14) {
                    d *= ((m->frame - f) - 14) / f;
                    m->anim = X_BACK_STOP2;
                    m->frame = 74;
                } else {
                    m->frame -= f;
                    d = 0;
                }
                break;
            default:
                m->anim = X_WALK;
                break;
            }
            break;
        case X_BACK_STOP:
            if (m->frame <= d) {
                m->anim = X_STAND;
                m->frame = 0;
            } else {
                m->frame -= d;
            }
            d = 0;
            break;
        case X_BACK_STOP2:
            if (m->frame - d <= 70) {
                m->anim = X_STAND;
                m->frame = 0;
            } else {
                m->frame -= d;
            }
            d = 0;
            break;
        case X_JUMP:
            if (go != GO_AIR) {
                m->anim = X_LAND;
                break;
            }
            d = 0;
            f = 5.0f / tps;
            if (68 <= m->frame + f) {
                m->anim = X_AIR;
                m->frame = 89;
                show_trans(m, 1);
            } else {
                m->frame += f;
            }
            break;
        case X_AIR:
            if (go == GO_AIR) {
                if (m->flying)
                    m->anim = X_TRANSFORM;
                else
                    d = 0;
            } else {
                m->anim = X_LAND;
                m->frame = 68;
                show_trans(m, 0);
            }
            break;
        case X_LAND:
            if (go == GO_AIR) {
                m->anim = X_JUMP;
                break;
            }
            d = 0;
            f = 5.0f / tps;
            if (m->frame - f <= 65) {
                m->anim = X_STAND;
                m->frame = 0;
            } else {
                m->frame -= f;
            }
            break;
        case X_TRANSFORM:
            d = 0;
            f = 30.0f / tps;
            if (m->flying) {
                m->frame -= f;
                if (m->frame <= 20) {
                    if (m->mode & MODE_AIR) {
                        /* the plane takes over */
                        m->mode = (short)((m->status ? 0 : MODE_FLY | MODE_FOOT) | MODE_JETS);
                        m->throttle = 0.5f;
                    }
                    if (m->frame < 0)
                        m->frame = 0;
                }
            } else {
                m->frame += f;
                if (20 < m->frame) {
                    if (m->mode & MODE_FLY) {
                        m->mode = (short)((m->status ? 0 : MODE_AIR | MODE_FOOT) | MODE_JETS);
                        m->in_throttle = 0;
                    }
                    if (89 <= m->frame) {
                        m->anim = X_AIR;
                        m->frame = 89;
                    }
                }
            }
            break;
        default:
            m->anim = X_STAND;
            d = 0;
            break;
        }
    }

    switch (m->anim) {
    case 1: case 2: case 3: case 4: case 5:
        m->vmin[1] = 5.0f / tps;
        if (m->mode & MODE_WALK) {
            m->vmax[1] = 25.0f / tps;
            if (m->vmax[1] < m->vel[1])
                m->vel[1] = m->vmax[1];
        }
        break;
    case 6: case 7: case 8:
        m->vmin[1] = 19.0f / tps;
        if (m->vel[1] < m->vmin[1])
            m->vel[1] = m->vmin[1];
        break;
    case 9: case 10:
        m->vmin[1] = 17.0f / tps;
        if (m->vel[1] < m->vmin[1])
            m->vel[1] = m->vmin[1];
        break;
    case 11: case 12: case 13: case 14: case 15:
        m->vmax[1] = -5.0f / tps;
        break;
    }
}

/* T: the Ex-Centinel turns into a plane (it jumps first) and back */
void transform(void)
{
    if (my_mek->type != EX_CENTINEL)
        return;
    if (my_mek->mode & MODE_WALK) {
        my_mek->mode = (short)((my_mek->status ? 0 : MODE_AIR | MODE_FOOT) | MODE_JETS);
        my_mek->in_throttle = 0.05f;
    }
    my_mek->flying = !my_mek->flying;
}

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
