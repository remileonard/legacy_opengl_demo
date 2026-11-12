/*
 * flight.c - the flight of the F-18 demo
 *
 * Rebuilt from the functions of the IRIX executable named by the scripts
 * and from the ones they call: initFlight, updateFlight, planeCrash,
 * dispFlight, eyeFlight, watchEnemy, toggleWingman, noSpeeding,
 * restartFlight, toggleHud, plusRadar, minusRadar, readRoute, initEnemy,
 * updateEnemy, and the "wingman" views of EasyScene (esWingmanFollow,
 * esWingmanTether, esWingmanTether2).
 *
 * The demo moves its objects once per frame at the frame rate of the script
 * (60 Hz): the rates below are per frame, and the program calls the app
 * modules at that rate.
 *
 * EasyScene keeps the positions in a frame where Y is up: (x, up, z) is
 * (x, -y, z) of Performer... with y and z exchanged: es (x, y, z) =
 * Performer (x, -z, y).  The code below works in Performer coordinates; the
 * constants of the demo given in EasyScene coordinates are converted.
 *
 * The airplane of the player is flown with the mouse: its distance to the
 * middle of the window gives the roll rate (5 degrees per frame at the
 * edge) and the pitch rate (1 degree per frame), the right button speeds up
 * and the middle button slows down (1 m/s per frame, from 100 to 1000 m/s).
 * The pitch rate turns the airplane around its wings: pitch -= rate * cos
 * roll, heading -= rate * sin roll.
 *
 * The enemies replay recorded flights (N.route: per frame, the roll rate,
 * the pitch rate and the speed, 12 bytes, as the demo records the flight of
 * the player in rec.pos), and leave them to keep between 1000 and 3000 m
 * above the ground (roll level, climb or dive to 1250 m, level, roll back)
 * and inside the area of the terrain (roll 90 degrees and turn to the
 * origin).  The route files are not in the data of the demo: when they are
 * missing, routes are made up from turns, climbs and straight lines.
 *
 * Not rebuilt yet: the missiles (left button, Alt keys, C key), the gun
 * (space bar in flight), the explosions, the smoke, the sound, the HUD of
 * the cockpit (a .drt file) and the radar.
 */
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <GL/freeglut.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "escene.h"
#include "flight.h"

F18Mouse f18_mouse;

#define MAX_ENEMIES 5

/* the start of the enemies, in EasyScene coordinates: x, up, z, h, p, r */
static const float enemy_start[MAX_ENEMIES][6] = {
    { 0, 2000, 0, 0, 0, 0 },
    { -72954, 3000, -35111, 240, 0, 0 },
    { 26000, 2500, 61000, 30, 0, 0 },
    { -5000, 2100, -10000, 90, 0, 0 },
    { -1795, 2900, -40314.9f, -90, 0, 0 },
};

/* the area of the terrain where the enemies stay, EasyScene x and z */
#define AREA_XMAX 26003.0f
#define AREA_XMIN -72959.9f
#define AREA_ZMAX 61163.9f
#define AREA_ZMIN -35112.3f

typedef struct Enemy {
    esObject *obj;
    float *route;           /* roll rate, pitch rate, speed per frame */
    int nroute, idx;
    pfNode *sw;             /* the airplane and its wreck */
    int state;              /* 0 route, 1-4 back to 1250 m, 5-6 back to the area */
    int hit;                /* hit by a missile: falls */
    float fall[3];          /* where it was hit */
    int fall_frame;
    int dead;
    float saved_x, saved_h, saved_y;
    int dead_frame;
    float saved_roll;
    int dir;                /* -1 or 1 */
    float rand_angle, pitch_rate;
    float height;           /* above the ground */
    float ground;           /* elevation of the ground under it */
    float speed;
    int isect_period, isect_count;
} Enemy;

typedef struct Flight {
    esObject *camera, *plane;
    float speed;            /* m/s */
    float radar;            /* range of the radar */
    int target;             /* enemy locked, -1 none */
    int crashed;            /* frames since the crash, 0 flying */
    int nospeed;
    int hud_off;
    int cockpit;            /* the eye is in the airplane */
    char message[128];
    float height;           /* above the ground */
    int isect_on, isect_period, isect_count;
    Enemy enemy[MAX_ENEMIES];
    int nenemies;
    float enemy_speed;      /* shared by the enemies, as in the demo */
    long frame;
} Flight;

static Flight F;

/* --- the views (EasyScene wingman) ------------------------------------------- */

enum { WM_NONE = 0, WM_TETHER = 4, WM_TETHER2 = 5, WM_FOLLOW = 11 };

typedef struct Wingman {
    int mode;
    esObject *from, *to;    /* the eye, the object it looks at */
    float dist, angle, pitch, delay, up, side;
    /* follow: the positions of the object, delay * 30 frames ago */
    float hist[64][6];
    int nhist, wr, rd;
} Wingman;

static Wingman W;

static void wingman_mode(int mode)
{
    W.mode = mode;
    if (mode == WM_FOLLOW) {
        int i;
        W.nhist = (int)(W.delay * 30);
        if (W.nhist < 1) W.nhist = 1;
        if (W.nhist > 64) W.nhist = 64;
        for (i = 0; i < W.nhist && W.to; i++) {
            memcpy(W.hist[i], W.to->xyz, sizeof(pfVec3));
            memcpy(W.hist[i] + 3, W.to->hpr, sizeof(pfVec3));
        }
        W.wr = 0;
        W.rd = 1 % W.nhist;
    }
}

/* esSetWingmanValues(distance, angle, pitch, delay, up, side) */
static void wingman_values(float dist, float angle, float pitch, float delay, float up, float side)
{
    W.dist = dist;
    W.angle = angle;
    W.pitch = pitch;
    W.delay = delay;
    W.up = up;
    W.side = side;
}

static void wingman_update(const char *arg)
{
    esObject *a = W.from, *b = W.to;
    (void)arg;
    if (!a || !b || W.mode == WM_NONE)
        return;
    if (W.mode == WM_FOLLOW) {
        /* the eye takes the place of the object some frames ago */
        float *h = W.hist[W.wr], *r;
        memcpy(h, b->xyz, sizeof(pfVec3));
        memcpy(h + 3, b->hpr, sizeof(pfVec3));
        r = W.hist[W.rd];
        esObjectPos(a, r[0], r[1], r[2], r[3], r[4], r[5]);
        W.wr = (W.wr + 1) % W.nhist;
        W.rd = (W.rd + 1) % W.nhist;
    } else if (W.mode == WM_TETHER || W.mode == WM_TETHER2) {
        /* the eye around the object, at an angle fixed in the world
         * (tether) or from the heading of the object (tether 2) */
        float ang = W.mode == WM_TETHER2 ? b->hpr[0] + W.angle : W.angle;
        float x, y, z, dx, dy, dz;
        if (ang > 360) ang -= 360;
        x = b->xyz[0] + (float)cos(PF_DEG2RAD(ang)) * W.dist + W.side;
        y = b->xyz[1] + (float)sin(PF_DEG2RAD(ang)) * W.dist;
        z = b->xyz[2] + W.up;
        dx = b->xyz[0] - x;
        dy = b->xyz[1] - y;
        dz = b->xyz[2] - z;
        esObjectPos(a, x, y, z, (float)PF_RAD2DEG(-atan2(dx, dy)),
                    (float)PF_RAD2DEG(atan2(dz, sqrt(dx * dx + dy * dy))) + W.pitch, 0);
    }
}

/* the eye in the cockpit: (0, -0.29, -11.45) from the airplane in
 * EasyScene coordinates */
static void cockpit_update(void)
{
    pfMatrix m;
    pfCoord c;
    pfVec3 off = { 0, 11.45f, -0.29f }, p;
    pfCopyVec3(c.xyz, F.plane->xyz);
    pfSetVec3(c.hpr, F.plane->hpr[0], F.plane->hpr[1], -F.plane->hpr[2]);
    pfMakeCoordMat(m, &c);
    pfXformPt3(p, off, m);
    esObjectPos(F.camera, p[0], p[1], p[2], F.plane->hpr[0], F.plane->hpr[1], F.plane->hpr[2]);
}

static float diag(const esObject *o)
{
    return 2 * o->radius;
}

/* --- helpers ------------------------------------------------------------------ */

static void wrap(float *a)
{
    if (*a <= -360) *a += 360;
    else if (*a >= 360) *a -= 360;
}

/* the unit vector of a heading and a pitch (pfMakePolarSeg) */
static void polar(pfVec3 d, float h, float p)
{
    float sh, ch, sp, cp;
    pfSinCos(h, &sh, &ch);
    pfSinCos(p, &sp, &cp);
    pfSetVec3(d, -sh * cp, ch * cp, sp);
}

/* turns and moves an object as the demo does: roll -= roll rate,
 * pitch -= pitch rate * cos roll, heading -= pitch rate * sin roll */
static void fly(esObject *o, float roll_rate, float pitch_rate, float speed)
{
    float h = o->hpr[0], p = o->hpr[1], r = o->hpr[2], sr, cr;
    pfVec3 d;
    pfSinCos(r, &sr, &cr);
    r -= roll_rate;
    p -= pitch_rate * cr;
    h -= pitch_rate * sr;
    wrap(&h);
    wrap(&r);
    wrap(&p);
    polar(d, h, p);
    esObjectPos(o, o->xyz[0] + d[0] * speed / es.framerate, o->xyz[1] + d[1] * speed / es.framerate,
                o->xyz[2] + d[2] * speed / es.framerate, h, p, r);
}

static float height_above_ground(const esObject *o)
{
    float g = esGroundHeight(o->xyz[0], o->xyz[1], o->xyz[2]);
    return g < -1e29f ? 1e6f : o->xyz[2] - g;
}

static void message(const char *fmt, int n)
{
    sprintf(F.message, fmt, n);
}

/* --- the views of the keys ----------------------------------------------------- */

/* E: the views around the airplane, in turn */
static void eyeFlight(const char *arg)
{
    static int view;
    float d = diag(F.plane);
    (void)arg;
    if (F.crashed)
        view = 0;
    F.cockpit = 0;
    W.from = F.camera;
    W.to = F.plane;
    switch (view) {
    case 0:
        wingman_values(2 * d, -90, -3, 1, 5, 0);
        wingman_mode(WM_TETHER);
        view = 1;
        break;
    case 1:
        wingman_values(3 * d, -90, -3, 1, d / 5, 0);
        wingman_mode(WM_TETHER2);
        view = 2;
        break;
    case 2:
        wingman_values(d, -90, 0, 1, 0, 0);
        wingman_mode(WM_FOLLOW);
        view = 3;
        break;
    default:
        wingman_values(0.1f, -90, 0, 1, 0, 0);
        wingman_mode(WM_FOLLOW);
        view = 0;
        break;
    }
}

/* P: the view of each enemy in turn, then the cockpit */
static void watchEnemy(const char *arg)
{
    static int watched;
    (void)arg;
    if (watched == F.nenemies) {
        F.cockpit = 1;
        wingman_mode(WM_NONE);
        cockpit_update();
        watched = 0;
        F.message[0] = 0;
        return;
    }
    F.cockpit = 0;
    W.from = F.camera;
    W.to = F.enemy[watched].obj;
    wingman_values(5 * diag(W.to), -90, 0, 1, 2 * diag(W.to), 0);
    wingman_mode(WM_TETHER);
    watched++;
    message("Watching Enemy %d", watched);
}

/* F: the airplane behind an enemy chosen at random */
static void toggleWingman(const char *arg)
{
    Enemy *e;
    pfVec3 d;
    float dist;
    int n;
    (void)arg;
    if (F.nenemies == 0)
        return;
    n = rand() % F.nenemies;
    e = &F.enemy[n];
    if (e->dead || e->hit)
        return;
    polar(d, e->obj->hpr[0], 0);
    dist = -diag(e->obj) * 20;
    esObjectPos(F.plane, e->obj->xyz[0] + d[0] * dist, e->obj->xyz[1] + d[1] * dist, e->obj->xyz[2],
                e->obj->hpr[0], 0, 0);
    message("following Enemy %d", n + 1);
}

/* N: the enemies at their lowest speed */
static void noSpeeding(const char *arg)
{
    (void)arg;
    F.nospeed = !F.nospeed;
    sprintf(F.message, "No speeding mode %s", F.nospeed ? "on" : "off");
}

/* R: the head up display of the cockpit */
static void toggleHud(const char *arg)
{
    esObject *hud = esFindObject("f18hud");
    (void)arg;
    F.hud_off = !F.hud_off;
    if (hud && hud->node)
        pfNodeTravMask(hud->node, PFTRAV_DRAW, F.hud_off ? 0 : 0xffffffffU, PFTRAV_SELF, PF_SET);
}

static void plusRadar(const char *arg)
{
    (void)arg;
    F.radar += 1000;
    if (F.radar >= 50000) F.radar = 50000;
}

static void minusRadar(const char *arg)
{
    (void)arg;
    F.radar -= 1000;
    if (F.radar <= 1000) F.radar = 1000;
}

static void restart(void)
{
    F.crashed = 0;
    if (F.height < 20)
        esObjectPos(F.plane, F.plane->xyz[0], F.plane->xyz[1], F.plane->xyz[2] + 500,
                    F.plane->hpr[0], F.plane->hpr[1], F.plane->hpr[2]);
    F.isect_on = 1;
    F.isect_period = 20;
    F.message[0] = 0;
    eyeFlight(NULL);
    eyeFlight(NULL);
}

/* space: starts again after a crash (in flight, the gun of the demo is not
 * rebuilt yet) */
static void restartFlight(const char *arg)
{
    (void)arg;
    if (F.crashed)
        restart();
}

static void toggleBeeBox(const char *arg) { (void)arg; }
static void eyeStuff(const char *arg) { (void)arg; }

static float fade_distance;
static void setFadeDistance(const char *arg) { fade_distance = (float)atof(arg); }

/* --- the airplane of the player --------------------------------------------------- */

/* the ground under the airplane: the demo tests it every 20 frames, and
 * every frame below 500 m */
static void planeCrash(void)
{
    float h = height_above_ground(F.plane);
    if (F.height > 500 && h < 500)
        F.isect_period = 1;
    else if (F.height < 500 && h > 500)
        F.isect_period = 20;
    F.height = h;
    if (h < 5) {
        F.crashed = 1;
        esObjectPos(F.plane, F.plane->xyz[0], F.plane->xyz[1], F.plane->xyz[2] + 500,
                    F.plane->hpr[0], 0, 0);
        F.isect_on = 0;
        eyeFlight(NULL);
    }
}

/* the enemy in front of the airplane, within 2 degrees */
static void lock_target(void)
{
    pfVec3 d, v, c;
    int i;
    polar(d, F.plane->hpr[0], F.plane->hpr[1]);
    for (i = 0; i < F.nenemies; i++) {
        Enemy *e = &F.enemy[i];
        float t, off, dist;
        if (e->dead || e->hit)
            continue;
        pfSubVec3(v, e->obj->xyz, F.plane->xyz);
        t = pfDotVec3(v, d);
        pfScaleVec3(c, t, d);
        pfAddVec3(c, c, F.plane->xyz);
        off = pfDistancePt3(e->obj->xyz, c);
        dist = pfLengthVec3(v);
        if (off < dist && t > 0 && off / t <= 0.0349208f) {
            F.target = i;
            return;
        }
        F.target = -1;
    }
}

static void updateFlight(const char *arg)
{
    float roll_rate = 0, pitch_rate = 0;
    (void)arg;
    F.frame++;
    if (F.crashed) {
        /* "You just died...": 5 seconds before starting again */
        if (++F.crashed > es.framerate * 5)
            restart();
        return;
    }
    lock_target();
    if (f18_mouse.inside && f18_mouse.w > 0 && f18_mouse.h > 0) {
        float hw = f18_mouse.w / 2.0f, hh = f18_mouse.h / 2.0f;
        /* the IRIS GL y goes up */
        pitch_rate = (hh - f18_mouse.y) / hh * 1.0f;
        roll_rate = (f18_mouse.x - hw) / hw * 5.0f;
        if (f18_mouse.right)
            F.speed += 1;
        else if (f18_mouse.middle)
            F.speed -= 1;
    }
    if (F.speed >= 1000) F.speed = 1000;
    else if (F.speed <= 100) F.speed = 100;
    fly(F.plane, roll_rate, pitch_rate, F.speed);
    if (F.isect_on && ++F.isect_count >= F.isect_period) {
        F.isect_count = 0;
        planeCrash();
    }
}

/* the eye follows its object, after the objects moved */
static void updateEye(const char *arg)
{
    (void)arg;
    if (F.cockpit)
        cockpit_update();
    else
        wingman_update(NULL);
}

/* --- the enemies ------------------------------------------------------------------- */

/* a made-up route: straight lines, turns and climbs, flown with the
 * equations of the demo so that the wings and the nose come back level */
static void make_route(Enemy *e, int n)
{
    unsigned int seed = 12345u + 977u * (unsigned int)n;
    float r = 0, p = 0;
    int cap = 60 * 120, len = 0, seg;
#define RND() ((seed = seed * 1103515245u + 12345u) >> 16 & 0x7fff)
    e->route = (float *)malloc(sizeof(float) * 3 * cap);
    for (seg = 0; len < cap - 600; seg++) {
        int kind = seg % 2 == 0 ? 0 : 1 + RND() % 3, frames, i;
        float bank = (float)(30 + RND() % 40) * (RND() % 2 ? 1 : -1);
        float pull = 0.2f + (RND() % 30) / 100.0f;
        float speed = (float)(450 + RND() % 300);
        frames = kind == 0 ? 180 + RND() % 240 : 120 + RND() % 240;
        for (i = 0; i < frames && len < cap; i++, len++) {
            float rr, pr, cr = (float)cos(PF_DEG2RAD(r));
            if (kind == 0) {
                /* wings and nose level */
                rr = PF_MAX(-1.0f, PF_MIN(1.0f, r));
                pr = fabsf(cr) > 0.3f ? PF_MAX(-0.3f, PF_MIN(0.3f, p / cr * 0.05f)) : 0;
            } else if (kind == 3) {
                /* climb or dive a little, wings level */
                rr = PF_MAX(-1.0f, PF_MIN(1.0f, r));
                pr = i < frames / 2 ? -0.1f : 0.1f;
                if (bank < 0) pr = -pr;
            } else {
                /* bank and pull */
                rr = PF_MAX(-1.0f, PF_MIN(1.0f, r - bank));
                pr = fabsf(r - bank) < 5 ? -pull : 0;
            }
            e->route[3 * len] = rr;
            e->route[3 * len + 1] = pr;
            e->route[3 * len + 2] = speed;
            r -= rr;
            p -= pr * cr;
        }
    }
#undef RND
    e->nroute = len;
}

static int be_float(const unsigned char *b, float *f)
{
    unsigned int u = ((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16) | ((unsigned int)b[2] << 8) | b[3];
    memcpy(f, &u, 4);
    return 1;
}

static void read_route(Enemy *e, int n)
{
    char name[64], path[4096];
    FILE *fp;
    long size;
    unsigned char *buf;
    int i;
    sprintf(name, "%d.route", n);
    if (!pfFindFile(name, path, 4) || (fp = fopen(path, "rb")) == NULL) {
        make_route(e, n);
        return;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    e->nroute = (int)(size / 12);
    buf = (unsigned char *)malloc(size + 1);
    if (e->nroute <= 0 || fread(buf, 1, size, fp) != (size_t)size) {
        fclose(fp);
        free(buf);
        make_route(e, n);
        return;
    }
    fclose(fp);
    e->route = (float *)malloc(sizeof(float) * 3 * e->nroute);
    for (i = 0; i < 3 * e->nroute; i++)
        be_float(buf + 4 * i, &e->route[i]);
    free(buf);
}

static void initEnemy(int n)
{
    Enemy *e = &F.enemy[n];
    const float *s = enemy_start[n % MAX_ENEMIES];
    /* EasyScene (x, up, z) to Performer (x, -z, up) */
    esObjectPos(e->obj, s[0], -s[2], s[1], s[3], s[4], s[5]);
    e->idx = 0;
    e->state = 0;
    e->hit = 0;
    e->dead = 0;
    e->height = 1000;
    e->dir = 0;
    e->dead_frame = 0;
    e->isect_period = 10;
    e->isect_count = n;
    if (e->sw)
        pfSwitchVal(e->sw, 0);
}

/* the enemies, their wrecks (Broken%d) and their routes */
static void readRoute(void)
{
    int i;
    F.nenemies = 0;
    for (i = 0; i < MAX_ENEMIES; i++) {
        char name[32];
        Enemy *e = &F.enemy[F.nenemies];
        esObject *broken;
        sprintf(name, "Enemy%d", i);
        memset(e, 0, sizeof(*e));
        if ((e->obj = esFindObject(name)) == NULL || !e->obj->node)
            break;
        sprintf(name, "Broken%d", i);
        broken = esFindObject(name);
        if (broken && broken->model && e->obj->model) {
            /* a switch: the airplane, then its wreck */
            e->sw = pfNewSwitch();
            pfRemoveChild(e->obj->node, e->obj->model);
            pfAddChild(e->sw, e->obj->model);
            pfAddChild(e->sw, broken->model);
            pfAddChild(e->obj->node, e->sw);
            if (broken->node)
                pfRemoveChild(es.root, broken->node);
        }
        read_route(e, i);
        F.nenemies++;
        initEnemy(i);
    }
}

/* the ground under an enemy (the isect callback of the enemies) */
static void enemy_ground(Enemy *e, int n)
{
    float h = height_above_ground(e->obj);
    if (e->height > 400 && h < 400)
        e->isect_period = 1;
    else if (e->height < 400 && h > 400)
        e->isect_period = 10;
    e->height = h;
    e->ground = e->obj->xyz[2] - h;
    if (h < 5 && !e->dead) {
        e->dead = 1;
        e->speed = 0;
        message(e->hit ? "Enemy %d is a dead duck" : "Enemy %d crashed", n + 1);
        e->dead_frame = (int)F.frame;
        e->saved_x = e->obj->xyz[0];
        e->saved_h = e->obj->hpr[0];
        e->saved_y = e->obj->xyz[1];
        if (e->sw)
            pfSwitchVal(e->sw, 1);
    }
}

static void updateEnemy(const char *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < F.nenemies; i++) {
        Enemy *e = &F.enemy[i];
        esObject *o = e->obj;
        float h = o->hpr[0], p = o->hpr[1], r = o->hpr[2];
        float roll_rate = 0, pitch_rate = 0;
        float ex = o->xyz[0], ez = -o->xyz[1];   /* EasyScene x and z */
        if (e->dead) {
            if (F.frame - e->dead_frame < 30 * es.framerate)
                continue;
            /* "reincarnating enemy": at the altitude of its start, where
             * it crashed */
            initEnemy(i);
            esObjectPos(o, e->saved_x, e->saved_y, o->xyz[2], e->saved_h, o->hpr[1], o->hpr[2]);
            continue;
        }
        if (e->hit) {
            /* hit by a missile: falls along its heading, turning */
            int t = ++e->fall_frame;
            float v = e->speed / es.framerate, sh, ch, z;
            pfSinCos(h - 180, &sh, &ch);
            z = e->fall[2] + v * (float)sin(PF_DEG2RAD(p)) * t - 5.0f * t * t / (es.framerate * es.framerate);
            if (z - e->ground <= 0)
                z = e->ground + 2;
            esObjectPos(o, e->fall[0] + v * t * sh, e->fall[1] - v * t * ch, z, h, p, r + 5);
        } else {
            if (e->state == 2)
                e->pitch_rate = 0;
            switch (e->state) {
            case 0: {
                const float *rt = e->route + 3 * e->idx;
                roll_rate = rt[0];
                pitch_rate = rt[1];
                if (F.nospeed) {
                    F.enemy_speed = 100;
                } else {
                    F.enemy_speed = rt[2] / 2;
                    if (F.enemy_speed >= 1000) F.enemy_speed = 1000;
                    else if (F.enemy_speed <= 100) F.enemy_speed = 100;
                }
                e->idx = (e->idx + 1) % e->nroute;
                if (e->height <= 1000 || e->height >= 3000) {
                    e->saved_roll = r;
                    e->state = 1;
                    e->pitch_rate = 0;
                } else if (ex >= AREA_XMAX || ex <= AREA_XMIN || ez >= AREA_ZMAX || ez <= AREA_ZMIN) {
                    e->dir = rand() % 2 ? 1 : -1;
                    e->saved_roll = r;
                    e->state = 5;
                }
                break;
            }
            case 1: /* wings level */
                roll_rate = r > 0 ? 5 : -5;
                if (r <= 3 && r >= -3) {
                    r = 0;
                    roll_rate = 0;
                    e->state = 2;
                }
                break;
            case 2: /* to 1250 m above the ground */
                if (e->height >= 1250)
                    pitch_rate = p <= -35 ? 0 : (e->pitch_rate != 0 ? e->pitch_rate : 0.1f);
                else
                    pitch_rate = p >= 35 ? 0 : (e->pitch_rate != 0 ? -e->pitch_rate : -0.1f);
                if (e->height <= 1260 && e->height >= 1240) {
                    e->state = 3;
                    e->pitch_rate = 0;
                    pitch_rate = 0;
                }
                break;
            case 3: /* nose level */
                pitch_rate = p > 0 ? 1 : -1;
                if (p <= 2 && p >= -2) {
                    p = 0;
                    pitch_rate = 0;
                    e->state = 4;
                }
                break;
            case 4: /* the roll of the route again */
                roll_rate = e->saved_roll <= 0 ? 2 : -2;
                if (r <= e->saved_roll + 3 && r >= e->saved_roll - 3) {
                    r = e->saved_roll;
                    e->saved_roll = -1;
                    roll_rate = 0;
                    e->state = 0;
                }
                break;
            case 5: /* on the side */
                roll_rate = e->dir * 3.0f;
                if (e->dir == -1 && r <= 93 && r >= 87) {
                    r = 90;
                    roll_rate = 0;
                    e->state = 6;
                } else if (e->dir == 1 && r <= -87 && r >= -93) {
                    r = -90;
                    roll_rate = 0;
                    e->state = 6;
                }
                break;
            case 6: { /* turn to the origin */
                float to = (float)PF_RAD2DEG(atan2(ex, ez));
                pitch_rate = -1;
                if (h <= 0 && to >= 0) to -= 360;
                else if (h >= 0 && to <= 0) to += 360;
                if (h <= to && h >= to - 3) {
                    h = to;
                    pitch_rate = 0;
                    e->state = 4;
                }
                break;
            }
            }
            esObjectPos(o, o->xyz[0], o->xyz[1], o->xyz[2], h, p, r);
            e->speed = F.enemy_speed;
            fly(o, roll_rate, pitch_rate, F.enemy_speed);
        }
        if (++e->isect_count >= e->isect_period) {
            e->isect_count = 0;
            enemy_ground(e, i);
        }
    }
}

/* --- the display of the flight -------------------------------------------------- */

static void text(float x, float y, const char *s)
{
    glRasterPos2f(x, y);
    while (*s)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s++);
}

static void dispFlight(const char *arg)
{
    GLint vp[4];
    char buf[128];
    float w, h;
    (void)arg;
    /* on the main channel only */
    if (F.cockpit || es.current_channel != 0)
        return;
    glGetIntegerv(GL_VIEWPORT, vp);
    w = (float)vp[2];
    h = (float)vp[3];
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-w / 2, w / 2, -h / 2, h / 2);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    /* the cross of the gun sight, and a circle when an enemy is locked */
    glColor3f(1, 0, 0);
    if (F.target >= 0) {
        int i;
        glBegin(GL_LINE_LOOP);
        for (i = 0; i < 32; i++)
            glVertex2f(24 * (float)cos(i * PF_DEG2RAD(360.0 / 32)), 24 * (float)sin(i * PF_DEG2RAD(360.0 / 32)));
        glEnd();
    }
    glBegin(GL_LINES);
    glVertex2f(0, 35);
    glVertex2f(0, -35);
    glVertex2f(-35, 0);
    glVertex2f(35, 0);
    glEnd();
    glColor3f(0, 1, 0);
    text(-w / 2 + w / 10, h / 2 - h / 10, F.message);
    sprintf(buf, "velocity: %3.2f", F.speed);
    text(w / 2 - w / 4, h / 2 - h / 10, buf);
    sprintf(buf, "heading: %3.2f", F.plane->hpr[0]);
    text(w / 2 - w / 4, h / 2 - h / 10 - 24, buf);
    sprintf(buf, "hat: %3.2f", F.height < 1e5f ? F.height : 0.0f);
    text(w / 2 - w / 4, h / 2 - h / 10 - 48, buf);
    if (F.target >= 0) {
        glColor3f(1, 0, 0);
        sprintf(buf, "Locked on Enemy %d", F.target + 1);
        text(-90, -h / 2 + h * 3 / 20, buf);
    }
    if (F.crashed) {
        glColor3f(0, 1, 0);
        text(-200, -h / 2 + h / 4, "You just died...Attempting to restart mission");
    }
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/* --- initFlight ------------------------------------------------------------------- */

static void initFlight(const char *arg)
{
    int i;
    (void)arg;
    memset(&F, 0, sizeof(F));
    F.target = -1;
    F.radar = 30000;
    F.speed = 100;
    F.isect_on = 1;
    F.isect_period = 20;
    F.height = 1e6f;
    if ((F.camera = esFindObject("camera")) == NULL) {
        fprintf(stderr, "Couldn't find camera object\n");
        exit(1);
    }
    for (i = 0; i < es.nchannels; i++)
        es.channels[i].attach = F.camera;
    if ((F.plane = esFindObject("f18")) == NULL) {
        fprintf(stderr, "Couldn't find f18\n");
        exit(1);
    }
    /* the eye follows the airplane 30 frames behind */
    W.from = F.camera;
    W.to = F.plane;
    wingman_values(0.1f, -90, 0, 1, 0, 0);
    wingman_mode(WM_FOLLOW);
    esAddModuleFunc("updateFlight", ES_PREAPP, updateFlight);
    esAddModuleFunc("dispFlight", ES_POSTDRAW, dispFlight);
    readRoute();
    esAddModuleFunc("updateEnemy", ES_PREAPP, updateEnemy);
    esAddModuleFunc("updateEye", ES_POSTAPP, updateEye);
    toggleWingman(NULL);
    wingman_mode(WM_FOLLOW);
}

void flight_register(void)
{
    esRegisterFunction("eyeStuff", eyeStuff);
    esRegisterFunction("initFlight", initFlight);
    esRegisterFunction("eyeFlight", eyeFlight);
    esRegisterFunction("watchEnemy", watchEnemy);
    esRegisterFunction("toggleWingman", toggleWingman);
    esRegisterFunction("noSpeeding", noSpeeding);
    esRegisterFunction("toggleHud", toggleHud);
    esRegisterFunction("plusRadar", plusRadar);
    esRegisterFunction("minusRadar", minusRadar);
    esRegisterFunction("restartFlight", restartFlight);
    esRegisterFunction("toggleBeeBox", toggleBeeBox);
    esRegisterFunction("setFadeDistance", setFadeDistance);
}
