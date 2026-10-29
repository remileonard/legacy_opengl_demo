/*
 * mek_weapons.c - plasma shots, hits, damage and explosions.
 *
 * Each player tests the shots of the others against its own mech, so a
 * shot of my mech only meets the colony here; the hits on other mechs
 * are found by their owners (see mek_comm.c).
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mekton.h"

#define MAX_PLASMA 48

Plasma *plasma[MAX_PLASMA];
int number_plasma, my_plasma, plasma_id;
static int free_aim;

/* gun muzzles in the frame of each type, and of the Ex-Centinel plane */
static const pfVec3 gun_offset[2] = { { -1.43784f, -0.39436f, 5.3282f }, { 1.9f, 4, 2 } };
static const pfVec3 plane_gun = { 0, 4, 2 };
static const float gun_length[2] = { 0, 7 };

static Obj *new_plasma(void)
{
    static int first = 1;
    if (first) {
        first = 0;
        return obj_plasma;
    }
    return clone_obj(obj_plasma, scene);
}

void init_projectiles(void)
{
    int i;
    for (i = 0; i < MAX_PLASMA; i++) {
        plasma[i] = (Plasma *)calloc(1, sizeof(Plasma));
        plasma[i]->obj = new_plasma();
        pfSwitchVal(plasma[i]->obj->sw, PFSWITCH_OFF);
    }
    my_plasma = 0;
    number_plasma = 0;
}

void reset_projectiles(void)
{
    int i;
    for (i = 0; i < MAX_PLASMA; i++)
        pfSwitchVal(plasma[i]->obj->sw, PFSWITCH_OFF);
    my_plasma = 0;
    number_plasma = 0;
}

/* my shots are kept at the front of the array */
Plasma *get_plasma(int mine)
{
    Plasma *p;
    if (number_plasma < MAX_PLASMA) {
        if (!mine)
            return plasma[number_plasma++];
        p = plasma[number_plasma];
        plasma[number_plasma] = plasma[my_plasma];
        plasma[my_plasma] = p;
        my_plasma++;
        number_plasma++;
        return p;
    }
    if (!mine && my_plasma == MAX_PLASMA)
        my_plasma--;
    return plasma[MAX_PLASMA - 1];
}

static void free_plasma(int i)
{
    Plasma *p = plasma[i];
    if (i < my_plasma) {
        plasma[i] = plasma[my_plasma - 1];
        my_plasma--;
        i = my_plasma;
    }
    plasma[i] = plasma[number_plasma - 1];
    plasma[number_plasma - 1] = p;
    number_plasma--;
    pfSwitchVal(p->obj->sw, PFSWITCH_OFF);
}

Mek *find_mek_by_cid(unsigned int cid)
{
    int i;
    for (i = 0; i < number_mektons; i++)
        if (mektons[i]->travmask == cid)
            return mektons[i];
    return NULL;
}

/* moves a shot by dt seconds; it stops at the first thing it meets */
void update_plasma_pos(Plasma *p, float dt, int stop)
{
    pfSegSet ss;
    pfHit **hits[PFIS_MAX_SEGS];
    pfVec3 pt;
    int flags;

    pfCopyVec3(p->old_pos, p->pos);
    p->pos[0] += p->vel[0] * dt;
    p->pos[1] += p->vel[1] * dt;
    p->pos[2] += p->vel[2] * dt;
    pfMakePtsSeg(&ss.segs[0], p->old_pos, p->pos);
    ss.mode = PFTRAV_IS_PRIM | PFTRAV_IS_NORM | PFTRAV_IS_CULL_BACK | 0x400;
    ss.userData = NULL;
    ss.activeMask = 1;
    ss.isectMask = 0xffffffffU;
    ss.bound = NULL;
    ss.discFunc = NULL;
    if (pfNodeIsectSegs(p->turret ? isect_group_w_self : isect_group, &ss, hits)) {
        pfHit *path;
        pfNode *n;
        pfQueryHit(*hits[0], PFQHIT_FLAGS, &flags);
        pfQueryHit(*hits[0], PFQHIT_POINT, p->hit_pt);
        pfCopyVec3(pt, p->hit_pt);
        if (stop)
            pfCopyVec3(p->pos, pt);
        pfQueryHit(*hits[0], PFQHIT_PATH, &path);
        n = (pfNode *)pfGet(path, 2);
        p->hit_mask = n ? pfGetNodeTravMask(n, PFTRAV_ISECT) : 0;
        p->state = 1;
    }
    if (p->state != 1)
        return;
    switch ((p->hit_mask & ~15u) - 0xdead0010u) {
    case 0: case 16: {
        /* a mech */
        Mek *m = find_mek_by_cid(p->hit_mask);
        Explosion *e;
        if (m && !m->status) {
            if (!p->turret && (my_mek->team != m->team || my_mek->team == 0))
                my_mek->score += 10;
            send_hit(m, p);
        }
        e = start_explosion(0, p->hit_pt, 10);
        if (m)
            tie_explosion(e, 1, m, p->hit_pt);
        break;
    }
    default:
        start_explosion(0, p->hit_pt, 10);
        break;
    }
}

void fire_plasma(void)
{
    Plasma *p;
    pfVec3 chpr;
    pfMatrix m;

    if ((my_mek->flags & 128) || 1.0f <= my_mek->heat)
        return;
    my_mek->heat += 0.05f;
    p = get_plasma(1);
    p->id = plasma_id++;
    p->state = 0;
    p->turret = 0;
    p->hit_mask = 0;
    p->damage = 0.5f;
    p->owner = my_id;
    chpr[0] = my_mek->chpr[0] - my_mek->aim[0];
    chpr[1] = my_mek->chpr[1] + my_mek->aim[1];
    chpr[2] = my_mek->chpr[2];
    colony2world(my_mek->cpos, chpr, NULL, NULL, p->hpr, NULL);
    if (my_mek->type == EX_CENTINEL && my_mek->anim == 19)
        pfXformPt3(p->pos, plane_gun, mek_mat);
    else
        pfXformPt3(p->pos, gun_offset[my_mek->type], mek_mat);
    pfSetVec3(p->vel, 0, 1000, 0);
    pfMakeEulerMat(m, p->hpr[0], p->hpr[1], p->hpr[2]);
    pfXformVec3(p->vel, p->vel, m);
    pfSwitchVal(p->obj->sw, PFSWITCH_ON);
    p->obj->rot[0] = p->hpr[0];
    p->obj->rot[1] = p->hpr[1];
    update_plasma_pos(p, (gun_length[my_mek->type] + 39.0f) / 1000.0f, 0);
    if (comm)
        send_plasma(p);
}

void update_projectiles(void)
{
    float dt = 1.0f / tps;
    int i;

    for (i = 0; i < my_plasma; i++) {
        Plasma *p = plasma[i];
        if (p->state == 0) {
            update_plasma_pos(p, dt, 1);
            if (comm)
                send_plasma(p);
        } else if (p->state == 1) {
            p->state = 2;
            if (comm)
                send_plasma(p);
        } else if (p->state == 2) {
            free_plasma(i);
            i--;
        }
    }
    /* the shots of the others live until their owners stop sending them */
    for (; i < number_plasma; i++) {
        if (--plasma[i]->ttl < 0) {
            free_plasma(i);
            i--;
        }
    }
    if (shadow || my_mek->status == 2)
        return;
    my_mek->heat -= 0.1f / tps;
    if (my_mek->heat < 0)
        my_mek->heat = 0;
    if (mek_input_state.e & 2) {
        mek_input_state.e &= ~2;
        fire_plasma();
    } else if (mek_input_state.space & 2) {
        mek_input_state.space &= ~2;
        fire_plasma();
    }
    /* W and Q raise and lower the guns */
    if (mek_input_state.w) {
        if (mek_input_state.shift) {
            my_mek->aim[1] = 0;
        } else {
            my_mek->aim[1] += 0.5f;
            if (20 < my_mek->aim[1])
                my_mek->aim[1] = 20;
        }
    } else if (mek_input_state.q) {
        if (mek_input_state.shift) {
            my_mek->aim[1] = 0;
        } else {
            my_mek->aim[1] -= 0.5f;
            if (my_mek->aim[1] < -20)
                my_mek->aim[1] = -20;
        }
    }
    /* tab and the left button aim with the pointer */
    if (mek_input_state.tab && mek_input_state.left) {
        extern int center_x, center_y;
        float s, c, d;
        pfVec2 v;
        pfSinCos(chan_fov * 0.5f, &s, &c);
        d = (float)center_x * c / s;
        v[0] = (float)(mek_input_state.x - center_x);
        v[1] = d;
        my_mek->aim[0] = xasin(v[0] / pfLengthVec2(v)) / 10.0f;
        if (20 < my_mek->aim[0]) my_mek->aim[0] = 20;
        if (my_mek->aim[0] < -20) my_mek->aim[0] = -20;
        v[0] = (float)(mek_input_state.y - center_y);
        my_mek->aim[1] = xasin(v[0] / pfLengthVec2(v)) / 10.0f;
        if (20 < my_mek->aim[1]) my_mek->aim[1] = 20;
        if (my_mek->aim[1] < -20) my_mek->aim[1] = -20;
        free_aim = 1;
    } else if (free_aim) {
        my_mek->aim[1] = 0;
        my_mek->aim[0] = 0;
        free_aim = 0;
    }
}

/* the shot is drawn as a flat flare turned to the camera */
static void update_plasma_obj(Plasma *p)
{
    pfVec3 v, c;
    pfVec2 s;
    float d, a;
    pfCopyVec3(p->obj->trans, p->pos);
    pfDCSTrans(p->obj->dcs, p->pos[0], p->pos[1], p->pos[2]);
    pfSubVec3(v, p->pos, camera.xyz);
    pfCrossVec3(c, v, p->vel);
    pfNormalizeVec3(c);
    s[0] = -p->vel[1];
    s[1] = p->vel[0];
    pfNormalizeVec2(s);
    d = s[0] * c[0] + s[1] * c[1];
    if (1 < d)
        a = 0;
    else if (d < -1)
        a = 180;
    else
        a = pfArcCos(d);
    a = c[2] < 0 ? -a - 90 : a - 90;
    if (a < 0)
        a += 360;
    p->obj->rot[2] = a;
    pfDCSRot(p->obj->dcs, p->obj->rot[0], p->obj->rot[1], p->obj->rot[2]);
}

void update_plasma_objs(void)
{
    int i;
    for (i = 0; i < number_plasma; i++)
        update_plasma_obj(plasma[i]);
}

/* --- damage ------------------------------------------------------------- */

static float frand(void)
{
    return (float)(rand() & 0xffff) / 65535.0f;
}

/* a shot of another player hit my mech */
void hit_mek(Mek *m, const HitMsg *h)
{
    float dmg;
    m->hit_by = h->turret ? -1 : h->shooter;
    m->flags |= 1;
    dmg = frand() * h->damage * m->t.f300;
    m->f276 -= dmg;
    if (m->f276 <= 0) {
        if (!m->status) {
            destroy_mek(m);
            if (comm)
                send_damage(m->hit_by, 3);
        }
        return;
    }
    if (!(frand() < dmg))
        return;
    /* something broke */
    switch (rand() % 5) {
    case 0: m->flags |= 2; break;
    case 1:
        m->flags |= 4;
        m->f280 *= frand();
        m->t.max_jet = mek_types[m->type].max_jet * m->f280;
        break;
    case 2: m->flags |= (m->flags & 16) ? 32 : 16; break;
    case 3: m->flags |= 64; break;
    case 4: m->flags |= 128; break;
    }
}

/* the wreck of a dead Vantage is the first child of its model */
void switch_dead(Mek *m)
{
    if (m->type == VANTAGE)
        pfSwitchVal(pfGetChild(m->obj->model, 0),
                    m->status == 2 ? PFSWITCH_ON : PFSWITCH_OFF);
}

void destroy_mek(Mek *m)
{
    m->status = 2;
    m->mode &= 0xff87;
    switch_dead(m);
}

/* --- explosions --------------------------------------------------------- */

static Explosion *exp_list[2];

static Explosion *make_exp_list(int type, int n)
{
    Explosion *first = NULL, *last = NULL;
    int i;
    for (i = 0; i < n; i++) {
        Explosion *e = (Explosion *)calloc(1, sizeof(Explosion));
        static int first_small = 1, first_big = 1;
        e->type = type;
        if (type == 0) {
            e->obj = first_small ? obj_small_exp : clone_obj(obj_small_exp, scene);
            first_small = 0;
        } else {
            e->obj = first_big ? obj_big_exp : clone_obj(obj_big_exp, scene);
            first_big = 0;
        }
        e->scale = 1;
        e->seq = e->obj->model;
        pfSeqMode(e->seq, PFSEQ_STOP);
        e->tied = 0;
        if (last) {
            last->next = e;
            e->prev = last;
        } else {
            first = e;
        }
        last = e;
    }
    last->next = first;
    first->prev = last;
    return first;
}

void init_explosions(void)
{
    exp_list[0] = make_exp_list(0, 48);
    exp_list[1] = make_exp_list(1, 8);
}

Explosion *start_explosion(int type, const pfVec3 pos, float scale)
{
    Explosion *e = exp_list[type];
    pfVec3 cpos, ccam, hpr;
    pfVec2 d;
    float h;

    exp_list[type] = e->next;
    pfCopyVec3(e->pos, pos);
    e->scale = scale;
    /* turned to the camera */
    world2colony(pos, NULL, NULL, cpos, NULL, NULL);
    world2colony(camera.xyz, NULL, NULL, ccam, NULL, NULL);
    d[0] = cpos[0] - ccam[0];
    d[1] = cpos[1] - ccam[1];
    pfNormalizeVec2(d);
    h = 0 < d[0] ? -pfArcCos(d[1]) : pfArcCos(d[1]);
    pfSetVec3(hpr, h, 0, 0);
    colony2world(cpos, hpr, NULL, NULL, hpr, NULL);
    pfCopyVec3(e->obj->trans, pos);
    pfDCSTrans(e->obj->dcs, pos[0], pos[1], pos[2]);
    pfCopyVec3(e->obj->rot, hpr);
    pfDCSRot(e->obj->dcs, hpr[0], hpr[1], hpr[2]);
    pfDCSScale(e->obj->dcs, scale);
    pfSeqMode(e->seq, PFSEQ_START);
    e->tied = 0;
    return e;
}

/* the explosion follows a mech */
void tie_explosion(Explosion *e, int tied, Mek *m, const pfVec3 pos)
{
    pfMatrix mat, inv;
    e->tied = tied;
    e->mek = m;
    /* keep the offset in the frame of the mech */
    pfMakeEulerMat(mat, m->hpr[0], m->hpr[1], m->hpr[2]);
    mat[3][0] = m->pos[0];
    mat[3][1] = m->pos[1];
    mat[3][2] = m->pos[2];
    pfInvertOrthoMat(inv, mat);
    pfXformPt3(e->offset, pos, inv);
}

void update_explosions(void)
{
    Explosion *e = exp_list[0]->prev;
    while (pfGetSeqMode(e->seq) != PFSEQ_STOP && e != exp_list[0]) {
        if (e->tied == 1) {
            pfMatrix mat;
            Mek *m = e->mek;
            pfMakeEulerMat(mat, m->hpr[0], m->hpr[1], m->hpr[2]);
            mat[3][0] = m->pos[0];
            mat[3][1] = m->pos[1];
            mat[3][2] = m->pos[2];
            pfXformPt3(e->pos, e->offset, mat);
            pfCopyVec3(e->obj->trans, e->pos);
            pfDCSTrans(e->obj->dcs, e->pos[0], e->pos[1], e->pos[2]);
        }
        e = e->prev;
    }
}
