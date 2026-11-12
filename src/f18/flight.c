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
 * origin).  The route files (data/f18/N.route, in the directory of the
 * demo as the original opened them) are found through the path of the
 * script; when one is missing, a route is made up from turns, climbs and
 * straight lines.
 *
 * The missiles (loadMissiles, initMissile, updateMissile, cbMissile,
 * explodeMissile) hang under the wings; the left button fires the left one
 * (then the right one) at the enemy locked: 10 m/s faster than the
 * airplane, 5 m/s faster each frame up to 600 m/s, turning by 0.3 degree a
 * frame toward its target until it goes past it (C: straight to it), its
 * segments (20 m ahead of the nose, 10 m across) tested every 2 frames
 * against its target; it explodes on its target, after 60 seconds, or with
 * the Alt keys, and is back under its wing, reloaded in 10 frames. The
 * enemy hit falls along its heading, rolling, and crashes as a wreck; an
 * enemy with a missile on its tail rolls to a random angle and pulls.
 *
 * The effects (effects.c): explosions and debris when a missile explodes
 * or an airplane crashes, black smoke behind the airplanes hit, smoke and
 * flames (flames.dwb, an animation) on their wrecks, the exhaust of the
 * missiles, a white flash when the player crashes. The gun (space bar)
 * makes explosions on the enemy in the sight, without damage, as in the
 * demo.
 *
 * Not rebuilt yet: the trails, the weather, the sound and the radar.
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
#include "effects.h"

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
    esObject *flames;       /* burning on its wreck */
    int fume;               /* its black smoke when hit, -1 none */
    int smoke;              /* the smoke of its wreck, -1 none */
} Enemy;

/* the two missiles under the wings (EasyScene: 32 bytes from +0xc0 of the
 * flight) */
typedef struct Missile {
    esObject *obj;
    long fire_frame;        /* the frame it was fired, 0 not flying */
    int passed;             /* went past its target: no more a threat */
    int locked;             /* guided to its target */
    int target;             /* enemy, -1 none */
    long reload_frame;      /* reloading since, 0 none */
    float speed;
    int isect_count;
    int fume;               /* its exhaust */
} Missile;

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
    Missile missile[2];
    int fired[2];           /* the missile is flying */
    int cheat;              /* C: the missiles fly straight to their target */
} Flight;

static void fire_missile(void);
static void updateMissile(const char *arg);

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
    if (hud)
        hud->hud_off = F.hud_off;
}

/* --- the variables of the HUD (easyScene.vars) ---------------------------------- */

enum {
    HV_X, HV_Y, HV_Z, HV_PITCH, HV_ROLL, HV_HEADING, HV_VELOCITY, HV_CLIMB,
    HV_HOT, HV_MSL, HV_ASL, HV_LEFT, HV_LCOL, HV_RIGHT, HV_RCOL, HV_INRANGE, HV_N
};

static float hudVars[HV_N];

static float signed_angle(float a)
{
    if (a > 180) return a - 360;
    if (a < -180) return a + 360;
    return a;
}

/* updateHudValues: the airplane in EasyScene coordinates, its pitch and
 * roll from -180 to 180, its heading in tenths of degrees, its speed and
 * height above the ground, the enemy locked (-1 none), and the missiles:
 * loading time (10 when loaded) and color (600 green when loaded, 250 red
 * while loading); the missiles are not rebuilt yet, both are loaded */
static void updateHudValues(const char *arg)
{
    esObject *p = F.plane;
    (void)arg;
    if (!p)
        return;
    hudVars[HV_X] = p->xyz[0];
    hudVars[HV_Y] = p->xyz[2];
    hudVars[HV_Z] = -p->xyz[1];
    hudVars[HV_PITCH] = signed_angle(p->hpr[1]);
    hudVars[HV_HEADING] = p->hpr[0] * 10;
    hudVars[HV_ROLL] = signed_angle(p->hpr[2]);
    hudVars[HV_VELOCITY] = F.speed;
    hudVars[HV_HOT] = F.height < 1e5f ? F.height : 0;
    hudVars[HV_ASL] = F.fired[0] && F.fired[1] ? -1.0f : (float)F.target;
    {
        int i;
        for (i = 0; i < 2; i++) {
            Missile *m = &F.missile[i];
            float *load = &hudVars[i ? HV_RIGHT : HV_LEFT], *col = &hudVars[i ? HV_RCOL : HV_LCOL];
            if (m->reload_frame) {
                /* loading for 10 frames, red, then "Ready" and green */
                *load = (float)(F.frame - m->reload_frame);
                *col = 250;
                if (*load >= 10) {
                    strcpy(F.message, "Ready");
                    m->reload_frame = 0;
                    *col = 600;
                    *load = 10;
                }
            } else {
                *col = 600;
                *load = F.fired[i] ? 0.0f : 10.0f;
            }
        }
    }
    hudVars[HV_INRANGE] = -1;
}

static void initAnimation(const char *arg) { (void)arg; }
static void updateAnimation(const char *arg) { (void)arg; }

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
    if (F.crashed) {
        restart();
    } else if (F.target >= 0 && F.target < F.nenemies) {
        /* the gun: explosions on the enemy in the sight (esGetObjInLOS),
         * without damage, as in the demo */
        fxAddEffect(FX_EXPLOSION, F.enemy[F.target].obj->xyz, 1);
        fxAddEffect(FX_DEBRIS, F.enemy[F.target].obj->xyz, 1);
    }
}

static void toggleBeeBox(const char *arg) { (void)arg; }
static void initEffects(const char *arg) { (void)arg; fxInit(); }
static void nothing(const char *arg) { (void)arg; }
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
        fxAddEffect(FX_FLASH, F.plane->xyz, 3);
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
    /* F18_LOG: the messages of the display on the error output */
    {
        static char last[128];
        static int log = -1;
        if (log < 0)
            log = getenv("F18_LOG") != NULL;
        if (log && strcmp(last, F.message) != 0) {
            strcpy(last, F.message);
            fprintf(stderr, "[%ld] %s (target %d)\n", F.frame, F.message, F.target);
        }
    }
    if (F.crashed) {
        /* "You just died...": 5 seconds before starting again */
        if (++F.crashed > es.framerate * 5)
            restart();
        return;
    }
    lock_target();
    if (f18_mouse.inside && !f18_mouse.no_steer && f18_mouse.w > 0 && f18_mouse.h > 0) {
        float hw = f18_mouse.w / 2.0f, hh = f18_mouse.h / 2.0f;
        /* the IRIS GL y goes up */
        pitch_rate = (hh - f18_mouse.y) / hh * 1.0f;
        roll_rate = (f18_mouse.x - hw) / hw * 5.0f;
    }
    /* the keyboard: the arrows hold the stick (up pushes the nose down) */
    if (f18_mouse.kleft != f18_mouse.kright)
        roll_rate = f18_mouse.kright ? 3.0f : -3.0f;
    if (f18_mouse.up != f18_mouse.down)
        pitch_rate = f18_mouse.up ? 0.7f : -0.7f;
    if (f18_mouse.faster || (f18_mouse.right && !f18_mouse.no_steer))
        F.speed += 1;
    else if (f18_mouse.slower || (f18_mouse.middle && !f18_mouse.no_steer))
        F.speed -= 1;
    /* the wheel: 10 m/s a step */
    F.speed += 10.0f * f18_mouse.wheel;
    f18_mouse.wheel = 0;
    if (F.speed >= 1000) F.speed = 1000;
    else if (F.speed <= 100) F.speed = 100;
    fly(F.plane, roll_rate, pitch_rate, F.speed);
    /* the left button (or Enter) fires a missile at the enemy locked: the
     * presses are counted, a quick tap falls between two frames */
    if (f18_mouse.fire > 0) {
        fire_missile();
        f18_mouse.fire = 0;
    }
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
        fprintf(stderr, "f18: no route for enemy %d (%s), a route is made up\n", n, name);
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
    if (e->flames && e->flames->node)
        pfNodeTravMask(e->flames->node, PFTRAV_DRAW, 0, PFTRAV_SELF, PF_SET);
    fxRemoveSmoke(e->smoke);
    e->smoke = -1;
    fxFumeOn(e->fume, 0);
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
        /* the flames of its wreck, as large as a fifth of the airplane */
        e->fume = e->smoke = -1;
        if ((e->flames = esAddObj("flames.dwb")) != NULL)
            pfDCSScale(e->flames->node, diag(e->obj) / 5);
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
        fprintf(stderr, "%s\n", F.message);
        e->dead_frame = (int)F.frame;
        e->saved_x = e->obj->xyz[0];
        e->saved_h = e->obj->hpr[0];
        e->saved_y = e->obj->xyz[1];
        if (e->sw)
            pfSwitchVal(e->sw, 1);
        /* the explosion, the smoke and the flames of the wreck */
        fxAddEffect(FX_EXPLOSION, e->obj->xyz, 2);
        fxAddEffect(FX_DEBRIS, e->obj->xyz, e->hit ? 1.5f : 1);
        fxFumeOn(e->fume, 0);
        e->smoke = fxAddSmoke(e->obj, 2 * diag(e->obj), 0.8f);
        if (e->flames && e->flames->node) {
            esObjectPos(e->flames, e->obj->xyz[0], e->obj->xyz[1], e->obj->xyz[2], 0, 0, 0);
            pfNodeTravMask(e->flames->node, PFTRAV_DRAW, 0xffffffffU, PFTRAV_SELF, PF_SET);
        }
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
            fprintf(stderr, "reincarnating enemy %d\n", i + 1);
            initEnemy(i);
            esObjectPos(o, e->saved_x, e->saved_y, o->xyz[2], e->saved_h, o->hpr[1], o->hpr[2]);
            continue;
        }
        if (e->hit) {
            /* hit by a missile: goes on along its heading, falling and
             * rolling, explosions after 1.5 and 4.5 seconds at 60 Hz */
            int t = e->fall_frame++;
            float fr = es.framerate, v = e->speed / fr, sp, cp, sh, ch, vz, z;
            pfSinCos(p, &sp, &cp);
            vz = v * sp - 10.0f * t / (fr * fr);
            pfSinCos(h - 180, &sh, &ch);
            z = e->fall[2] + vz * t - 5.0f * t * t / (fr * fr);
            if (e->height <= o->xyz[2] - z)
                z = e->ground + 2;
            esObjectPos(o, e->fall[0] + v * t * sh, e->fall[1] - v * t * ch, z, h, p, r + 5);
            /* explosions from 1.5 to 3 and from 4.5 to 6 seconds (the demo
             * adds one each frame, here one every 6 frames) */
            if ((t / 90 == 1 || t / 90 == 3) && t % 6 == 0) {
                fxAddEffect(FX_EXPLOSION, o->xyz, 0.5f);
                fxAddEffect(FX_DEBRIS, o->xyz, 1);
            }
        } else {
            /* a missile coming: evade (roll to a random angle and pull),
             * or climb first when low */
            if (e->state == 0 || e->state == 2 || e->state == 3 || e->state == 4) {
                int k, threat = 0;
                for (k = 0; k < 2; k++)
                    if (F.fired[k] && F.missile[k].target == i && !F.missile[k].passed)
                        threat = 1;
                if (threat) {
                    e->dir = rand() % 2 ? 1 : -1;
                    if (e->saved_roll == -1)
                        e->saved_roll = r;
                    e->rand_angle = (float)(rand() % 70);
                    if (e->state == 2)
                        e->pitch_rate = 0.3f;
                    else if (e->height > 2000)
                        e->state = 1;
                    else
                        e->state = 7;
                } else if (e->state == 2) {
                    e->pitch_rate = 0;
                }
            }
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
            case 7: { /* evade a missile */
                float to = e->rand_angle * -e->dir;
                pitch_rate = -0.7f;
                roll_rate = e->dir * 3.0f;
                if (r <= to + 3 && r >= to - 3) {
                    r = to;
                    roll_rate = 0;
                    pitch_rate = 0;
                    e->state = 4;
                }
                break;
            }
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

/* --- the missiles ------------------------------------------------------------------- */

/* positionMissile: under the wing, (-4, -0.45, 0) and (4, -0.45, 0) from the
 * airplane in EasyScene coordinates */
static void attach_missile(int i)
{
    pfMatrix m;
    pfCoord c;
    pfVec3 off, p;
    pfSetVec3(off, i ? 4.0f : -4.0f, 0, -0.45f);
    pfCopyVec3(c.xyz, F.plane->xyz);
    pfSetVec3(c.hpr, F.plane->hpr[0], F.plane->hpr[1], -F.plane->hpr[2]);
    pfMakeCoordMat(m, &c);
    pfXformPt3(p, off, m);
    esObjectPos(F.missile[i].obj, p[0], p[1], p[2], F.plane->hpr[0], F.plane->hpr[1], F.plane->hpr[2]);
}

static void loadMissiles(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        memset(&F.missile[i], 0, sizeof(Missile));
        F.missile[i].target = -1;
        if ((F.missile[i].obj = esAddObj("missile.dwb")) == NULL) {
            fprintf(stderr, "Couldn't find missile\n");
            exit(1);
        }
        attach_missile(i);
        F.missile[i].fume = fxAddFume(F.missile[i].obj, 0.9f, 0.9f, 0.88f, 1, 10, 75);
        fxFumeOn(F.missile[i].fume, 0);
    }
    esAddModuleFunc("updateMissile", ES_PREAPP, updateMissile);
}

/* initMissile: leaves the wing with the orientation of the airplane, 10 m/s
 * faster, toward the enemy locked */
static void initMissile(int i)
{
    Missile *m = &F.missile[i];
    F.fired[i] = 1;
    esObjectPos(m->obj, m->obj->xyz[0], m->obj->xyz[1], m->obj->xyz[2],
                F.plane->hpr[0], F.plane->hpr[1], F.plane->hpr[2]);
    m->speed = F.speed + 10;
    m->fire_frame = F.frame;
    m->passed = 0;
    m->isect_count = 0;
    fxFumeOn(m->fume, 1);
}

static void fire_missile(void)
{
    /* the left one, the right one when the left one is flying (again when
     * both are flying, as the demo did) */
    int i = F.fired[0] ? 1 : 0;
    F.missile[i].target = F.target;
    F.missile[i].locked = F.target >= 0;
    initMissile(i);
    message("Fired missile %d", i + 1);
}

/* explodeMissile: back under its wing, reloaded in 10 frames */
static void explodeMissile(int i)
{
    Missile *m = &F.missile[i];
    if (F.fired[i]) {
        fxAddEffect(FX_EXPLOSION, m->obj->xyz, 2);
        fxAddEffect(FX_DEBRIS, m->obj->xyz, 1);
    }
    fxFumeOn(m->fume, 0);
    F.fired[i] = 0;
    m->locked = 0;
    m->fire_frame = 0;
    m->speed = 0;
    m->reload_frame = F.frame;
    F.target = 0;
    attach_missile(i);
}

static void explodeLeftMissile(const char *arg)
{
    (void)arg;
    strcpy(F.message, "Exploding missile 1");
    if (F.fired[0])
        explodeMissile(0);
}

static void explodeRightMissile(const char *arg)
{
    (void)arg;
    strcpy(F.message, "Exploding missile 2");
    if (F.fired[1])
        explodeMissile(1);
}

/* C: the missiles turn at once toward their target */
static void cheatMissile(const char *arg)
{
    (void)arg;
    F.cheat = !F.cheat;
    sprintf(F.message, "cheatMode: %s", F.cheat ? "on" : "off");
}

/* cbMissile: the segments of the missile (20 m ahead of its nose, 10 m
 * across it, up and sideways) against its target */
static int missile_hits(Missile *m, esObject *target)
{
    static const float seg[3][7] = {
        /* EasyScene (0,0,-3) along -Z, (0,-5,-3) along Y, (-5,0,-3) along X */
        { 0, 3, 0, 0, 1, 0, 20 }, { 0, 3, -5, 0, 0, 1, 10 }, { -5, 3, 0, 1, 0, 0, 10 }
    };
    pfSegSet ss;
    pfHit **hits[PFIS_MAX_SEGS];
    pfMatrix mat;
    pfCoord c;
    int k;
    if (!target || !target->node)
        return 0;
    memset(&ss, 0, sizeof(ss));
    pfCopyVec3(c.xyz, m->obj->xyz);
    pfSetVec3(c.hpr, m->obj->hpr[0], m->obj->hpr[1], -m->obj->hpr[2]);
    pfMakeCoordMat(mat, &c);
    for (k = 0; k < 3; k++) {
        pfXformPt3(ss.segs[k].pos, seg[k], mat);
        pfXformVec3(ss.segs[k].dir, seg[k] + 3, mat);
        ss.segs[k].length = seg[k][6];
    }
    ss.activeMask = 7;
    ss.isectMask = 0xffffffffU;
    return pfNodeIsectSegs(target->node, &ss, hits) > 0;
}

static void cbMissile(int i)
{
    Missile *m = &F.missile[i];
    Enemy *e;
    if (m->target < 0 || m->target >= F.nenemies)
        return;
    e = &F.enemy[m->target];
    if (e->dead || e->hit || !missile_hits(m, e->obj))
        return;
    sprintf(F.message, "Missile %d hit Enemy %d", i + 1, m->target + 1);
    fprintf(stderr, "%s\n", F.message);
    /* the enemy falls from there, a wreck */
    e->hit = 1;
    e->fall_frame = 1;
    pfCopyVec3(e->fall, m->obj->xyz);
    if (e->sw)
        pfSwitchVal(e->sw, 1);
    e->isect_period = 1;
    /* black smoke behind it */
    if (e->fume < 0)
        e->fume = fxAddFume(e->obj, 0.12f, 0.11f, 0.1f, 1, 10, 100);
    fxFumeOn(e->fume, 1);
    explodeMissile(i);
}

static float wrap_to(float want, float h)
{
    if (h <= 0 && want >= 0) return want - 360;
    if (h >= 0 && want <= 0) return want + 360;
    return want;
}

/* updateMissile: faster up to 600 m/s, turning by 0.3 degree a frame toward
 * the target until it goes past it, exploding after 60 seconds */
static void updateMissile(const char *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < 2; i++) {
        Missile *m = &F.missile[i];
        esObject *o = m->obj;
        float h = o->hpr[0], p = o->hpr[1], d0 = 0;
        pfVec3 d;
        if (!F.fired[i]) {
            attach_missile(i);
            continue;
        }
        if (m->fire_frame && F.frame - m->fire_frame >= 60 * es.framerate) {
            m->fire_frame = 0;
            explodeMissile(i);
            message("Auto detonating missile %d", i + 1);
            continue;
        }
        if (m->speed <= 600)
            m->speed += 5;
        if (m->locked && m->target >= 0) {
            const esObject *t = F.enemy[m->target].obj;
            float dx = t->xyz[0] - o->xyz[0], dy = t->xyz[1] - o->xyz[1], dz = t->xyz[2] - o->xyz[2];
            float want_h = (float)PF_RAD2DEG(-atan2(dx, dy));
            float want_p = (float)PF_RAD2DEG(atan2(dz, sqrt(dx * dx + dy * dy)));
            d0 = dx * dx + dy * dy + dz * dz;
            if (F.cheat) {
                h = want_h;
                p = want_p;
            } else {
                float dh = wrap_to(want_h, h) - h, dp = want_p - p;
                /* as the demo: below 0.3 degree away on the negative side
                 * the missile turns the wrong way, then back by 0.3 */
                if (dh > 0) h += dh >= 0.3f ? 0.3f : dh;
                else h -= dh <= -0.3f ? 0.3f : dh;
                if (dp > 0) p += dp >= 0.3f ? 0.3f : dp;
                else p -= dp <= -0.3f ? 0.3f : dp;
            }
        }
        polar(d, h, p);
        esObjectPos(o, o->xyz[0] + d[0] * m->speed / es.framerate, o->xyz[1] + d[1] * m->speed / es.framerate,
                    o->xyz[2] + d[2] * m->speed / es.framerate, h, p, o->hpr[2]);
        if (m->locked && m->target >= 0) {
            const esObject *t = F.enemy[m->target].obj;
            pfVec3 v;
            pfSubVec3(v, t->xyz, o->xyz);
            if (d0 < pfDotVec3(v, v)) {
                m->locked = 0;
                m->passed = 1;
                if (getenv("F18_LOG"))
                    fprintf(stderr, "[%ld] missile %d went past enemy %d\n", F.frame, i + 1, m->target + 1);
            }
        }
        /* its segments are tested every 2 frames */
        if (++m->isect_count >= 2) {
            m->isect_count = 0;
            cbMissile(i);
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
    /* the HUD 2.5 m in front of the eye, with the variables of the flight */
    {
        esObject *hud = esFindObject("f18hud");
        if (hud) {
            esSetHud(hud, 1, 0, 0, -2.5f);
            esSetDrtBaseAddr(hud, hudVars, HV_N);
        }
    }
    loadMissiles();
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
    esRegisterFunction("explodeLeftMissile", explodeLeftMissile);
    esRegisterFunction("explodeRightMissile", explodeRightMissile);
    esRegisterFunction("cheatMissile", cheatMissile);
    /* the effects: the smoke and the fumes are updated and drawn with them */
    esRegisterFunction("initEffects", initEffects);
    esRegisterFunction("initSmoke", initEffects);
    esRegisterFunction("initFumes", initEffects);
    esRegisterFunction("updateSmoke", nothing);
    esRegisterFunction("drawSmoke", nothing);
    esRegisterFunction("toggleBeeBox", toggleBeeBox);
    esRegisterFunction("setFadeDistance", setFadeDistance);
    esRegisterFunction("updateHudValues", updateHudValues);
    esRegisterFunction("initAnimation", initAnimation);
    esRegisterFunction("updateAnimation", updateAnimation);
}
