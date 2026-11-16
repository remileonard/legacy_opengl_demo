/*
 * ci_main.c - Certain Impact (Paradigm Simulation, 1996), a World War I
 * flight game written with Vega, ported to OpenGL and GLUT.
 *
 * This file is the application (the mardi class of the original): the main
 * loop, the views (cockpit, follow, train and dying views), the cockpit
 * instruments and map, the selection screen, the intro pages and the
 * explosions (vgObjBank).
 *
 * usage: cimpact [-noinstruct] [-nofog] [-nosound] [-nonetwork] [data directory] [file.adf]
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200112L
#endif
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
#include <time.h>

#include "ci.h"
#include "jpeg.h"

/* --- globals ----------------------------------------------------------------------- */

vgScene *gMainScene, *gCockpitScene, *gSelScene, *gSelCockpitScene;
Plane *gUserPlane;
int gDifficulty = 1;
int gPlaneType;
double gNow;
float gDt = 1 / 30.0f, gDt90 = 3;
float gRand;
int gLayout = 4;
int gTrainFront = 1;
int gShowMap = 1;
AmmoBank *gGunBank, *gBombBank, *gEnemyBank, *gFlakBank;

PlaneType gTypes[6] = {
    { 51.40979f, 186, 1 / 186.0f, 0.6f, 100, 2, { 0, -15, 3 }, "German Fokker D VII" },
    { 51.40979f, 186, 1 / 186.0f, 0.95f, 150, 4, { 0, -20, 3.75f }, "German Fokker Dr I" },
    { 46.0452919f, 206, 1 / 206.0f, 0.8f, 75, 4, { 0, -20, 3.5f }, "German Albatros D III" },
    { 49.621624f, 333, 1 / 333.0f, 0.8f, 100, 4, { 0, -15, 3.5f }, "British Sopwith" },
    { 41.5748749f, 100, 1 / 100.0f, 1.0f, 100, 6, { 0, -15, 1 }, "British Fe2b" },
    { 43.3630409f, 175, 1 / 175.0f, 0.9f, 150, 4, { 0, -15, 3.5f }, "American Standard" },
};

static const int plane_codes[6] = { 17, 49, 33, 513, 256, 769 };

static int playing;              /* i2556 */
static int sel_code = 17;        /* i156 */
static int instruct = 1;
static int fog = 1;
static float tether_scale = 5;   /* f2288 */
static double last_time, start_time;
static double anim_next;         /* f2620 */
static float crash_timer;        /* f2580 */
static float rattle_x, rattle_y; /* f2660, f2664 */
static int win_w = 1000, win_h = 800;
static int vp_x, vp_y, vp_w, vp_h;   /* the 5:4 area of the game */
static float spin_angle;
static pfVec3 follow_pos;
static int follow_valid;

/* input */
static int mouse_x, mouse_y, mouse_in;
static int btn_left, btn_right, btn_middle;
static int key_up, key_down, key_fire;

/* intro pages */
static int page = -1;
static double page_time;
static unsigned int page_tex;
static int page_w, page_h;

/* cockpit */
static vgObject *cockpit, *cockpit_map, *cockpit_sel;
static pfNode *map_switch;
static pfNode *gauge_thrust, *gauge_engine, *gauge_speed, *gauge_fuel;
static pfNode *own_switch;        /* i2344: the own airplane, hidden in the cockpit view */

/* selection screen (vgSelections) */
typedef struct {
    float x0, x1, y0, y1;
    pfNode *sw;
} SelBox;
static SelBox sel_boxes[11];
static int sel_hover;             /* i0 */
static int sel_difficulty = 1;    /* i420 */
static int sel_start;             /* i424 */
static int sel_clicked;
static pfNode *sel_windsocks[8];
static int sel_nwindsocks;

double ciRand(void) { return (double)rand() / ((double)RAND_MAX + 1); }

/* hpr of a rotation matrix (inverse of pfMakeEulerMat) */
void ciCopyMatHPR(pfMatrix m, float *hpr)
{
    double sp = m[1][2], cp;
    if (sp > 1) sp = 1;
    if (sp < -1) sp = -1;
    cp = sqrt(1 - sp * sp);
    hpr[1] = (float)PF_RAD2DEG(asin(sp));
    if (cp > 1e-6) {
        hpr[0] = (float)PF_RAD2DEG(atan2(-m[1][0], m[1][1]));
        hpr[2] = (float)PF_RAD2DEG(atan2(-m[0][2], m[2][2]));
    } else {
        hpr[0] = (float)PF_RAD2DEG(atan2(m[0][1], m[0][0]));
        hpr[2] = 0;
    }
}

/* --- explosions (vgObjBank) ------------------------------------------------------- */

typedef struct {
    vgObject *obj;
    pfNode *seq;
    int busy;
    double t0;
} BankItem;

typedef struct {
    int n, last;
    BankItem items[8];
    double life;
} ObjBank;

static ObjBank flak_bank, small_bank, big_bank, air_bank;

static void bank_init(ObjBank *b, int n, const char *name, float speed)
{
    vgObject *proto = vgFindObj(name);
    int i;
    memset(b, 0, sizeof(*b));
    b->n = n;
    b->last = -1;
    b->life = 2.0;
    if (!proto)
        return;
    for (i = 0; i < n; i++) {
        BankItem *it = &b->items[i];
        it->obj = vgCopyObj(proto);
        vgObjScale(it->obj, (float)(ciRand() + 0.75));
        it->seq = vgFindPart(it->obj, "seqnce", 0);
        if (it->seq && it->seq->type == PFNODE_SEQUENCE) {
            pfSeqDuration(it->seq, (float)(it->seq->speed * (speed + 0.25 * (1 - ciRand()))), 1);
            pfSeqMode(it->seq, PFSEQ_STOP);
        } else {
            it->seq = NULL;
        }
    }
}

static void bank_get(ObjBank *b, const float *p)
{
    int i, k;
    for (k = 0; k < b->n; k++) {
        i = (b->last + 1 + k) % b->n;
        if (!b->items[i].busy && b->items[i].obj) {
            BankItem *it = &b->items[i];
            b->last = i;
            it->busy = 1;
            it->t0 = gNow;
            if (it->seq)
                pfSeqMode(it->seq, PFSEQ_START);
            vgObjPos(it->obj, p[0], p[1], p[2], 0, 0, 0);
            vgAddSceneObj(gMainScene, it->obj);
            return;
        }
    }
}

static void bank_update(ObjBank *b)
{
    int i;
    for (i = 0; i < b->n; i++) {
        BankItem *it = &b->items[i];
        if (it->busy && it->t0 + b->life < gNow) {
            it->busy = 0;
            if (it->seq)
                pfSeqMode(it->seq, PFSEQ_STOP);
            vgRemSceneObj(gMainScene, it->obj);
        }
    }
}

void ciCreateFlakExplosion(const float *p) { bank_get(&flak_bank, p); }
void ciCreateAirExplosion(const float *p) { bank_get(&air_bank, p); }
void ciCreateSmallGroundExplosion(const float *p) { bank_get(&small_bank, p); }
void ciCreateBigGroundExplosion(const float *p) { bank_get(&big_bank, p); }

/* --- views (mardi::changeWindowLayout) ---------------------------------------------- */

void ciChangeLayout(int layout)
{
    gLayout = layout;
    follow_valid = 0;
    if (own_switch)
        pfSwitchVal(own_switch, layout == 0 ? PFSWITCH_OFF : PFSWITCH_ON);
    if (layout == 3)
        spin_angle = gUserPlane ? gUserPlane->pos[3] : 0;
}

static void toggle_layout(void)
{
    int l = gLayout + 1;
    if (l >= 3)
        l = 0;
    ciChangeLayout(l);
}

void ciTetherTrainBack(void)
{
    if (!gTrainFront)
        return;
    gTrainFront = 0;
}

/* camera of the main channel: position and orientation */
static void camera(float *eye, pfMatrix cam)
{
    pfCoord c;
    Plane *u = gUserPlane;
    pfMatrix m;
    pfVec3 off, d;
    pfSetVec3(c.xyz, 0, 0, 0);
    pfSetVec3(c.hpr, 0, 0, 0);
    if (gLayout == 4 || !u) {
        /* selection screen */
        pfSetVec3(c.xyz, 95, -37, 55);
        pfSetVec3(c.hpr, 65, -15.5f, 0);
    } else if (gLayout == 0) {
        pfMatrix pm;
        pfCoord pc;
        pfSetVec3(pc.xyz, u->pos[0], u->pos[1], u->pos[2]);
        pfSetVec3(pc.hpr, u->pos[3], u->pos[4], u->pos[5]);
        pfMakeCoordMat(pm, &pc);
        pfXformPt3(c.xyz, u->eye, pm);
        pfSetVec3(c.hpr, u->pos[3], u->pos[4], u->pos[5]);
    } else if (gLayout == 1) {
        /* follows behind the airplane, with some lag */
        pfVec3 target;
        float h = u->pos[3];
        pfSetVec3(off, gTypes[gPlaneType].tether[0], gTypes[gPlaneType].tether[1], gTypes[gPlaneType].tether[2]);
        pfScaleVec3(off, tether_scale / 5, off);
        pfSetVec3(c.hpr, h, 0, 0);
        pfMakeCoordMat(m, &c);
        pfXformVec3(d, off, m);
        target[0] = u->pos[0] + d[0];
        target[1] = u->pos[1] + d[1];
        target[2] = u->pos[2] + d[2];
        if (!follow_valid) {
            pfCopyVec3(follow_pos, target);
            follow_valid = 1;
        } else {
            float k = 1 - (float)pow(0.2, gDt * 5);
            follow_pos[0] += (target[0] - follow_pos[0]) * k;
            follow_pos[1] += (target[1] - follow_pos[1]) * k;
            follow_pos[2] += (target[2] - follow_pos[2]) * k;
        }
        pfCopyVec3(c.xyz, follow_pos);
        d[0] = u->pos[0] - c.xyz[0];
        d[1] = u->pos[1] - c.xyz[1];
        d[2] = u->pos[2] + 1 - c.xyz[2];
        c.hpr[0] = -pfArcTan2(d[0], d[1]);
        c.hpr[1] = pfArcTan2(d[2], sqrtf(d[0] * d[0] + d[1] * d[1]));
        c.hpr[2] = 0;
    } else if (gLayout == 2) {
        /* fixed on the engine of the train */
        float tr[3], planes[20][4], friends[20][4], gz = 0, hh, pp;
        int n1, n2;
        worldMapPositions(tr, &n1, planes, &n2, friends);
        if (gTrainFront) {
            pfSetVec3(off, -20, 25, 8);
            hh = -150;
            pp = -5;
        } else {
            pfSetVec3(off, -50, -60, 8);
            hh = -50;
            pp = -10;
        }
        pfSetVec3(c.hpr, tr[2], 0, 0);
        pfMakeCoordMat(m, &c);
        pfXformVec3(d, off, m);
        vgIsectZ(gMainScene, CL_HOT, tr[0], tr[1], 0, &gz, NULL, NULL, NULL);
        pfSetVec3(c.xyz, tr[0] + d[0], tr[1] + d[1], gz + d[2]);
        pfSetVec3(c.hpr, tr[2] + hh, pp, 0);
    } else {
        /* the airplane goes down: the camera turns around it */
        float s, co, r = 30;
        spin_angle += 0.75f * gDt * 30;
        pfSinCos(spin_angle, &s, &co);
        pfSetVec3(c.xyz, u->pos[0] + r * s, u->pos[1] - r * co, u->pos[2] + 10);
        d[0] = u->pos[0] - c.xyz[0];
        d[1] = u->pos[1] - c.xyz[1];
        d[2] = u->pos[2] - c.xyz[2];
        c.hpr[0] = -pfArcTan2(d[0], d[1]);
        c.hpr[1] = pfArcTan2(d[2], sqrtf(d[0] * d[0] + d[1] * d[1]));
        c.hpr[2] = 0;
    }
#ifdef CI_DEBUG
    if (getenv("CI_CAM"))
        sscanf(getenv("CI_CAM"), "%f,%f,%f,%f,%f", &c.xyz[0], &c.xyz[1], &c.xyz[2], &c.hpr[0], &c.hpr[1]);
#endif
    pfMakeCoordMat(cam, &c);
    pfCopyVec3(eye, c.xyz);
}

/* --- game flow (mardi::playGame and mardi::selectGameParams) ------------------------ */

static void sel_place_planes(void);

static void play_game(void)
{
    float init[6] = { 0, 0, 0, 0, 0, 0 };
    int i, nenemies;
    for (i = 0; i < 6; i++)
        if (plane_codes[i] == sel_code)
            gPlaneType = i;
    if (sel_code == 17 || sel_code == 49 || sel_code == 33) {
        init[0] = 4014.18994f;
        init[1] = -730.580017f;
        init[2] = 110;
        init[3] = 158;
    } else {
        init[0] = 1128.98999f;
        init[1] = 2060.81006f;
        init[2] = 53;
        init[3] = 135;
    }
    worldTerrainInitDoors();
    gUserPlane = planeNew(sel_code, init);
    gunsNew(gUserPlane);
    own_switch = vgFindPart(gUserPlane->obj, "onoff", 0);
    if (own_switch)
        pfSwitchVal(own_switch, PFSWITCH_ON);
    nenemies = gDifficulty == 2 ? 5 : 3;
    worldCreate(gUserPlane, nenemies);
    worldReset();
    crash_timer = 0;
    gTrainFront = 1;
    ciChangeLayout(0);
    playing = 1;
    gUserPlane->start_time = gNow;
}

static void select_game_params(void)
{
    if (playing) {
        worldDestroy();
        gunsDelete();
        planeDelete(gUserPlane);
        gUserPlane = NULL;
        own_switch = NULL;
    }
    sel_place_planes();
    playing = 0;
    ciChangeLayout(4);
}

/* mardi::reset and the reset of every motion model */
void ciResetMotionModels(void)
{
    Plane *u = gUserPlane;
    crash_timer = 0;
    rattle_x = rattle_y = 0;
    gTrainFront = 1;
    worldReset();
    worldResetMotion();
    planeResetMotion(u);
    ciChangeLayout(0);
}

/* --- selection screen (vgSelections) -------------------------------------------- */

static void sel_init(void)
{
    static const float b[11][4] = {
        { 0.06f, 0.325f, 0.43f, 0.55f }, { 0.41f, 0.69f, 0.41f, 0.58f }, { 0.72f, 0.95f, 0.44f, 0.565f },
        { 0.06f, 0.25f, 0.56f, 0.64f }, { 0.345f, 0.55f, 0.56f, 0.64f }, { 0.65f, 0.80f, 0.56f, 0.64f },
        { 0.40f, 0.60f, 0.115f, 0.165f }, { 0.40f, 0.60f, 0.205f, 0.245f }, { 0.40f, 0.60f, 0.165f, 0.205f },
        { 0.90f, 1.00f, 0.15f, 0.20f }, { 0.00f, 0.10f, 0.15f, 0.20f },
    };
    static const char *sw[11] = { NULL, NULL, NULL, NULL, NULL, NULL, "inv", "cadet", "ace", "start", "quit" };
    vgObject *tile = vgFindObj("SelectionTile");
    int i;
    cockpit_sel = vgFindObj("CockPitSel");
    vgObjModel(cockpit_sel);
    for (i = 0; i < 11; i++) {
        sel_boxes[i].x0 = b[i][0];
        sel_boxes[i].x1 = b[i][1];
        sel_boxes[i].y0 = b[i][2];
        sel_boxes[i].y1 = b[i][3];
        sel_boxes[i].sw = sw[i] ? vgFindPart(cockpit_sel, sw[i], 0) : NULL;
        if (sel_boxes[i].sw)
            pfSwitchVal(sel_boxes[i].sw, 0);
    }
    sel_nwindsocks = 0;
    for (i = 0; i < 8; i++) {
        pfNode *n = vgFindPart(tile, "windsck", i);
        if (!n)
            break;
        if (n->type == PFNODE_SEQUENCE) {
            pfSeqInterval(n, PFSEQ_SWING, 0, -1);
            sel_windsocks[sel_nwindsocks++] = n;
        }
    }
}

static void sel_place_planes(void)
{
    static const char *names[6] = { "Fokker", "RedBaron", "Albatros", "Sopwith", "Fe2b", "American" };
    static const char *shadows[6] = { "FokkerShadow", "RedBaronShadow", "AlbatrosShadow", "SopwithShadow",
                                      "Fe2bShadow", "AmericanShadow" };
    float a = 10.0f / 3, b = 30.0f / 3, c = 15.0f / 3, d = 35.0f / 3, base = 43.1f;
    float pos[6][6];
    int i;
    /* German airplanes in front, Allied ones behind */
    pos[0][0] = 55 + a;     pos[0][1] = -40 + b;     pos[0][2] = base + 1.65f; pos[0][4] = 18;
    pos[1][0] = 55 + 2 * a; pos[1][1] = -40 + 2 * b; pos[1][2] = base + 1.45f; pos[1][4] = 16;
    pos[2][0] = 55 + 3 * a; pos[2][1] = -40 + 3 * b; pos[2][2] = base + 1.45f; pos[2][4] = 13;
    pos[3][0] = 35 + c;     pos[3][1] = -40 + d;     pos[3][2] = base + 1.45f; pos[3][4] = 18;
    pos[4][0] = 35 + 2 * c; pos[4][1] = -40 + 2 * d; pos[4][2] = base + 1.39f; pos[4][4] = 0;
    pos[5][0] = 35 + 3 * c; pos[5][1] = -40 + 3 * d; pos[5][2] = base + 1.45f; pos[5][4] = 18;
    vgClearScene(gSelScene);
    vgAddSceneObj(gSelScene, vgFindObj("SelectionTile"));
    for (i = 0; i < 6; i++) {
        vgObject *o = vgFindObj(names[i]), *s = vgFindObj(shadows[i]);
        if (o) {
            vgObjModel(o);
            vgObjPos(o, pos[i][0], pos[i][1], pos[i][2], 280, pos[i][4], 0);
            vgAddSceneObj(gSelScene, o);
        }
        if (s) {
            vgObjModel(s);
            vgObjPos(s, pos[i][0], pos[i][1], base, 280, 0, 0);
            vgAddSceneObj(gSelScene, s);
        }
    }
}

/* mouse in the selection screen; x, y from -1 to 1 */
static void sel_input(float x, float y, int click)
{
    float aspect = 36.0f / 45.0f, off = (aspect - 1) * 0.5f;
    float mx = (x + 1) * 0.5f, my = ((y + 1) * 0.5f) * aspect - off;
    float best = 999999.0f;
    int i;
    sel_start = 0;
    for (i = 0; i < 11; i++)
        if (sel_boxes[i].sw)
            pfSwitchVal(sel_boxes[i].sw, 0);
    /* the difficulty that is selected */
    if (sel_boxes[6 + sel_difficulty].sw)
        pfSwitchVal(sel_boxes[6 + sel_difficulty].sw, 1);
    for (i = 0; i < 11; i++) {
        float cx = (sel_boxes[i].x0 + sel_boxes[i].x1) * 0.5f - mx;
        float cy = (sel_boxes[i].y0 + sel_boxes[i].y1) * 0.5f - my;
        float d = cx * cx + cy * cy;
        if (d < best) {
            best = d;
            sel_hover = i;
        }
    }
    if (sel_boxes[sel_hover].sw)
        pfSwitchVal(sel_boxes[sel_hover].sw, 2);
    if (click) {
        if (sel_hover < 6)
            sel_code = plane_codes[sel_hover];
        else if (sel_hover <= 8)
            sel_difficulty = sel_hover == 6 ? 0 : sel_hover == 7 ? 1 : 2;
        else if (sel_hover == 9)
            sel_start = 1;
        else
            exit(0);
        gDifficulty = sel_difficulty;
    }
}

/* --- time and frame (mardi::updateTime and mardi::updateFrame) --------------------- */

/*
 * The original runs at 30 frames per second (the frame rate of its
 * application definition file) and its flight model works per frame: the
 * frame rate is limited to 30 Hz as well. A slower machine gets longer
 * frames, as on the original.
 */
#define FRAME_TIME (1 / 30.0)

static double real_time(void)
{
    return pfGetTime() - start_time;
}

static void sleep_ms(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = ms * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

/* 0 while the frame is not over, else starts a new frame */
static int update_time(void)
{
    double t = real_time(), dt = t - last_time;
    if (dt < FRAME_TIME) {
        int ms = (int)((FRAME_TIME - dt) * 1000);
        if (ms > 1)
            sleep_ms(ms - 1);
        return 0;
    }
    if (dt > 0.5)
        dt = 0.5;
    /* frames late by less than a frame keep the 30 Hz rhythm */
    last_time = t - FRAME_TIME < last_time + FRAME_TIME ? last_time + FRAME_TIME : t;
    gDt = (float)(dt < FRAME_TIME * 1.5 ? FRAME_TIME : dt);
    gDt90 = gDt * 90;
    gNow = t;
    return 1;
}

static void main_viewport(int *x, int *y, int *w, int *h)
{
    *x = vp_x;
    *w = vp_w;
    if (playing && gLayout == 0) {
        *y = vp_y + (int)(vp_h * 0.17f);
        *h = vp_h - (int)(vp_h * 0.17f);
    } else {
        *y = vp_y;
        *h = vp_h;
    }
}

static void stick(float *sx, float *sy)
{
    int x, y, w, h;
    main_viewport(&x, &y, &w, &h);
    if (!mouse_in) {
        *sx = *sy = 0;
        return;
    }
    *sx = ((mouse_x - x) / (float)w) * 2 - 1;
    *sy = (((win_h - mouse_y) - y) / (float)h) * 2 - 1;
    if (*sx < -1) *sx = -1;
    if (*sx > 1) *sx = 1;
    if (*sy < -1) *sy = -1;
    if (*sy > 1) *sy = 1;
}

static void update_frame(void)
{
    gRand = (float)ciRand();
    if (!playing) {
        if (anim_next < gNow) {
            int i;
            float t = sel_nwindsocks && sel_windsocks[0]->nframes ? sel_windsocks[0]->times[0] : 0.2f;
            t += (float)((gRand - 0.5) * gDt);
            if (t > 0.4f) t = 0.4f;
            if (t < 0.1f) t = 0.1f;
            for (i = 0; i < sel_nwindsocks; i++)
                vgSeqFrameTime(sel_windsocks[i], t);
            anim_next = gNow + gRand * 5.0;
        }
        return;
    }
    {
        float sx, sy;
        int fire = btn_middle || (btn_left && btn_right) || key_fire;
        stick(&sx, &sy);
#ifdef CI_DEBUG
        if (getenv("CI_AUTO") && gUserPlane->state != 0) {
            /* test autopilot: climbs to 250 m and circles */
            Plane *u = gUserPlane;
            float tp = (250 - u->pos[2]) * 0.05f;
            if (tp > 10) tp = 10;
            if (tp < -10) tp = -10;
            sy = (u->pos[4] - tp) * 0.05f;
            sx = ((float)atof(getenv("CI_AUTO")) - u->pos[5]) * 0.02f;
            if (sy > 0.5f) sy = 0.5f;
            if (sy < -0.5f) sy = -0.5f;
            if (sx > 0.5f) sx = 0.5f;
            if (sx < -0.5f) sx = -0.5f;
        }
#endif
#ifdef CI_DEBUG
        if (getenv("CI_AUTO") && gUserPlane->state != 0) {
            /* test autopilot: climbs to 250 m and circles */
            Plane *u = gUserPlane;
            float tp = (250 - u->pos[2]) * 0.05f;
            if (tp > 10) tp = 10;
            if (tp < -10) tp = -10;
            sy = (u->pos[4] - tp) * 0.05f;
            sx = (atof(getenv("CI_AUTO")) - u->pos[5]) * 0.02f;
            if (sy > 0.5f) sy = 0.5f;
            if (sy < -0.5f) sy = -0.5f;
            if (sx > 0.5f) sx = 0.5f;
            if (sx < -0.5f) sx = -0.5f;
        }
#endif
        if (fire && !gUserPlane->dead)
            gunsTrigger();
        planeUpdate(gUserPlane, sx, sy, (btn_left && !btn_right) || key_up, (btn_right && !btn_left) || key_down);
    }
    bank_update(&flak_bank);
    bank_update(&small_bank);
    bank_update(&big_bank);
    bank_update(&air_bank);
    if (anim_next < gNow) {
        worldTerrainAnimations();
        anim_next = gNow + gRand * 5.0;
    }
    worldUpdate(gNow, gDt);
    gunsUpdate(gNow, gDt);
    bombsUpdate(gNow, gDt);
    {
        Plane *u = gUserPlane;
        if (u->state == 0) {
            if (u->fuel > 0) {
                if (gLayout == 0) {
                    /* the cockpit rattles with the engine */
                    float dr = (float)((0.5 - gRand) * 0.1);
                    rattle_x += dr;
                    rattle_y += dr;
                    if (rattle_x > 0.00075f) rattle_x = 0.00075f;
                    if (rattle_x < -0.00075f) rattle_x = -0.00075f;
                    if (rattle_y > 0.00075f) rattle_y = 0.00075f;
                    if (rattle_y < -0.00075f) rattle_y = -0.00075f;
                }
            } else {
                crash_timer += gDt;
                if (crash_timer > 0) {
                    ciChangeLayout(1);
                    ciResetMotionModels();
                }
            }
        } else if (u->state == 3 || u->state == 4) {
            crash_timer += gDt;
            if (crash_timer > 5) {
                ciChangeLayout(1);
                ciResetMotionModels();
            }
        }
    }
}

/* --- drawing ------------------------------------------------------------------------ */

static void setup_light(void)
{
    float pos[4] = { 0, 0.0871557f, 0.9961947f, 0 };
    float amb[4] = { 0.3f, 0.3f, 0.3f, 1 }, dif[4] = { 1, 1, 1, 1 }, global[4] = { 0.2f, 0.2f, 0.2f, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, dif);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global);
    glEnable(GL_LIGHT0);
}

static void draw_main(void)
{
    int x, y, w, h;
    pfMatrix cam, inv;
    float eye[3], fovy;
    vgEnv *env = vgGetEnv();
    main_viewport(&x, &y, &w, &h);
    glViewport(x, y, w, h);
    glScissor(x, y, w, h);
    glEnable(GL_SCISSOR_TEST);
    glClearColor(env->skycolor[0], env->skycolor[1], env->skycolor[2], 1);
#ifdef CI_DEBUG
    if (getenv("CI_CAM") || getenv("CI_MAGENTA"))
        glClearColor(1, 0, 1, 1);
#endif
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    fovy = (float)PF_RAD2DEG(2 * atan(tan(PF_DEG2RAD(22.5)) * h / w));
    gluPerspective(fovy, (double)w / h, 4, 3500);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    camera(eye, cam);
    glRotatef(-90, 1, 0, 0);
    pfInvertOrthoMat(inv, cam);
    glMultMatrixf(&inv[0][0]);
    setup_light();
    if (playing && fog) {
        float fc[4];
        fc[0] = env->skycolor[0];
        fc[1] = env->skycolor[1];
        fc[2] = env->skycolor[2];
        fc[3] = 1;
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogf(GL_FOG_START, env->visnear);
        glFogf(GL_FOG_END, env->visfar);
        glFogfv(GL_FOG_COLOR, fc);
        glEnable(GL_FOG);
    }
    pfEnable(PFEN_LIGHTING);
    pfEnable(PFEN_TEXTURE);
#ifdef CI_DEBUG
    if (getenv("CI_NOTEX"))
        pfDisable(PFEN_TEXTURE);
    if (getenv("CI_CAM"))
        vgDrawScene(gMainScene, gNow);
    else
#endif
    vgDrawScene(playing ? gMainScene : gSelScene, gNow);
    if (playing) {
        ammoDraw(gGunBank);
        ammoDraw(gEnemyBank);
    }
    glDisable(GL_FOG);

    /* under water or crashed (mardi::mainPostDraw) */
    if (playing && gUserPlane->state == 3) {
        glClearColor(0, 0.117647059f, 0.3137f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    } else if (playing && gUserPlane->state == 4) {
        glClearColor(0.8f * gRand, 0.1f * gRand, 0.05f * gRand, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glDisable(GL_SCISSOR_TEST);
}

/* 2D coordinates of the cockpit channel: x from 0 to 1, y from 0.1 to 0.9 */
static void ortho_cockpit(void)
{
    float aspect = 36.0f / 45.0f, off = (aspect - 1) * 0.5f;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1, -off, aspect - off);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void draw_box(const SelBox *b)
{
    glBegin(GL_LINE_LOOP);
    glVertex2f(b->x0, b->y0);
    glVertex2f(b->x0, b->y1);
    glVertex2f(b->x1, b->y1);
    glVertex2f(b->x1, b->y0);
    glEnd();
}

static void map_plane(float x, float y, float z, float h)
{
    float s = 150 * (z / 500 + 1);
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(h, 0, 0, 1);
    glScalef(s, s, s);
    glBegin(GL_LINE_LOOP);
    glVertex2f(1, 1); glVertex2f(0.8f, 0.5f); glVertex2f(0.15f, 0.5f); glVertex2f(0.15f, -0.5f);
    glVertex2f(0.45f, -0.5f); glVertex2f(0.45f, -0.75f); glVertex2f(-0.45f, -0.75f); glVertex2f(-0.45f, -0.5f);
    glVertex2f(-0.15f, -0.5f); glVertex2f(-0.15f, 0.5f); glVertex2f(-0.8f, 0.5f); glVertex2f(-1, 1);
    glEnd();
    glPopMatrix();
}

static void map_train(float x, float y, float h)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(h, 0, 0, 1);
    glScalef(200, 200, 200);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0, 1); glVertex2f(0.25f, 0.75f); glVertex2f(0.25f, -0.5f); glVertex2f(0.5f, -0.5f);
    glVertex2f(0.5f, -1); glVertex2f(-0.5f, -1); glVertex2f(-0.5f, -0.5f); glVertex2f(-0.25f, -0.5f);
    glVertex2f(-0.25f, 0.75f);
    glEnd();
    glPopMatrix();
}

/* instruments of the cockpit (vgCockPit::updateInstruments) */
static void update_instruments(void)
{
    Plane *u = gUserPlane;
    float jitter = (float)(u->speedfrac * (gRand * 10.0));
    float v;
    v = u->throttle * 0.11f;
    if (v < 0.001f) v = 0;
    vgPartPos(gauge_thrust, 0, 0, v, 0, 0, 0);
    v = (1 - u->damagef) * 216 + jitter;
    if (v < 0.001f) v = 0;
    vgPartPos(gauge_engine, 0, 0, 0, 0, 0, v);
    v = u->speedfrac * 272 + jitter;
    if (v > 272) v = 272;
    if (v < 0.001f) v = 0;
    vgPartPos(gauge_speed, 0, 0, 0, 0, 0, v);
    v = u->fuel * 180 + jitter;
    if (v < 0.001f) v = 0;
    vgPartPos(gauge_fuel, 0, 0, 0, 0, 0, v);
}

static void draw_cockpit_model(vgObject *o1, vgObject *o2)
{
    glViewport(vp_x, vp_y, vp_w, vp_h);
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(36, 45.0 / 36.0, 1, 20);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(-90, 1, 0, 0);
    glTranslatef(0, 1.54f, -0.51f);
    pfDisable(PFEN_LIGHTING);
    pfEnable(PFEN_TEXTURE);
    pfFrameTime(gNow);
    if (o1) {
        glPushMatrix();
        glMultMatrixf(&o1->mat[0][0]);
        pfDrawScene(vgObjModel(o1), gNow);
        glPopMatrix();
    }
    if (o2) {
        glPushMatrix();
        glMultMatrixf(&o2->mat[0][0]);
        pfDrawScene(vgObjModel(o2), gNow);
        glPopMatrix();
    }
    pfEnable(PFEN_LIGHTING);
}

static void draw_cockpit(void)
{
    Plane *u = gUserPlane;
    char buf[64];
    update_instruments();
    vgObjPos(cockpit, rattle_x, rattle_y * 0.25f, 0, 0, 0, 0);
    draw_cockpit_model(cockpit, cockpit_map);

    /* ammo, bombs and score (vgCockPit::postDraw) */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    ortho_cockpit();
    glTranslatef(rattle_x, rattle_y * 0.25f, 0);
    glLineWidth(1.5f);
    glColor3f(0.25f, 0.0862745121f, 0.31764707f);
    vgFontSize(0.007f, 0.01f);
    vgFontPos(0.09f, 0.165f);
    vgDrawFont("AMMO");
    vgFontPos(0.0875f, 0.121f);
    vgDrawFont("BOMBS");
    glColor3f(1, 1, 0);
    vgFontPos(0.485f, 0.123f);
    vgDrawFont("SCORE");
    glColor3f(0.588235319f, 0, 0.588235319f);
    vgFontSize(0.0126f, 0.018f);
    sprintf(buf, "%d", u->ammo);
    vgFontPos(0.0865f, 0.180f);
    vgDrawFont(buf);
    sprintf(buf, "%d", u->bombs);
    vgFontPos(0.0865f, 0.137f);
    vgDrawFont(buf);
    glColor3f(1, 1, 0);
    vgFontSize(0.0154f, 0.022f);
    sprintf(buf, "%d", u->score);
    vgFontPos(0.474f, 0.135f);
    vgDrawFont(buf);
    glLineWidth(1);

    /* the map in the upper right corner */
    if (map_switch && map_switch->switchval == PFSWITCH_ON) {
        float tr[3], planes[20][4], friends[20][4];
        int np, nf, i;
        int x0 = vp_x + (int)(vp_w * 0.735), x1 = vp_x + (int)(vp_w * 0.973);
        int y0 = vp_y + (int)(vp_h * 0.651), y1 = vp_y + (int)(vp_h * 0.956);
        worldMapPositions(tr, &np, planes, &nf, friends);
        glViewport(x0, y0, x1 - x0, y1 - y0);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(-4600, 4500, -3800, 4500);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glColor3f(1, 1, 0);
        map_plane(u->pos[0], u->pos[1], u->pos[2], u->pos[3]);
        glColor3f(1, 0, 1);
        map_train(tr[0], tr[1], tr[2]);
        for (i = 0; i < np; i++) {
            if (u->pos[2] < planes[i][2])
                glColor3f(1, 0.5f, 0);
            else
                glColor3f(1, 0, 0);
            map_plane(planes[i][0], planes[i][1], planes[i][2], planes[i][3]);
        }
        glViewport(vp_x, vp_y, vp_w, vp_h);
    }
    glEnable(GL_DEPTH_TEST);
}

static void draw_selection_overlay(void)
{
    const PlaneType *t;
    char buf[64];
    int idx = 0, i;
    float y;
    for (i = 0; i < 6; i++)
        if (plane_codes[i] == sel_code)
            idx = i;
    t = &gTypes[idx];
    draw_cockpit_model(cockpit_sel, NULL);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    ortho_cockpit();
    glLineWidth(2);
    glColor3f(0, 0, 1);
    draw_box(&sel_boxes[idx]);
    glColor3f(1, 1, 0);
    if (sel_hover < 6)
        draw_box(&sel_boxes[sel_hover]);
    glLineWidth(1.5f);
    vgFontSize(0.0175f, 0.025f);
    glColor3f(0.78f, 0, 0.39f);
    y = 0.75f - 0.0195f;
    vgFontPos(0.06f, y);
    vgDrawFont(" maxspeed");
    vgFontPos(0.06f, y - 0.0195f);
    vgDrawFont(" range");
    vgFontPos(0.06f, y - 2 * 0.0195f);
    vgDrawFont(" ammo");
    vgFontPos(0.06f, y - 3 * 0.0195f);
    vgDrawFont(" agility");
    glColor3f(1, 1, 0);
    vgFontPos(0.06f, 0.75f + 0.0195f * 0.5f);
    vgDrawFont(t->name);
    sprintf(buf, "%3d mph", (int)(t->vmax * 2.23693629));
    vgFontPos(0.21f, y);
    vgDrawFont(buf);
    sprintf(buf, "%3d miles", (int)t->range);
    vgFontPos(0.21f, y - 0.0195f);
    vgDrawFont(buf);
    sprintf(buf, "%3d rounds", t->ammo);
    vgFontPos(0.21f, y - 2 * 0.0195f);
    vgDrawFont(buf);
    sprintf(buf, "%3d %%", (int)(t->agility * 100));
    vgFontPos(0.21f, y - 3 * 0.0195f);
    vgDrawFont(buf);
    glLineWidth(1);
    glEnable(GL_DEPTH_TEST);
}

static void draw_page(void)
{
    float sw, sh;
    glViewport(0, 0, win_w, win_h);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!page_tex)
        return;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, win_w, 0, win_h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    sh = (float)vp_h;
    sw = sh * page_w / page_h;
    if (sw > vp_w) {
        sw = (float)vp_w;
        sh = sw * page_h / page_w;
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, page_tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f((win_w - sw) / 2, (win_h - sh) / 2);
    glTexCoord2f(1, 1); glVertex2f((win_w + sw) / 2, (win_h - sh) / 2);
    glTexCoord2f(1, 0); glVertex2f((win_w + sw) / 2, (win_h + sh) / 2);
    glTexCoord2f(0, 0); glVertex2f((win_w - sw) / 2, (win_h + sh) / 2);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
}

static void display(void)
{
    if (page >= 0) {
        draw_page();
        glutSwapBuffers();
        return;
    }
    glViewport(0, 0, win_w, win_h);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_main();
    if (playing) {
        if (gLayout == 0)
            draw_cockpit();
    } else {
        draw_selection_overlay();
    }
    glutSwapBuffers();
}

/* --- intro pages (mardi::drawIntroScreen) -------------------------------------------- */

static const char *page_files[] = { "intro.jpg", "instruct1.jpg", "instruct2.jpg", "instruct3.jpg" };

static void load_page(int p)
{
    char path[2048];
    unsigned char *img;
    if (page_tex) {
        glDeleteTextures(1, &page_tex);
        page_tex = 0;
    }
    page = p;
    page_time = pfGetTime();
    if (!vgFindFile(page_files[p], path, sizeof(path)) || !(img = jpegLoad(path, &page_w, &page_h)))
        return;
    glGenTextures(1, &page_tex);
    glBindTexture(GL_TEXTURE_2D, page_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, page_w, page_h, GL_RGB, GL_UNSIGNED_BYTE, img);
    free(img);
}

static void next_page(void)
{
    if (page < 0)
        return;
    if (instruct && page < 3) {
        load_page(page + 1);
    } else {
        if (page_tex)
            glDeleteTextures(1, &page_tex);
        page_tex = 0;
        page = -1;
    }
}

/* --- GLUT callbacks ---------------------------------------------------------------- */

static void reshape(int w, int h)
{
    win_w = w;
    win_h = h > 0 ? h : 1;
    /* the game was made for 1280x1024: a 5:4 area centered in the window */
    if (win_w * 4 > win_h * 5) {
        vp_h = win_h;
        vp_w = win_h * 5 / 4;
    } else {
        vp_w = win_w;
        vp_h = win_w * 4 / 5;
    }
    vp_x = (win_w - vp_w) / 2;
    vp_y = (win_h - vp_h) / 2;
}

static void idle(void)
{
#ifdef CI_DEBUG
    static int frames;
    static double fps_t;
    Plane *u = gUserPlane;
#endif
    if (!update_time())
        return;
#ifdef CI_DEBUG
    if (u && ++frames % 30 == 0) {
        fprintf(stderr, "fps %.1f state %d pos %.0f %.0f %.1f hpr %.0f %.0f %.0f thr %.2f sf %.2f stick %.3f "
                "dmg %d score %d ammo %d\n", 30 / (gNow - fps_t), u->state, u->pos[0], u->pos[1], u->pos[2],
                u->pos[3], u->pos[4], u->pos[5], u->throttle, u->speedfrac, u->stick_y, u->damage, u->score,
                u->ammo);
        fps_t = gNow;
    }
#endif
    if (page >= 0) {
        if (pfGetTime() - page_time > 17)
            next_page();
    } else {
        if (!playing) {
            float x, y;
            x = ((mouse_x - vp_x) / (float)vp_w) * 2 - 1;
            y = (((win_h - mouse_y) - vp_y) / (float)vp_h) * 2 - 1;
            sel_input(x, y, sel_clicked);
            sel_clicked = 0;
            if (sel_start)
                play_game();
        }
        update_frame();
    }
    glutPostRedisplay();
}

static void keyboard(unsigned char k, int x, int y)
{
    (void)x; (void)y;
    if (page >= 0) {
        if (k == 27)
            exit(0);
        next_page();
        return;
    }
    switch (k) {
    case 27:
        exit(0);
    case ' ':
        if (playing)
            bombsTrigger();
        break;
    case 'a': case 'A': case '+': case '=':
        key_up = 1;
        break;
    case 'z': case 'Z': case '-': case 'w': case 'W':
        key_down = 1;
        break;
    case 'x': case 'X': case 13: case '\t':
        key_fire = 1;
        break;
    case 'f': case 'F':
        fog = !fog;
        break;
    case 'm': case 'M':
        if (playing && gShowMap >= 0 && map_switch)
            pfSwitchVal(map_switch, map_switch->switchval == PFSWITCH_ON ? PFSWITCH_OFF : PFSWITCH_ON);
        break;
    }
}

static void keyboard_up(unsigned char k, int x, int y)
{
    (void)x; (void)y;
    switch (k) {
    case 'a': case 'A': case '+': case '=':
        key_up = 0;
        break;
    case 'z': case 'Z': case '-': case 'w': case 'W':
        key_down = 0;
        break;
    case 'x': case 'X': case 13: case '\t':
        key_fire = 0;
        break;
    }
}

static void special(int k, int x, int y)
{
    (void)x; (void)y;
    if (page >= 0) {
        next_page();
        return;
    }
    switch (k) {
    case GLUT_KEY_F1:
        if (playing && gLayout < 3)
            toggle_layout();
        break;
    case GLUT_KEY_F2:
        if (playing) {
            if (tether_scale == 1) tether_scale = 5;
            else if (tether_scale == 5) tether_scale = 10;
            else if (tether_scale == 10) tether_scale = 15;
            else if (tether_scale == 15) tether_scale = 20;
            else tether_scale = 1;
            ciChangeLayout(gLayout);
        }
        break;
    case GLUT_KEY_F3:
        if (playing && map_switch)
            pfSwitchVal(map_switch, map_switch->switchval == PFSWITCH_ON ? PFSWITCH_OFF : PFSWITCH_ON);
        break;
    case GLUT_KEY_F9:
        if (playing)
            select_game_params();
        else
            play_game();
        break;
    case GLUT_KEY_PAGE_UP:
        key_up = 1;
        break;
    case GLUT_KEY_PAGE_DOWN:
        key_down = 1;
        break;
    }
}

static void special_up(int k, int x, int y)
{
    (void)x; (void)y;
    if (k == GLUT_KEY_PAGE_UP)
        key_up = 0;
    else if (k == GLUT_KEY_PAGE_DOWN)
        key_down = 0;
}

static void mouse(int b, int state, int x, int y)
{
    int down = state == GLUT_DOWN;
    mouse_x = x;
    mouse_y = y;
    mouse_in = 1;
    if (page >= 0) {
        if (down)
            next_page();
        return;
    }
    if (b == GLUT_LEFT_BUTTON) {
        btn_left = down;
        if (down && !playing)
            sel_clicked = 1;
    } else if (b == GLUT_RIGHT_BUTTON) {
        btn_right = down;
    } else if (b == GLUT_MIDDLE_BUTTON) {
        btn_middle = down;
    }
}

static void motion(int x, int y)
{
    mouse_x = x;
    mouse_y = y;
    mouse_in = 1;
}

static void entry(int state)
{
    if (state == GLUT_LEFT)
        btn_left = btn_right = btn_middle = 0;
}

/* --- configuration (mardi::postConfig) --------------------------------------------- */

static void configure(void)
{
    gMainScene = vgFindScene("MainScene");
    gCockpitScene = vgFindScene("MainCockPitScene");
    gSelScene = vgFindScene("SelScene");
    gSelCockpitScene = vgFindScene("SelCockPitScene");

    worldConfigure();

    cockpit = vgFindObj("CockPit");
    cockpit_map = vgFindObj("CockPitMap");
    if (!cockpit || !cockpit_map) {
        fprintf(stderr, "Cannot find cockpit!\n");
        exit(1);
    }
    vgObjModel(cockpit);
    vgObjModel(cockpit_map);
    gauge_thrust = vgFindPart(cockpit, "thrust", 0);
    gauge_engine = vgFindPart(cockpit, "engine", 0);
    gauge_speed = vgFindPart(cockpit, "speed", 0);
    gauge_fuel = vgFindPart(cockpit, "fuel", 0);
    {
        pfNode *p = vgFindPart(cockpit, "oneprop", 0);
        if (p)
            pfSeqMode(p, PFSEQ_STOP);
    }
    map_switch = vgFindPart(cockpit_map, "map", 0);
    if (map_switch)
        pfSwitchVal(map_switch, PFSWITCH_ON);
    sel_init();

    /* weapons */
    gGunBank = ammoBankNew(12, 1500, 100, 0xfffefff8, 1, 1, 0, 0, NULL);
    gBombBank = ammoBankNew(4, 1000, 35, 0xfffeffff, 0, 0, 0, 0, vgFindObj("Bomb"));
    gEnemyBank = ammoBankNew(10, 1000, 50, CL_USER, 1, 1, 0.5f, 0, NULL);
    gFlakBank = ammoBankNew(6, 1500, 0, 0, 0, 0, 0, 0, vgFindObj("GroundFire"));
    bank_init(&flak_bank, 5, "Flak", 0.5f);
    bank_init(&small_bank, 4, "SmallGndExplosion", 0.75f);
    bank_init(&big_bank, 2, "BigGndExplosion", 0.75f);
    bank_init(&air_bank, 5, "AirExplosion", 0.7f);

    sel_place_planes();
    ciChangeLayout(4);
}

int main(int argc, char **argv)
{
    const char *adf = "cimpact.adf", *dir = NULL;
    int i;
    glutInit(&argc, argv);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-noinstruct") == 0)
            instruct = 0;
        else if (strcmp(argv[i], "-nofog") == 0)
            fog = 0;
        else if (strcmp(argv[i], "-nosound") == 0 || strcmp(argv[i], "-nonetwork") == 0)
            ;
        else if (strstr(argv[i], ".adf"))
            adf = argv[i];
        else
            dir = argv[i];
    }
    if (dir) {
        vgDataDir(dir);
    } else {
        /* the default directory, or the original distribution next to the program */
        static const char *dirs[] = { "data/cimpact", "Certain", "." };
        char path[2048];
        for (i = 0; i < 3; i++) {
            vgDataDir(dirs[i]);
            if (vgFindFile(adf, path, sizeof(path)))
                break;
        }
        if (i == 3)
            vgDataDir(dirs[0]);
    }
    srand(12345);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(win_w, win_h);
    glutCreateWindow("Certain Impact - MardiGras WWI Adventure");
    reshape(win_w, win_h);

    /* the intro page while loading */
    load_page(0);
    draw_page();
    glutSwapBuffers();
    draw_page();
    glutSwapBuffers();

    if (vgLoadADF(adf) < 0) {
        fprintf(stderr, "cimpact: cannot load %s (data directory: %s)\n", adf, dir ? dir : "data/cimpact");
        return 1;
    }
    configure();
    start_time = pfGetTime();
    last_time = 0;
    page_time = pfGetTime();
    if (!page_tex)
        page = -1;

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboard_up);
    glutSpecialFunc(special);
    glutSpecialUpFunc(special_up);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutPassiveMotionFunc(motion);
    glutEntryFunc(entry);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
