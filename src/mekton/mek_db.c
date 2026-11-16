/*
 * mek_db.c - the scene database of Mekton: models, objects and the
 * keyframe animation of the mechs' joints.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mekton.h"

Obj *eye, *vantage, *cg_vantage, *ex_centinel_trans, *ex_centinel_walk;
Obj *cg_ex_centinel, *pod, *cg_pod;
Obj *colony_int[9], *exterior_windows, *colony_windows, *colony_city;
Obj *colony_lights, *waterfall, *powerplant, *bolts, *obj_plasma;
Obj *obj_small_exp, *obj_big_exp, *obj_double_door, *obj_plasma_turret;
pfGroup *isect_group, *isect_group_w_self;

/* --- objects ------------------------------------------------------------ */

Obj *make_obj(pfNode *model, int type, pfNode *parent)
{
    Obj *o = (Obj *)pfMalloc(sizeof(Obj), pfGetSharedArena());
    o->model = model;
    o->sw = pfNewSwitch();
    o->type = type;
    if (type == 0) {
        o->dcs = NULL;
        pfAddChild(o->sw, model);
    } else if (type == 1) {
        o->dcs = pfNewDCS();
        pfAddChild(o->dcs, model);
        pfAddChild(o->sw, o->dcs);
    }
    pfSwitchVal(o->sw, PFSWITCH_ON);
    if (parent)
        pfAddChild(parent, o->sw);
    return o;
}

Obj *load_obj(const char *file, int type, pfNode *parent)
{
    pfNode *n = pfdLoadFile(file);
    if (!n)
        n = pfNewGroup();
    return make_obj(n, type, parent);
}

Obj *clone_obj(Obj *o, pfNode *parent)
{
    return make_obj(pfClone(o->model, 0), o->type, parent);
}

/* free lists of the mech models: 0 Vantage, 1 Ex-Centinel walking,
 * 2 Vantage collision, 3 Ex-Centinel collision, 4 Ex-Centinel transforming */
#define NKINDS 5
#define MAX_FREE 32
static Obj *free_list[NKINDS][MAX_FREE];
static int nfree[NKINDS];

Obj *find_obj(int kind)
{
    Obj *o;
    if (nfree[kind] == 0)
        return NULL;
    o = free_list[kind][--nfree[kind]];
    pfSwitchVal(o->sw, PFSWITCH_ON);
    return o;
}

void free_obj(int kind, Obj *o)
{
    pfSwitchVal(o->sw, PFSWITCH_OFF);
    if (nfree[kind] < MAX_FREE)
        free_list[kind][nfree[kind]++] = o;
}

void find_mek_objs(Mek *m)
{
    if (m->type == VANTAGE) {
        m->walk_obj = m->obj = find_obj(0);
        m->cg_obj = find_obj(2);
    } else if (m->type == EX_CENTINEL) {
        m->walk_obj = m->obj = find_obj(1);
        m->trans_obj = find_obj(4);
        m->cg_obj = find_obj(3);
    }
    m->snd = NULL;
}

void free_mek_objs(Mek *m)
{
    if (m->type == VANTAGE) {
        free_obj(0, m->walk_obj);
        free_obj(2, m->cg_obj);
    } else if (m->type == EX_CENTINEL) {
        free_obj(1, m->walk_obj);
        free_obj(4, m->trans_obj);
        free_obj(3, m->cg_obj);
    }
}

static int count_vantage, count_cg_vantage, count_ex, count_ex_trans, count_cg_ex;

static Obj *new_vantage(void)
{
    Obj *o = count_vantage ? clone_obj(vantage, scene) : vantage;
    da_init(o->dcs);
    count_vantage++;
    return o;
}

static Obj *new_cg_vantage(void)
{
    Obj *o = count_cg_vantage ? clone_obj(cg_vantage, scene) : cg_vantage;
    /* each collision hull gets its own intersection mask */
    pfNodeTravMask(o->model, PFTRAV_ISECT, 0xdead0010u + count_cg_vantage,
                   PFTRAV_SELF | PFTRAV_DESCEND, PF_SET);
    count_cg_vantage++;
    return o;
}

static Obj *new_ex_centinel(void)
{
    Obj *o = count_ex ? clone_obj(ex_centinel_walk, scene) : ex_centinel_walk;
    da_init(o->dcs);
    count_ex++;
    return o;
}

static Obj *new_ex_centinel_trans(void)
{
    Obj *o = count_ex_trans ? clone_obj(ex_centinel_trans, scene) : ex_centinel_trans;
    da_init(o->dcs);
    count_ex_trans++;
    return o;
}

static Obj *new_cg_ex_centinel(void)
{
    Obj *o = count_cg_ex ? clone_obj(cg_ex_centinel, scene) : cg_ex_centinel;
    pfNodeTravMask(o->model, PFTRAV_ISECT, 0xdead0020u + count_cg_ex,
                   PFTRAV_SELF | PFTRAV_DESCEND, PF_SET);
    count_cg_ex++;
    return o;
}

/* collision models are wrapped in a group so that clones keep a parent */
static void wrap_model(Obj *o)
{
    pfGroup *g = pfNewGroup();
    pfAddChild(g, o->model);
    pfReplaceChild(o->dcs, o->model, g);
    o->model = g;
}

void init_db(void)
{
    static const char *colony_files[9] = {
        "c_AG2MTN.pfb", "c_AGRO1.pfb", "c_AGRO3.pfb", "c_ARENA.pfb", "c_CANYON.pfb",
        "c_CITY2.pfb", "c_CITY3.pfb", "c_DAM.pfb", "c_SEA.pfb"
    };
    Obj *dead;
    pfSwitch *sw;
    int i;

    eye = load_obj("eye.pfb", 1, scene);
    vantage = load_obj("vantage.pfb", 1, scene);
    cg_vantage = load_obj("cg_Vantage.pfb", 1, scene);
    wrap_model(cg_vantage);
    ex_centinel_trans = load_obj("ex_trans.pfb", 1, scene);
    ex_centinel_walk = load_obj("ex_walk.pfb", 1, scene);
    cg_ex_centinel = load_obj("cg_Vantage.pfb", 1, scene);
    wrap_model(cg_ex_centinel);
    pod = load_obj("eye.pfb", 1, scene);
    cg_pod = load_obj("eye.pfb", 1, scene);
    for (i = 0; i < 9; i++)
        colony_int[i] = load_obj(colony_files[i], 0, scene);
    exterior_windows = load_obj("exterior_windows.pfb", 0, scene);
    colony_windows = load_obj("colony_windows.pfb", 0, scene);
    colony_city = load_obj("colony_city.pfb", 0, scene);
    colony_lights = load_obj("colony_lights.pfb", 0, scene);
    waterfall = load_obj("waterfall.pfb", 0, scene);
    powerplant = load_obj("powerplant.pfb", 0, scene);
    bolts = load_obj("bolts.pfb", 0, scene);
    obj_plasma = load_obj("plasma.pfb", 1, scene);
    obj_small_exp = load_obj("small_exp.pfb", 1, scene);
    obj_big_exp = load_obj("small_exp.pfb", 1, scene);
    obj_double_door = load_obj("double_door.pfb", 1, scene);
    obj_plasma_turret = load_obj("plasma_turret.pfb", 1, scene);

    isect_group = pfNewGroup();
    isect_group_w_self = pfNewGroup();
    pfAddChild(isect_group_w_self, isect_group);
    for (i = 0; i < 9; i++)
        pfAddChild(isect_group, colony_int[i]->model);
    pfAddChild(isect_group, colony_windows->model);
    pfAddChild(isect_group, colony_city->model);
    pfAddChild(isect_group, waterfall->model);
    pfAddChild(isect_group, powerplant->model);
    pfNodeTravMask(powerplant->model, PFTRAV_ISECT, 0xdead0000u,
                   PFTRAV_SELF | PFTRAV_DESCEND, PF_SET);

    /* the wreck shown in place of a dead Vantage */
    dead = load_obj("dead.pfb", 1, scene);
    pfSwitchVal(dead->sw, PFSWITCH_OFF);
    sw = pfNewSwitch();
    pfAddChild(sw, pfClone(dead->model, 0));
    pfSwitchVal(sw, PFSWITCH_OFF);
    pfInsertChild(vantage->model, 0, sw);

    for (i = 0; i < MAX_MEKTONS; i++) {
        mektons[i] = (Mek *)pfMalloc(sizeof(Mek), pfGetSharedArena());
        memset(mektons[i], 0, sizeof(Mek));
        mektons[i]->s124 = -1;
        free_obj(0, new_vantage());
        free_obj(2, new_cg_vantage());
        free_obj(1, new_ex_centinel());
        free_obj(4, new_ex_centinel_trans());
        free_obj(3, new_cg_ex_centinel());
    }
    init_pods();
    init_turrets();
    init_doors();
    init_projectiles();
    init_explosions();
}

void update_db(void)
{
    int i;
    for (i = 0; i < number_mektons; i++) {
        Mek *m = mektons[i];
        pfCopyVec3(m->obj->trans, m->origin);
        pfDCSTrans(m->obj->dcs, m->origin[0], m->origin[1], m->origin[2]);
        pfCopyVec3(m->cg_obj->trans, m->origin);
        pfDCSTrans(m->cg_obj->dcs, m->origin[0], m->origin[1], m->origin[2]);
        pfCopyVec3(m->obj->rot, m->hpr);
        pfDCSRot(m->obj->dcs, m->hpr[0], m->hpr[1], m->hpr[2]);
        pfCopyVec3(m->cg_obj->rot, m->hpr);
        pfDCSRot(m->cg_obj->dcs, m->hpr[0], m->hpr[1], m->hpr[2]);
    }
    animation_control();
    update_plasma_objs();
}

/* the model origin of my mek is offset from its reference point */
void adjust_origin(void)
{
    pfXformPt3(my_mek->origin, my_mek->t.model_offset, mek_mat);
}

/* --- joint animation ---------------------------------------------------- */

/* keyframes stored in the user data of each animated DCS:
 * flags (1 trans, 2 rot, 4 scale, 8 xyz scale), frame count, then the
 * arrays in that order */
typedef struct {
    pfDCS *dcs;
    int flags, nframes;
    float *trans, *rot, *scale, *scalexyz;
} Joint;

typedef struct {
    int njoints;
    float frame;
    Joint *joints;
} Anim;

static int njoint_found;
static pfDCS *joint_found[256];

static void find_joints(pfNode *n)
{
    int i, k;
    if (!pfIsOfType(n, pfGetGroupClassType()))
        return;
    if (pfIsExactType(n, pfGetDCSClassType())) {
        void *ud = pfGetUserData(n);
        if (ud && pfGetSize(ud)) {
            for (k = 0; k < njoint_found; k++)
                if (joint_found[k] == n)
                    break;
            if (k == njoint_found && njoint_found < 256)
                joint_found[njoint_found++] = n;
        }
    }
    for (i = 0; i < pfGetNumChildren(n); i++)
        find_joints(pfGetChild(n, i));
}

void da_init(pfDCS *root)
{
    Anim *a;
    int i;
    njoint_found = 0;
    find_joints(root);
    a = (Anim *)malloc(sizeof(Anim));
    a->njoints = njoint_found;
    a->frame = 0;
    a->joints = (Joint *)calloc(njoint_found + 1, sizeof(Joint));
    for (i = 0; i < njoint_found; i++) {
        Joint *j = &a->joints[i];
        int *ud = (int *)pfGetUserData(joint_found[i]);
        float *p = (float *)(ud + 2);
        j->dcs = joint_found[i];
        j->flags = ud[0];
        j->nframes = ud[1];
        if (j->flags & 1) { j->trans = p; p += 3 * j->nframes; }
        if (j->flags & 2) { j->rot = p; p += 3 * j->nframes; }
        if (j->flags & 4) { j->scale = p; p += j->nframes; }
        if (j->flags & 8) j->scalexyz = p;
    }
    pfUserData(root, a);
}

void da_frame(pfDCS *root, float frame)
{
    Anim *a = (Anim *)pfGetUserData(root);
    int f = (int)frame, i;
    float t = frame - (float)f;

    if (!a)
        return;
    for (i = 0; i < a->njoints; i++) {
        Joint *j = &a->joints[i];
        if (t == 0) {
            if (f >= j->nframes)
                return;
            if (j->trans)
                pfDCSTrans(j->dcs, j->trans[3 * f], j->trans[3 * f + 1], j->trans[3 * f + 2]);
            if (j->rot)
                pfDCSRot(j->dcs, j->rot[3 * f], j->rot[3 * f + 1], j->rot[3 * f + 2]);
            if (j->scale)
                pfDCSScale(j->dcs, j->scale[f]);
            if (j->scalexyz)
                pfDCSScaleXYZ(j->dcs, j->scalexyz[3 * f], j->scalexyz[3 * f + 1], j->scalexyz[3 * f + 2]);
        } else {
            int g = f + 1, k;
            float v[3];
            if (g >= j->nframes)
                return;
            if (j->trans) {
                for (k = 0; k < 3; k++)
                    v[k] = (j->trans[3 * g + k] - j->trans[3 * f + k]) * t + j->trans[3 * f + k];
                pfDCSTrans(j->dcs, v[0], v[1], v[2]);
            }
            if (j->rot) {
                for (k = 0; k < 3; k++)
                    v[k] = (j->rot[3 * g + k] - j->rot[3 * f + k]) * t + j->rot[3 * f + k];
                pfDCSRot(j->dcs, v[0], v[1], v[2]);
            }
            if (j->scale)
                pfDCSScale(j->dcs, (j->scale[g] - j->scale[f]) * t + j->scale[f]);
            if (j->scalexyz) {
                for (k = 0; k < 3; k++)
                    v[k] = (j->scalexyz[3 * g + k] - j->scalexyz[3 * f + k]) * t + j->scalexyz[3 * f + k];
                pfDCSScaleXYZ(j->dcs, v[0], v[1], v[2]);
            }
        }
    }
    a->frame = frame;
}

float da_get_frame(pfDCS *root)
{
    Anim *a = (Anim *)pfGetUserData(root);
    return a ? a->frame : 0;
}
