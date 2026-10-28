/*
 * ci_world.c - Certain Impact: the world around the player: terrain
 * (vgTerrain), battle regions with enemy airplanes, balloons and flak
 * cannons, ships, the bridge, the bunkers, the train and the blimp, and the
 * effects of the weapons (mardi::detonate and mardi::collision).
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ci.h"

/* --- terrain (vgTerrain) ------------------------------------------------------- */

#define MAX_DOORS 16
#define MAX_SEQS 64
#define MAX_DAMAGED 256

static struct {
    vgObject *obj;
    int doors_active;
    float door_pos;
    int ndoors;
    pfNode *ldoor[MAX_DOORS], *rdoor[MAX_DOORS];
    int nseqs;
    pfNode *seqs[MAX_SEQS];
    int ndamaged;
    pfNode *damaged[MAX_DAMAGED];
} terrain;

static void terrain_classes(void)
{
    int i, n = vgNumParts(terrain.obj);
    for (i = 0; i < n; i++) {
        pfNode *p = vgGetPart(terrain.obj, i);
        const char *s = p->name;
        if (strncmp(s, "hangar", 6) == 0) vgPartClass(p, CL_HANGAR);
        else if (strncmp(s, "house", 5) == 0) vgPartClass(p, CL_HOUSE);
        else if (strcmp(s, "castle") == 0) vgPartClass(p, CL_CASTLE);
        else if (strncmp(s, "crates", 6) == 0) vgPartClass(p, CL_CRATES);
        else if (strcmp(s, "fbridge") == 0) vgPartClass(p, CL_BRIDGE);
        else if (strncmp(s, "tower", 5) == 0) vgPartClass(p, CL_HOUSE);
        else if (strncmp(s, "water", 5) == 0) vgPartClass(p, CL_WATER);
        else if (strncmp(s, "artil", 5) == 0) vgPartClass(p, CL_ARTIL);
        else if (strncmp(s, "trees", 5) == 0) vgPartClass(p, CL_TREES);
    }
}

void worldTerrainInitDoors(void)
{
    int i;
    terrain.ndoors = 0;
    for (i = 0; i < MAX_DOORS; i++) {
        terrain.ldoor[i] = vgFindPart(terrain.obj, "ldoor", i);
        terrain.rdoor[i] = vgFindPart(terrain.obj, "rdoor", i);
        if (!terrain.ldoor[i])
            break;
        vgPartPos(terrain.ldoor[i], 0, 0, 0, 0, 0, 0);
        vgPartPos(terrain.rdoor[i], 0, 0, 0, 0, 0, 0);
        terrain.ndoors++;
    }
    terrain.doors_active = 1;
    terrain.door_pos = 0;
}

/* the hangar doors slide open at the start of the game */
static void terrain_update_doors(void)
{
    int i;
    terrain.door_pos += 0.05f * gDt * 30;
    if (terrain.door_pos >= 7)
        terrain.doors_active = 0;
    for (i = 0; i < terrain.ndoors; i++) {
        vgPartPos(terrain.ldoor[i], terrain.door_pos, 0, 0, 0, 0, 0);
        vgPartPos(terrain.rdoor[i], -terrain.door_pos, 0, 0, 0, 0, 0);
    }
}

/* the windsocks and flags flutter at a random speed */
void worldTerrainAnimations(void)
{
    int i;
    float t;
    if (!terrain.nseqs)
        return;
    t = terrain.seqs[0]->nframes ? terrain.seqs[0]->times[0] : 0.2f;
    t += (float)((gRand - 0.5) * gDt);
    if (t > 0.4f) t = 0.4f;
    if (t < 0.1f) t = 0.1f;
    for (i = 0; i < terrain.nseqs; i++)
        vgSeqFrameTime(terrain.seqs[i], t);
}

static void add_named_seqs(vgObject *o, const char *name)
{
    int k;
    for (k = 0; terrain.nseqs < MAX_SEQS; k++) {
        pfNode *n = vgFindPart(o, name, k);
        if (!n)
            break;
        if (n->type == PFNODE_SEQUENCE)
            terrain.seqs[terrain.nseqs++] = n;
    }
}

static void switch_damaged(pfNode *n, int depth)
{
    int i;
    if (!n || depth > 64)
        return;
    if (n->type == PFNODE_SWITCH && n->name && strcmp(n->name, "damage") == 0)
        for (i = 0; i < n->nchildren; i++)
            if (n->children[i]->name && strcmp(n->children[i]->name, "dam") == 0)
                pfSwitchVal(n, i);
    for (i = 0; i < n->nchildren; i++)
        switch_damaged(n->children[i], depth + 1);
}

void ciSwitchToDamaged(pfNode *n)
{
    int i;
    for (i = 0; i < terrain.ndamaged; i++)
        if (terrain.damaged[i] == n)
            break;
    if (i == terrain.ndamaged && terrain.ndamaged < MAX_DAMAGED)
        terrain.damaged[terrain.ndamaged++] = n;
    switch_damaged(n, 0);
}

void worldTerrainUndamage(void)
{
    int i, k;
    for (i = 0; i < terrain.ndamaged; i++) {
        pfNode *n = terrain.damaged[i];
        for (k = 0; k < n->nchildren; k++)
            if (n->children[k]->name && strcmp(n->children[k]->name, "undam") == 0)
                pfSwitchVal(n, k);
    }
    terrain.ndamaged = 0;
}

pfNode *ciGetSwitchNode(vgHit *h)
{
    int i;
    for (i = h->pathlen - 1; i >= 0; i--)
        if (h->path[i]->name && strcmp(h->path[i]->name, "damage") == 0)
            return h->path[i];
    return NULL;
}

/* --- balloons (vgBalloon) -------------------------------------------------------- */

typedef struct Balloon {
    int type, hits, enabled;
    vgObject *obj;
    float x, y, z0, z, speed;
} Balloon;

static Balloon *balloon_new(int type, float x, float y, float z)
{
    static const char *names[3] = { "FuelBalloon", "AmmoBalloon", "HealthBalloon" };
    static const unsigned int cls[3] = { CL_FUEL_BALLOON, CL_AMMO_BALLOON, CL_HEALTH_BALLOON };
    Balloon *b = (Balloon *)calloc(1, sizeof(Balloon));
    b->type = type;
    b->obj = vgCopyObj(vgFindObj(names[type]));
    vgObjClass(b->obj, cls[type]);
    b->obj->user = b;
    b->speed = (float)(ciRand() * 5);
    b->x = x;
    b->y = y;
    b->z0 = b->z = z;
    vgObjPos(b->obj, x, y, z, 0, 0, 0);
    return b;
}

static void balloon_enable(Balloon *b)
{
    b->enabled = 1;
    vgAddSceneObj(gMainScene, b->obj);
}

static void balloon_disable(Balloon *b)
{
    b->enabled = 0;
    vgRemSceneObj(gMainScene, b->obj);
}

static void balloon_update(Balloon *b, float dt)
{
    if (!b->enabled)
        return;
    b->z += b->speed * dt;
    if (b->z >= 400) {
        b->z = 400;
        b->speed = -b->speed;
    } else if (b->z <= b->z0) {
        b->z = b->z0;
        b->speed = -b->speed;
    }
    vgObjPos(b->obj, b->x, b->y, b->z, 0, 0, 0);
}

static void balloon_explode(Balloon *b)
{
    ciCreateAirExplosion(b->obj->pos);
    balloon_disable(b);
}

/* --- enemy airplanes (vgEnemyAirPlane and vgEnemyMachGun) -------------------------- */

typedef struct Enemy {
    vgObject *obj;
    pfNode *smoke;
    int active;              /* in the scene and moving */
    int state;
    float hits;
    int maxhits;
    float cx, cy, maxalt, minalt, size2;
    float turn;              /* f96: degrees per frame at 30 Hz */
    float pos[6];
    float init[6];
    pfVec3 vel;
    /* machine gun */
    int trigger, prev;
    double trig_time, last_fire;
} Enemy;

static void enemy_initial_pos(Enemy *e, float *pos)
{
    float s = sqrtf(e->size2);
    pos[0] = (float)((0.5 - ciRand()) * s + e->cx);
    pos[1] = (float)((0.5 - ciRand()) * s + e->cy);
    pos[2] = (float)(e->minalt + ciRand() * 150);
    pos[3] = (float)(ciRand() * 360);
    pos[4] = 0;
    pos[5] = 0;
}

static Enemy *enemy_new(float x, float y, float minalt, float maxalt, float size2, int leader, float *formation, float rnd)
{
    static const char *allied[3] = { "Fe2b", "Sopwith", "American" };
    static const char *german[3] = { "Fokker", "Albatros", "RedBaron" };
    Enemy *e = (Enemy *)calloc(1, sizeof(Enemy));
    int k = (int)(ciRand() * 3) % 3;
    pfNode *prop;
    e->cx = x;
    e->cy = y;
    e->minalt = minalt;
    e->maxalt = maxalt;
    e->size2 = size2;
    e->turn = 3 * 0.4f * rnd * 1.25f;
    e->maxhits = gDifficulty == 2 ? 10 : 5;
    if (gUserPlane->side == 0) {
        e->obj = vgCopyObj(vgFindObj(allied[k]));
        vgObjClass(e->obj, CL_ALLIED_PLANE);
    } else {
        e->obj = vgCopyObj(vgFindObj(german[k]));
        vgObjClass(e->obj, CL_GERMAN_PLANE);
    }
    e->obj->user = e;
    prop = vgFindPart(e->obj, "prop", 0);
    if (prop)
        pfSwitchVal(prop, 1);
    e->smoke = vgFindPart(e->obj, "smoke", 0);
    if (e->smoke)
        pfSwitchVal(e->smoke, PFSWITCH_OFF);
    if (leader) {
        enemy_initial_pos(e, e->init);
        memcpy(formation, e->init, sizeof(e->init));
    } else {
        memcpy(e->init, formation, sizeof(e->init));
    }
    memcpy(e->pos, e->init, sizeof(e->pos));
    vgObjPosVec(e->obj, e->pos);
    e->last_fire = -1;
    return e;
}

static void enemy_add(Enemy *e)
{
    e->active = 1;
    vgAddSceneObj(gMainScene, e->obj);
}

static void enemy_remove(Enemy *e)
{
    e->active = 0;
    vgRemSceneObj(gMainScene, e->obj);
}

static void enemy_reset(Enemy *e)
{
    e->state = 0;
    e->hits = 0;
    memcpy(e->pos, e->init, sizeof(e->pos));
    pfSetVec3(e->vel, 0, 0, 0);
    if (e->smoke)
        pfSwitchVal(e->smoke, PFSWITCH_OFF);
    vgObjPosVec(e->obj, e->pos);
    enemy_add(e);
}

static void enemy_explode(Enemy *e)
{
    ciCreateAirExplosion(e->pos);
    enemy_remove(e);
}

static void enemy_dive(Enemy *e)
{
    e->state = 5;
    if (e->smoke)
        pfSwitchVal(e->smoke, 0);
}

static int enemy_gun_hit_cb(Ammo *a, vgHit *h, void *data)
{
    (void)a; (void)data;
    worldDetonate(1, h->point, h);
    return 0;
}

static void enemy_gun_update(Enemy *e, double t)
{
    if (e->trigger != e->prev) {
        e->prev = e->trigger;
    } else if (e->trig_time + 0.21 < t) {
        e->trigger = 0;
    }
    if (e->trigger && e->last_fire + 0.5 < t) {
        Plane *u = gUserPlane;
        pfVec3 pos, aim, uv;
        float d;
        pfSetVec3(pos, e->pos[0], e->pos[1], e->pos[2]);
        pfCopyVec3(uv, u->vel);
        pfNormalizeVec3(uv);
        aim[0] = u->pos[0] + uv[0] * 10 - pos[0];
        aim[1] = u->pos[1] + uv[1] * 10 - pos[1];
        aim[2] = u->pos[2] + uv[2] * 10 - pos[2];
        d = pfNormalizeVec3(aim);
        aim[2] += d * 0.01f;
        pfNormalizeVec3(aim);
        pfScaleVec3(aim, 800, aim);
        ammoFire(gEnemyBank, pos, aim, enemy_gun_hit_cb, e);
        e->last_fire = t;
    }
}

static int enemy_outside(Enemy *e)
{
    float dx = e->pos[0] - e->cx, dy = e->pos[1] - e->cy;
    return dx * dx + dy * dy > e->size2;
}

static int enemy_well_inside(Enemy *e)
{
    float dx = e->pos[0] - e->cx, dy = e->pos[1] - e->cy;
    return dx * dx + dy * dy < 0.9f * e->size2;
}

static int user_in_region(Enemy *e, const float *u)
{
    float dx = u[0] - e->cx, dy = u[1] - e->cy;
    return dx * dx + dy * dy <= e->size2 && e->minalt <= u[2] && u[2] <= e->maxalt;
}

static float wrap180(float a)
{
    if (a > 180) a -= 360;
    else if (a < -180) a += 360;
    return a;
}

/* turns toward a heading difference dh, banking up to 60 degrees */
static void enemy_steer(Enemy *e, float dh, float f30)
{
    float c = dh, target;
    if (c > e->turn) c = e->turn;
    if (c < -e->turn) c = -e->turn;
    e->pos[3] += c * fabsf(dh / 180) * f30;
    target = -60 * dh / 180;
    if (gDt90 * 0.4f < e->pos[5] - target)
        e->pos[5] -= 0.4f * gDt90;
    else if (gDt90 * 0.4f < target - e->pos[5])
        e->pos[5] += 0.4f * gDt90;
    else
        e->pos[5] = target;
}

static void enemy_update(Enemy *e, double t, float dt)
{
    Plane *u = gUserPlane;
    pfMatrix m;
    pfCoord c;
    pfVec3 nose, to;
    float f30 = dt * 30, d2;
    if (!e->active)
        return;
    enemy_gun_update(e, t);
    pfSetVec3(c.xyz, e->pos[0], e->pos[1], e->pos[2]);
    pfSetVec3(c.hpr, e->pos[3], e->pos[4], e->pos[5]);
    pfMakeCoordMat(m, &c);
    pfSetVec3(nose, m[1][0], m[1][1], m[1][2]);
    pfScaleVec3(e->vel, 40, nose);
    e->pos[0] += e->vel[0] * dt;
    e->pos[1] += e->vel[1] * dt;
    e->pos[2] += e->vel[2] * dt;

    switch (e->state) {
    case 0:  /* cruise */
        if (e->maxalt < e->pos[2]) e->state = 4;
        else if (e->pos[2] < e->minalt) e->state = 3;
        else if (enemy_outside(e)) e->state = 2;
        else if (user_in_region(e, u->pos)) e->state = 1;
        break;
    case 1:  /* attack */
        if (e->maxalt < e->pos[2]) e->state = 4;
        else if (e->pos[2] < e->minalt) e->state = 3;
        else if (enemy_outside(e)) e->state = 2;
        else {
            float dh, dp, cp, h, p;
            to[0] = u->pos[0] - e->pos[0];
            to[1] = u->pos[1] - e->pos[1];
            to[2] = u->pos[2] - e->pos[2];
            if (pfNormalizeVec3(to) < 100) {
                e->state = 6;
                break;
            }
            h = -pfArcTan2(to[0], to[1]);
            p = pfArcTan2(to[2], sqrtf(to[0] * to[0] + to[1] * to[1]));
            dh = wrap180(h - e->pos[3]);
            enemy_steer(e, dh, f30);
            dp = wrap180(p - e->pos[4]);
            cp = dp;
            if (gDt90 * 0.5f < cp) cp = gDt90 * 0.5f;
            else if (cp < gDt90 * -0.5f) cp = gDt90 * -0.5f;
            e->pos[4] += cp * fabsf(dp / 180);
            if (e->pos[4] > 25) e->pos[4] = 25;
            if (e->pos[4] < -25) e->pos[4] = -25;
        }
        break;
    case 2:  /* back to the region */
        if (enemy_well_inside(e)) {
            e->state = 0;
        } else {
            float dh = wrap180(-pfArcTan2(e->cx - e->pos[0], e->cy - e->pos[1]) - e->pos[3]);
            if (fabsf(dh) > 5)
                enemy_steer(e, dh, f30);
        }
        break;
    case 6:  /* break off after a pass */
        if (e->maxalt < e->pos[2]) e->state = 4;
        else if (e->pos[2] < e->minalt) e->state = 3;
        else if (enemy_outside(e)) e->state = 2;
        else {
            float dx = u->pos[0] - e->pos[0], dy = u->pos[1] - e->pos[1], dz = u->pos[2] - e->pos[2];
            if (dx * dx + dy * dy + dz * dz > 360000) {
                e->state = 1;
            } else {
                e->pos[5] += (float)(0.4f * gDt90 * (1 - gRand) * 0.1);
                if (e->pos[5] > 60) e->pos[5] = 60;
                if (e->pos[5] < -60) e->pos[5] = -60;
                e->pos[3] -= e->turn * (e->pos[5] / 60) * f30;
            }
        }
        break;
    case 3:  /* climb */
        if (enemy_outside(e)) e->state = 2;
        else if (!(e->pos[2] < e->minalt)) e->state = 0;
        else {
            e->pos[4] += 0.5f * gDt90;
            if (e->pos[4] > 25) e->pos[4] = 25;
        }
        break;
    case 4:  /* descend */
        if (enemy_outside(e)) e->state = 2;
        else if (!(e->maxalt < e->pos[2])) e->state = 0;
        else {
            e->pos[4] -= 0.5f * gDt90;
            if (e->pos[4] < -25) e->pos[4] = -25;
        }
        break;
    case 5:  /* shot down */
        e->pos[4] -= 0.5f * gDt90;
        if (e->pos[4] < -25) e->pos[4] = -25;
        if (e->pos[2] < e->minalt) {
            e->hits += e->maxhits;
            enemy_explode(e);
            return;
        }
        break;
    }
    vgObjPosVec(e->obj, e->pos);

    /* fires when the user is less than 750 m away and within 5 degrees of the nose */
    to[0] = u->pos[0] - e->pos[0];
    to[1] = u->pos[1] - e->pos[1];
    to[2] = u->pos[2] - e->pos[2];
    d2 = pfDotVec3(to, to);
    if (d2 < 562500 && !u->dead) {
        float d;
        pfNormalizeVec3(to);
        d = pfDotVec3(to, nose);
        if (d > 1) d = 1;
        if (d < -1) d = -1;
        if (pfArcCos(d) < 5) {
            e->trigger = 1;
            e->trig_time = t;
        }
    }
}

/* --- flak cannons (vgCannon) ----------------------------------------------------- */

typedef struct Cannon {
    float x, y, z;
    double next;
    Ammo *shell;
    float maxdist2;
} Cannon;

static void cannon_update(Cannon *c, double t, float dt)
{
    Plane *u = gUserPlane;
    float dx = u->pos[0] - c->x, dy = u->pos[1] - c->y, dz = u->pos[2] - c->z;
    float d2 = dx * dx + dy * dy + dz * dz;
    if (c->shell) {
        if (!ammoUpdateNoIsect(c->shell, dt, c->maxdist2)) {
            float ex = c->shell->pos[0] - u->pos[0], ey = c->shell->pos[1] - u->pos[1];
            float ez = c->shell->pos[2] - u->pos[2];
            ciCreateFlakExplosion(c->shell->pos);
            if (ex * ex + ey * ey + ez * ez < 625 && !u->dead) {
                planeHit(u, 1, 0);
                if (u->damage >= 26)
                    planeDiveAndExplode(u);
            }
            ammoKill(c->shell);
            c->shell = NULL;
        }
        return;
    }
    if (d2 < 1000000 && c->next < t && !u->dead) {
        pfVec3 pos, aim;
        float d = sqrtf(d2);
        pfSetVec3(pos, c->x, c->y, c->z);
        aim[0] = dx + (float)(20 - ciRand() * 40);
        aim[1] = dy + (float)(20 - ciRand() * 40);
        aim[2] = dz + d * 0.2f;
        pfNormalizeVec3(aim);
        pfScaleVec3(aim, 250, aim);
        c->shell = ammoFire(gFlakBank, pos, aim, NULL, NULL);
        c->maxdist2 = 0.85f * d2;
        c->next = t + ciRand() * 5;
    }
}

/* --- battle regions (vgBattleRegion) --------------------------------------------- */

#define MAX_REGION_PLANES 8

typedef struct Region {
    char name[32];
    float x, y, alt, maxalt, size, act2;
    int active;
    int nplanes;
    Enemy *planes[MAX_REGION_PLANES];
    Balloon *balloons[4];
    int nballoons;
    Cannon cannon;
} Region;

static Region regions[5];
static int nregions;

static void region_add_balloon(Region *r, int type)
{
    float x = (float)(r->x + (0.5 - ciRand()) * 1000);
    float y = (float)(r->y + (0.5 - ciRand()) * 1000);
    float z = (float)(ciRand() * 100 + r->alt);
    r->balloons[r->nballoons++] = balloon_new(type, x, y, z);
}

static void region_add_planes(Region *r, int n)
{
    float formation[6];
    int i;
    for (i = 0; i < n / 2 && r->nplanes + 2 <= MAX_REGION_PLANES; i++) {
        float rnd = (float)ciRand();
        r->planes[r->nplanes++] = enemy_new(r->x, r->y, r->alt, r->maxalt, r->size * r->size, 1, formation, rnd);
        formation[0] += 20;
        formation[1] += 20;
        formation[2] += 20 * rnd;
        r->planes[r->nplanes++] = enemy_new(r->x, r->y, r->alt, r->maxalt, r->size * r->size, 0, formation, rnd);
    }
    if ((n & 1) && r->nplanes < MAX_REGION_PLANES)
        r->planes[r->nplanes++] = enemy_new(r->x, r->y, r->alt, r->maxalt, r->size * r->size, 1, formation, (float)ciRand());
}

static Region *region_new(const char *name, float x, float y, float alt, float maxalt, int nplanes)
{
    Region *r = &regions[nregions++];
    memset(r, 0, sizeof(*r));
    strncpy(r->name, name, sizeof(r->name) - 1);
    r->x = x;
    r->y = y;
    r->alt = alt;
    r->maxalt = maxalt;
    r->size = 1000;
    r->act2 = 5e6f;
    region_add_balloon(r, 0);
    region_add_balloon(r, 2);
    region_add_balloon(r, 1);
    region_add_balloon(r, 1);
    region_add_planes(r, nplanes);
    r->cannon.x = x;
    r->cannon.y = y;
    r->cannon.z = alt >= 150 ? alt - 150 : 0;
    r->cannon.next = 0;
    return r;
}

static void region_destroy(Region *r)
{
    int i;
    for (i = 0; i < r->nplanes; i++) {
        enemy_remove(r->planes[i]);
        vgDelObj(r->planes[i]->obj);
        free(r->planes[i]);
    }
    for (i = 0; i < r->nballoons; i++) {
        balloon_disable(r->balloons[i]);
        vgDelObj(r->balloons[i]->obj);
        free(r->balloons[i]);
    }
    if (r->cannon.shell)
        ammoKill(r->cannon.shell);
}

static int region_update(Region *r, const float *u, double t, float dt)
{
    float dx = u[0] - r->x, dy = u[1] - r->y;
    int i;
    if (r->act2 < dx * dx + dy * dy) {
        if (r->active) {
            for (i = 0; i < r->nballoons; i++)
                balloon_disable(r->balloons[i]);
            for (i = 0; i < r->nplanes; i++) {
                enemy_remove(r->planes[i]);
                if (r->planes[i]->smoke)
                    pfSwitchVal(r->planes[i]->smoke, PFSWITCH_OFF);
            }
            r->active = 0;
        }
        return 0;
    }
    if (!r->active) {
        for (i = 0; i < r->nballoons; i++)
            if (r->balloons[i]->hits < 5)
                balloon_enable(r->balloons[i]);
        for (i = 0; i < r->nplanes; i++) {
            Enemy *e = r->planes[i];
            if ((float)e->maxhits < e->hits)
                enemy_reset(e);
            else
                enemy_add(e);
        }
        r->active = 1;
    }
    for (i = 0; i < r->nballoons; i++)
        balloon_update(r->balloons[i], dt);
    for (i = 0; i < r->nplanes; i++)
        enemy_update(r->planes[i], t, dt);
    cannon_update(&r->cannon, t, dt);
    return 1;
}

/* --- ships (vgShip) --------------------------------------------------------------- */

typedef struct Ship {
    vgObject *obj;
    vgPath *path;
    float pos[6], init[6];
    int moving, hits;
} Ship;

static Ship ships[6];
static int nships;

static void ship_new(const char *name, const char *pathfile, const float *pos, unsigned int cls)
{
    Ship *s = &ships[nships++];
    memset(s, 0, sizeof(*s));
    s->obj = vgFindObj(name);
    if (!s->obj) {
        nships--;
        return;
    }
    vgObjModel(s->obj);
    vgObjClass(s->obj, cls);
    s->obj->user = s;
    vgAddSceneObj(gMainScene, s->obj);
    if (pathfile) {
        s->path = vgNewPath();
        vgAddPathFile(s->path, pathfile);
        vgFollowPath(s->path, 0.1f, gNow, s->pos, s->pos + 3);
    } else {
        memcpy(s->pos, pos, sizeof(s->pos));
    }
    memcpy(s->init, s->pos, sizeof(s->init));
    vgObjPosVec(s->obj, s->pos);
    s->moving = 1;
}

static void ship_update(Ship *s, double t, float dt)
{
    float f30 = dt * 30;
    if (s->moving == 1) {
        if (s->path) {
            vgFollowPath(s->path, dt, t, s->pos, s->pos + 3);
            vgObjPosVec(s->obj, s->pos);
        }
    } else if (s->moving == 0) {
        /* sinking */
        s->pos[4] += 0.01f * f30;
        if (s->pos[4] > 60) s->pos[4] = 60;
        s->pos[5] += 0.01f * f30;
        if (s->pos[5] > 30) s->pos[5] = 30;
        s->pos[2] -= 0.001f * f30;
        if (s->pos[2] < -20)
            s->moving = -1;
        vgObjPosVec(s->obj, s->pos);
    }
}

static void ship_explode(Ship *s)
{
    ciCreateAirExplosion(s->pos);
    s->moving = 0;
}

/* --- bridge (vgBridge and vgBridgeSection) ----------------------------------------- */

typedef struct Section {
    vgObject *obj;
    int id, hits, destroyed;
} Section;

static struct {
    int any;
    Section sec[8];
} bridge;

/* ranges of path distances of the train on each section, and the drop heights */
static const float bridge_range[8][2] = {
    { 4067, 4140 }, { 3987, 4067 }, { 3886, 3987 }, { 3792, 3886 },
    { 3704, 3792 }, { 3616, 3704 }, { 3536, 3616 }, { 3479, 3536 }
};
static const float bridge_drop[8] = { 50, 45, 39, 35, 30, 19.32f, 23.95f, 20.82f };

static void bridge_create(void)
{
    int i;
    char name[16];
    memset(&bridge, 0, sizeof(bridge));
    for (i = 0; i < 8; i++) {
        Section *s = &bridge.sec[i];
        sprintf(name, "Bridge%d", i + 1);
        s->obj = vgFindObj(name);
        s->id = 20 + i;
        if (!s->obj)
            continue;
        vgObjModel(s->obj);
        vgObjClass(s->obj, CL_BRIDGE);
        s->obj->user = s;
        vgDISAppearance(s->obj, 24, 0);
        vgAddSceneObj(gMainScene, s->obj);
    }
}

static void bridge_reset(void)
{
    int i;
    bridge.any = 0;
    for (i = 0; i < 8; i++) {
        bridge.sec[i].hits = 0;
        bridge.sec[i].destroyed = 0;
        if (bridge.sec[i].obj)
            vgDISAppearance(bridge.sec[i].obj, 24, 0);
    }
}

static void section_explode(Section *s, const float *p)
{
    ciCreateAirExplosion(p);
    vgDISAppearance(s->obj, 24, 24);
    s->destroyed = 1;
    bridge.any = 1;
}

static float bridge_drop_at(float d)
{
    int i;
    for (i = 0; i < 8; i++)
        if (bridge.sec[i].destroyed && bridge_range[i][0] <= d && d < bridge_range[i][1])
            return bridge_drop[i];
    return -1;
}

/* --- bunkers (vgBunker) ----------------------------------------------------------- */

typedef struct Bunker {
    vgObject *obj;
    float dist;              /* distance of the train path where it blocks */
    int hits, alive;
} Bunker;

static Bunker bunkers[4];

static void bunker_create(int i, const char *name, float dist)
{
    Bunker *b = &bunkers[i];
    b->obj = vgFindObj(name);
    b->dist = dist;
    b->hits = 0;
    b->alive = 1;
    if (!b->obj)
        return;
    vgObjModel(b->obj);
    vgObjClass(b->obj, CL_BUNKER);
    b->obj->user = b;
    vgDISAppearance(b->obj, 24, 0);
    vgAddSceneObj(gMainScene, b->obj);
}

static void bunker_reset(Bunker *b)
{
    b->hits = 0;
    b->alive = 1;
    if (b->obj)
        vgDISAppearance(b->obj, 24, 0);
}

static void bunker_explode(Bunker *b, const float *p)
{
    ciCreateBigGroundExplosion(p);
    vgDISAppearance(b->obj, 24, 24);
}

/* --- train (vgTrain and vgTrainCar) ---------------------------------------------- */

typedef struct Car {
    vgObject *obj;
    vgPath *path;
    unsigned int cls;
    int id;
    float offset;
    float pos[6];
    int alive, exploded, falling, hits;
    float drop;
} Car;

static struct {
    vgPath *path;
    Car cars[8];
    int ncars;
} train;

static void car_place(Car *c)
{
    vgObjPosVec(c->obj, c->pos);
}

static void car_reset(Car *c)
{
    c->path->cur = NULL;
    c->path->speed = 1;
    c->path->target = c->path->accel = 0;
    c->path->delay_end = 0;
    c->path->segdist = c->path->total = 0;
    vgFollowPath(c->path, c->offset, gNow, c->pos, c->pos + 3);
    c->alive = 1;
    c->exploded = 0;
    c->falling = 0;
    c->hits = 0;
    vgDISAppearance(c->obj, 0x400018, 0x400000);
    vgAddSceneObj(gMainScene, c->obj);
    car_place(c);
}

static void car_new(const char *name, unsigned int cls, float offset, int id)
{
    Car *c = &train.cars[train.ncars];
    memset(c, 0, sizeof(*c));
    c->obj = vgFindObj(name);
    if (!c->obj)
        return;
    train.ncars++;
    vgObjModel(c->obj);
    vgObjClass(c->obj, cls);
    c->obj->user = c;
    c->cls = cls;
    c->id = id;
    c->offset = offset;
    c->path = vgSharePath(train.path);
    car_reset(c);
}

static void car_explode(Car *c)
{
    c->exploded = 1;
    ciCreateBigGroundExplosion(c->pos);
    vgDISAppearance(c->obj, 24, 24);
}

static void car_update(Car *c, double t, float dt)
{
    int i;
    if (!c->alive)
        return;
    if (!c->falling) {
        float d = vgFollowPath(c->path, dt, t, c->pos, c->pos + 3);
        if (bridge.any) {
            c->drop = bridge_drop_at(d);
            if (c->drop > 0)
                c->falling = 1;
        }
        /* the blockades stop the cars of the middle of the train */
        if (c->id >= 12 && c->id <= 16 && !c->exploded)
            for (i = 0; i < 4; i++) {
                Bunker *b = &bunkers[i];
                if (b->alive && b->hits < 11 && fabsf(b->dist - d) < 2) {
                    car_explode(c);
                    b->alive--;
                }
            }
        car_place(c);
    } else {
        pfCoord co;
        pfMatrix m;
        float sp = c->path->speed;
        if (c->id == 10)
            ciTetherTrainBack();
        pfSetVec3(co.xyz, 0, 0, 0);
        pfSetVec3(co.hpr, c->pos[3], c->pos[4], c->pos[5]);
        pfMakeCoordMat(m, &co);
        c->pos[0] += m[1][0] * sp * dt;
        c->pos[1] += m[1][1] * sp * dt;
        c->pos[2] += m[1][2] * sp * dt;
        c->pos[4] -= 0.5f * dt * 30;
        if (c->pos[4] < -37.5f)
            c->pos[4] = -37.5f;
        car_place(c);
        if (c->pos[2] <= c->drop) {
            if (!c->exploded) {
                int n = c->id == 17 ? 50 : 100;
                planeAddScore(gUserPlane, gUserPlane->side ? n : -n);
            }
            ciCreateBigGroundExplosion(c->pos);
            c->alive = 0;
            vgRemSceneObj(gMainScene, c->obj);
        }
    }
}

static void train_create(void)
{
    float off = -1.77026987f;
    memset(&train, 0, sizeof(train));
    train.path = vgNewPath();
    vgAddPathFile(train.path, "trainPath");
    car_new("Engine", CL_ENGINE, 5.0f, 10);
    car_new("CoalCar", CL_COALCAR, off, 11);
    off -= 6.2625f;
    car_new("Car1", CL_CAR, off, 12);
    off -= 8.78947353f;
    car_new("Car2", CL_CAR, off, 13);
    off -= 8.78947353f;
    car_new("Car3", CL_CAR, off, 14);
    off -= 7.47761202f;
    car_new("ArtilCar", CL_ARTILCAR, off, 16);
    off -= 7.15714264f;
    car_new("Car4", CL_CAR, off, 15);
    off -= 8.34999943f;
    car_new("Caboose", CL_CABOOSE, off, 17);
}

static void train_location(float *x, float *y, float *h)
{
    Car *e = &train.cars[0];
    *x = e->pos[0];
    *y = e->pos[1];
    *h = e->pos[3];
}

/* --- blimp (vgBlimp) ---------------------------------------------------------------- */

static struct {
    vgObject *obj;
    int hits, alive;
    float pos[6];
} blimp;

static void blimp_create(void)
{
    memset(&blimp, 0, sizeof(blimp));
    blimp.obj = vgFindObj("Blimp");
    if (!blimp.obj)
        return;
    vgObjModel(blimp.obj);
    vgObjClass(blimp.obj, CL_BLIMP);
    blimp.obj->user = &blimp;
    vgAddSceneObj(gMainScene, blimp.obj);
    blimp.alive = 1;
}

static void blimp_update(void)
{
    if (!blimp.alive)
        return;
    train_location(&blimp.pos[0], &blimp.pos[1], &blimp.pos[3]);
    blimp.pos[2] = 250;
    blimp.pos[4] = blimp.pos[5] = 0;
    vgObjPosVec(blimp.obj, blimp.pos);
}

static void blimp_explode(void)
{
    int i;
    for (i = 0; i < 5; i++) {
        float p[3];
        p[0] = blimp.pos[0] + (float)((ciRand() - 0.5) * 40);
        p[1] = blimp.pos[1] + (float)((ciRand() - 0.5) * 40);
        p[2] = blimp.pos[2] + (float)((ciRand() - 0.5) * 20);
        ciCreateAirExplosion(p);
    }
    blimp.alive = 0;
    vgRemSceneObj(gMainScene, blimp.obj);
}

/* --- creation of the world ------------------------------------------------------- */

static int world_created;

void worldConfigure(void)
{
    int i;
    static const char *anims[] = { "windsck", "flag01", "flag02", "flag03", "flag04", "flag05", NULL };
    char name[16];
    terrain.obj = vgFindObj("Terrain");
    if (!terrain.obj) {
        fprintf(stderr, " Can't find <Terrain> object.\n");
        exit(1);
    }
    vgObjModel(terrain.obj);
    terrain_classes();
    for (i = 0; anims[i]; i++)
        add_named_seqs(terrain.obj, anims[i]);
    for (i = 1; i <= 10; i++) {
        sprintf(name, "falls%02d", i);
        add_named_seqs(terrain.obj, name);
    }
    worldTerrainInitDoors();
}

void worldCreate(Plane *user, int nenemies)
{
    static const float ship3[6] = { -11, -2890, 0, 0, 0, 0 };
    static const float ship6[6] = { -1564, -184, 0, 0, 0, 0 };
    (void)user;
    nregions = 0;
    region_new("German1", 3500, -1700, 150, 600, nenemies);
    region_new("German2", 345, -2258, 150, 600, nenemies);
    region_new("British1", -1000, -500, 100, 600, nenemies);
    region_new("British2", -2157, 2218, 100, 600, nenemies);
    region_new("British3", 924, 2065, 100, 600, nenemies);
    nships = 0;
    ship_new("Ship1", "AlliedShipPath1", NULL, CL_ALLIED_SHIP);
    ship_new("Ship2", "AlliedShipPath2", NULL, CL_ALLIED_SHIP);
    ship_new("Ship3", NULL, ship3, CL_ALLIED_SHIP);
    ship_new("Ship4", "AxesShipPath1", NULL, CL_AXIS_SHIP);
    ship_new("Ship5", "AxesShipPath2", NULL, CL_AXIS_SHIP);
    ship_new("Ship6", NULL, ship6, CL_AXIS_SHIP);
    bridge_create();
    train_create();
    blimp_create();
    bunker_create(0, "Bunker1", 6127);
    bunker_create(1, "Bunker2", 7692);
    bunker_create(2, "Bunker3", 8751);
    bunker_create(3, "Bunker4", 10269);
    world_created = 1;
}

void worldDestroy(void)
{
    int i;
    if (!world_created)
        return;
    for (i = 0; i < nregions; i++)
        region_destroy(&regions[i]);
    nregions = 0;
    for (i = 0; i < nships; i++)
        vgRemSceneObj(gMainScene, ships[i].obj);
    nships = 0;
    for (i = 0; i < 8; i++)
        if (bridge.sec[i].obj)
            vgRemSceneObj(gMainScene, bridge.sec[i].obj);
    for (i = 0; i < train.ncars; i++)
        vgRemSceneObj(gMainScene, train.cars[i].obj);
    train.ncars = 0;
    if (blimp.obj)
        vgRemSceneObj(gMainScene, blimp.obj);
    for (i = 0; i < 4; i++)
        if (bunkers[i].obj)
            vgRemSceneObj(gMainScene, bunkers[i].obj);
    for (i = 0; i < gEnemyBank->n; i++)
        ammoKill(&gEnemyBank->ammo[i]);
    for (i = 0; i < gFlakBank->n; i++)
        ammoKill(&gFlakBank->ammo[i]);
    world_created = 0;
}

/* mardi::reset: the bunkers, terrain, bridge and train come back */
void worldReset(void)
{
    int i;
    for (i = 0; i < 4; i++)
        bunker_reset(&bunkers[i]);
    worldTerrainInitDoors();
    worldTerrainUndamage();
    bridge_reset();
    for (i = 0; i < train.ncars; i++)
        car_reset(&train.cars[i]);
    blimp.hits = 0;
    if (!blimp.alive && blimp.obj) {
        blimp.alive = 1;
        vgAddSceneObj(gMainScene, blimp.obj);
    }
}

/* every vehicle back to its initial position (vgMotResetPos of all motions) */
void worldResetMotion(void)
{
    int i, k;
    for (i = 0; i < nregions; i++) {
        Region *r = &regions[i];
        for (k = 0; k < r->nplanes; k++) {
            Enemy *e = r->planes[k];
            memcpy(e->pos, e->init, sizeof(e->pos));
            e->state = 0;
            vgObjPosVec(e->obj, e->pos);
        }
    }
    for (i = 0; i < nships; i++) {
        Ship *s = &ships[i];
        memcpy(s->pos, s->init, sizeof(s->pos));
        s->hits = 0;
        s->moving = 1;
        if (s->path) {
            s->path->cur = NULL;
            s->path->speed = 1;
            s->path->segdist = s->path->total = 0;
        }
        vgObjPosVec(s->obj, s->pos);
    }
}

void worldUpdate(double t, float dt)
{
    int i;
    if (terrain.doors_active)
        terrain_update_doors();
    if (!world_created)
        return;
    for (i = 0; i < gEnemyBank->n; i++)
        if (gEnemyBank->ammo[i].active)
            ammoUpdate(&gEnemyBank->ammo[i], dt);
    for (i = 0; i < nregions; i++)
        region_update(&regions[i], gUserPlane->pos, t, dt);
    for (i = 0; i < nships; i++)
        ship_update(&ships[i], t, dt);
    for (i = 0; i < train.ncars; i++)
        car_update(&train.cars[i], t, dt);
    blimp_update();
}

void worldMapPositions(float *tr, int *nplanes, float planes[][4], int *nfriends, float friends[][4])
{
    int i, k;
    train_location(&tr[0], &tr[1], &tr[2]);
    *nplanes = 0;
    *nfriends = 0;
    for (i = 0; i < nregions; i++) {
        Region *r = &regions[i];
        if (!r->active)
            continue;
        for (k = 0; k < r->nplanes && *nplanes < 20; k++) {
            Enemy *e = r->planes[k];
            if (!e->active)
                continue;
            planes[*nplanes][0] = e->pos[0];
            planes[*nplanes][1] = e->pos[1];
            planes[*nplanes][2] = e->pos[2];
            planes[*nplanes][3] = e->pos[3];
            (*nplanes)++;
        }
    }
}

/* --- weapons effects (mardi::detonate and mardi::collision) ------------------------ */

static int side_score(int german_value)
{
    return gUserPlane->side ? -german_value : german_value;
}

/* kind 1: bullet, 2: bomb */
void worldDetonate(int kind, const float *pos, vgHit *h)
{
    Plane *u = gUserPlane;
    unsigned int cls = h->cls;
    int inc = kind == 1 ? 1 : 100;
    switch (cls) {
    case CL_CASTLE:
    case CL_TREES:
        break;
    case CL_CRATES:
    case CL_ARTIL:
    case CL_HOUSE:
        {
            pfNode *sw;
            planeAddScore(u, 100);
            ciCreateBigGroundExplosion(pos);
            if ((sw = ciGetSwitchNode(h)) != NULL)
                ciSwitchToDamaged(sw);
        }
        break;
    case CL_BRIDGE:
        {
            Section *s = (Section *)h->obj->user;
            if (!s || h->obj == terrain.obj)
                break;
            s->hits += inc;
            if (s->hits >= 26 && !s->destroyed) {
                planeAddScore(u, kind == 1 ? side_score(-200) : 200);
                section_explode(s, pos);
            }
        }
        break;
    case CL_BLIMP:
        if (kind == 1 && blimp.alive && ++blimp.hits >= 21) {
            planeAddScore(u, side_score(-50));
            blimp_explode();
        }
        break;
    case CL_AMMO_BALLOON:
    case CL_FUEL_BALLOON:
    case CL_HEALTH_BALLOON:
        if (kind == 1) {
            Balloon *b = (Balloon *)h->obj->user;
            planeAddScore(u, 10);
            if (++b->hits >= 5)
                balloon_explode(b);
        }
        break;
    case CL_ALLIED_SHIP:
    case CL_AXIS_SHIP:
        {
            Ship *s = (Ship *)h->obj->user;
            s->hits += inc;
            if (s->hits >= 11 && s->moving == 1) {
                planeAddScore(u, side_score(cls == CL_ALLIED_SHIP ? 50 : -50));
                ship_explode(s);
            }
        }
        break;
    case CL_USER:
        if (gDifficulty && kind == 1) {
            planeHit(u, 1, 1);
            if (u->damage >= 26)
                planeDiveAndExplode(u);
        }
        break;
    case CL_GERMAN_PLANE:
    case CL_ALLIED_PLANE:
        if (kind == 1) {
            Enemy *e = (Enemy *)h->obj->user;
            e->hits += 1;
            if (0.75f * e->maxhits < e->hits && e->state != 5)
                enemy_dive(e);
            if ((float)e->maxhits < e->hits || e->state == 5) {
                if (e->active && e->state != 5) {
                    planeAddScore(u, side_score(cls == CL_ALLIED_PLANE ? 200 : -200));
                    enemy_explode(e);
                } else if (e->active && (float)e->maxhits < e->hits) {
                    planeAddScore(u, side_score(cls == CL_ALLIED_PLANE ? 200 : -200));
                    enemy_explode(e);
                }
            }
        }
        break;
    case CL_BUNKER:
        {
            Bunker *b = (Bunker *)h->obj->user;
            int was = b->hits >= 11;
            b->hits += inc;
            if (b->hits >= 11 && !was) {
                planeAddScore(u, side_score(200));
                bunker_explode(b, pos);
            }
        }
        break;
    case CL_ENGINE:
    case CL_COALCAR:
    case CL_CAR:
    case CL_ARTILCAR:
    case CL_CABOOSE:
        {
            Car *c = (Car *)h->obj->user;
            int was = c->hits >= 11;
            c->hits += inc;
            if (c->hits >= 11 && !was) {
                int n = cls == CL_CABOOSE ? 50 : 100;
                planeAddScore(u, side_score(-n));
                car_explode(c);
            }
        }
        break;
    default:
        if (kind == 1)
            ciCreateSmallGroundExplosion(pos);
        else
            ciCreateBigGroundExplosion(pos);
        break;
    }
}

void worldCollision(vgObject *o, unsigned int cls)
{
    Plane *u = gUserPlane;
    if (u->dead)
        return;
#ifdef CI_DEBUG
    fprintf(stderr, "collision with %s class %#x\n", o ? o->name : "?", cls);
#endif
    switch (cls) {
    case CL_WATER:
        planeSink(u);
        break;
    case CL_AMMO_BALLOON:
        balloon_explode((Balloon *)o->user);
        u->ammo += 100;
        u->bombs += 2;
        break;
    case CL_FUEL_BALLOON:
        balloon_explode((Balloon *)o->user);
        u->fuel += 1;
        if (u->fuel > 1)
            u->fuel = 1;
        break;
    case CL_HEALTH_BALLOON:
        balloon_explode((Balloon *)o->user);
        planeHit(u, -20, -1);
        break;
    default:
        planeExplode(u);
        break;
    }
}
