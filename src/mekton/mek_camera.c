/*
 * mek_camera.c - the eight camera modes of Mekton (F1 .. F7 and the
 * dropped camera), and the Z, X, C keys that zoom, spin and raise them.
 */
#include <math.h>

#include "mekton.h"

#define HIST 31

pfCoord camera;
int camera_mode, new_camera_mode;

static struct { pfVec3 pos, hpr; } hist[HIST];
static int hist_idx;
static float zoom;              /* -1000 resets the zoom */
static float height_rate, spin_rate;
static int wingman_back = 15, follow_back = 15;
static float spin_speed = 1, spin_dist = 20, spin_height = 7, tether_dist = 20;
static float tether_height, tether_angle, spin_angle;
static int watch_count, watch_point;
static float fov_in_mek, fov_dropped;
static pfVec3 dropped_pos;

static const pfVec3 eye_offset[2] = { { 0, 1, 5.5f }, { 0, 1, 4 } };
static const pfVec3 watch_points[4] = {
    { -88.92f, 388.55f, -4518.82f },
    { 183.92f, -364.11f, -4618.33f },
    { -164.05f, 1050.54f, -4680.39f },
    { 124.37f, 6.11f, -4812.38f }
};

void init_camera_hist(void)
{
    int i;
    for (i = 0; i < HIST; i++) {
        pfCopyVec3(hist[i].pos, my_mek->pos);
        pfCopyVec3(hist[i].hpr, my_mek->hpr);
    }
    hist_idx = 0;
}

void init_camera(void)
{
    init_camera_hist();
    pfCopyVec3(dropped_pos, my_mek->cpos);
    camera_mode = -1;
    new_camera_mode = 0;
    fov_in_mek = fov;
    fov_dropped = fov;
}

void reset_cameras(void)
{
    fov_in_mek = fov;
    wingman_back = 15;
    follow_back = 15;
    spin_speed = 1;
    spin_dist = 20;
    spin_height = 7;
    tether_dist = 20;
    tether_height = 0;
    tether_angle = 0;
    fov_dropped = fov;
}

static void reset_camera_zoom(void)
{
    switch (camera_mode) {
    case 0: case 7: zoom = -1000; break;
    case 1: wingman_back = 15; break;
    case 2: follow_back = 15; break;
    case 4: tether_dist = 20; break;
    case 5: spin_dist = 20; break;
    }
}

static void reset_camera_spin(void)
{
    if (camera_mode == 4)
        tether_angle = 0;
}

static void reset_camera_height(void)
{
    if (camera_mode == 4)
        tether_height = 0;
    else if (camera_mode == 5)
        spin_height = 7;
}

/* Z zooms in, shift Z out, ctrl Z resets */
void cc_zoom(void)
{
    if (inp.ctrl) reset_camera_zoom();
    else zoom = inp.shift ? 1.0f : -1.0f;
}
void cc_zoom_stop(void) { zoom = 0; }

/* X spins, shift X the other way, ctrl X resets */
void cc_spin(void)
{
    if (inp.ctrl) reset_camera_spin();
    else spin_rate = inp.shift ? -1.0f : 1.0f;
}
void cc_spin_stop(void) { spin_rate = 0; }

/* C raises the camera, shift C lowers it, ctrl C resets */
void cc_height(void)
{
    if (inp.ctrl) reset_camera_height();
    else height_rate = inp.shift ? -0.5f : 0.5f;
}
void cc_height_stop(void) { height_rate = 0; }

void cm_set(int mode) { new_camera_mode = mode; }

/* zoom of the cameras that change the field of view */
static void zoom_fov(float *f)
{
    if (zoom == 0)
        return;
    if (zoom == -1000) {
        zoom = 0;
        *f = fov;
    } else {
        *f += zoom * 1.6f;
        if (*f < 5) *f = 5;
        if (90 < *f) *f = 90;
    }
    chan_fov = *f;
    cd_app->fov_changed = 1;
}

static void place_eye(const Mek *m)
{
    pfMatrix mat;
    pfMakeCoordMat(mat, &camera);
    pfXformPt3(camera.xyz, eye_offset[m->type], mat);
}

/* looks from the colony position from at the colony position to */
static void set_camera(const pfVec3 from, const pfVec3 to)
{
    pfVec3 d, hp;
    pfSubVec3(d, to, from);
    pfNormalizeVec3(d);
    path2hp(d, hp);
    hp[2] = 0;
    colony2world(from, hp, NULL, camera.xyz, camera.hpr, NULL);
}

static void camera_in_mek(Mek *m)
{
    if (camera_mode == 0)
        zoom_fov(&fov_in_mek);
    if (m->status == 2) {
        pfSetVec3(camera.xyz, 1000000, 1000000, 1000000);
        return;
    }
    pfXformPt3(camera.xyz, eye_offset[m->type], mek_mat);
    pfCopyVec3(camera.hpr, m->hpr);
}

static float wrap180(float a)
{
    if (180 < a) return a - 360;
    if (a < -180) return a + 360;
    return a;
}

/* position along the path of the mech, back steps of 5 units behind */
static int trail(int back, float f)
{
    int a = hist_idx - back, b, k;
    if (a < 0)
        a += HIST;
    b = a + 1 == HIST ? 0 : a + 1;
    for (k = 0; k < 3; k++)
        camera.xyz[k] = hist[b].pos[k] * f + (1 - f) * hist[a].pos[k];
    for (k = 0; k < 3; k++)
        camera.hpr[k] = wrap180(hist[b].hpr[k] - hist[a].hpr[k]) * f + hist[a].hpr[k];
    return a;
}

static void step_back(int *back)
{
    if (zoom == 0)
        return;
    if (0 < zoom) {
        if (++*back == HIST)
            *back = HIST - 1;
    } else if (--*back < 0) {
        *back = 0;
    }
}

static void camera_wingman(Mek *m, float d)
{
    step_back(&wingman_back);
    if (!wingman_back) {
        camera_in_mek(m);
        return;
    }
    trail(wingman_back, d / 5.0f);
    place_eye(m);
}

static void camera_follow(Mek *m, float d)
{
    pfVec3 c;
    step_back(&follow_back);
    if (!follow_back) {
        camera_in_mek(m);
        return;
    }
    trail(follow_back, d / 5.0f);
    if (camera.xyz[0] == m->pos[0] && camera.xyz[1] == m->pos[1] && camera.xyz[2] == m->pos[2]) {
        camera_in_mek(m);
        return;
    }
    world2colony(camera.xyz, NULL, NULL, c, NULL, NULL);
    if (15393.8037f < c[1] - m->cpos[1])
        c[1] -= 30787.6074f;
    else if (c[1] - m->cpos[1] < -15393.8037f)
        c[1] += 30787.6074f;
    set_camera(c, m->cpos);
    place_eye(m);
}

/* fixed cameras around the arena that take turns every three seconds */
static void camera_watch(Mek *m)
{
    pfVec3 v, up, s;
    if (int_tps * 3 < ++watch_count) {
        watch_count = 0;
        if (++watch_point == 4)
            watch_point = 0;
    }
    pfCopyVec3(camera.xyz, watch_points[watch_point]);
    pfSubVec3(v, m->pos, camera.xyz);
    pfSetVec3(up, 0, -camera.xyz[1], -camera.xyz[2]);
    pfCrossVec3(s, up, v);
    pfCrossVec3(s, v, s);
    pfNormalizeVec3(s);
    pfNormalizeVec3(v);
    path_norm2hpr(v, s, camera.hpr);
}

static void orbit(Mek *m, float angle, float *dist, float height)
{
    pfVec3 c;
    float s, co;
    if (zoom != 0) {
        *dist += zoom * 0.8f;
        if (*dist < 10) *dist = 10;
        if (80 < *dist) *dist = 80;
    }
    pfSinCos(angle, &s, &co);
    c[0] = m->cpos[0] + co * *dist;
    c[1] = m->cpos[1] - s * *dist;
    c[2] = m->cpos[2] + height;
    set_camera(c, m->cpos);
    place_eye(m);
}

/* a camera tied behind the mech */
static void camera_tether(Mek *m)
{
    float a;
    tether_angle += 4.0f * spin_rate;
    a = tether_angle - m->chpr[0];
    if (360 < a)
        a -= 360;
    tether_height += height_rate;
    orbit(m, a, &tether_dist, tether_height);
}

/* a camera turning around the mech */
static void camera_spin(Mek *m)
{
    if (spin_rate != 0)
        spin_speed = spin_rate;
    spin_angle += spin_speed + spin_speed;
    if (360 < spin_angle)
        spin_angle -= 360;
    spin_height += height_rate;
    orbit(m, spin_angle, &spin_dist, spin_height);
}

/* F6 leaves a camera where the mech stands */
static void camera_drop(Mek *m)
{
    new_camera_mode = 7;
    pfCopyVec3(dropped_pos, m->cpos);
}

static void camera_dropped(Mek *m)
{
    zoom_fov(&fov_dropped);
    if (dropped_pos[0] == m->cpos[0] && dropped_pos[1] == m->cpos[1] && dropped_pos[2] == m->cpos[2])
        return;
    set_camera(dropped_pos, m->cpos);
    place_eye(m);
}

void update_camera(Mek *m)
{
    float d;
    int k;

    if (new_camera_mode != camera_mode) {
        camera_mode = new_camera_mode;
        if (camera_mode == 0) {
            chan_fov = fov_in_mek;
            if (!shadow || number_mektons > 0)
                pfSwitchVal(my_mek->obj->sw, PFSWITCH_OFF);
        } else {
            chan_fov = camera_mode == 7 ? fov_dropped : fov;
            if (!shadow || number_mektons > 0)
                pfSwitchVal(my_mek->obj->sw, PFSWITCH_ON);
        }
        cd_app->fov_changed = 1;
    } else {
        cd_app->fov_changed = 0;
    }

    /* keep a trail of the mech every 5 units */
    d = range(m->pos, hist[hist_idx].pos);
    while (5 <= d) {
        float f = 5 / d;
        int n = hist_idx + 1 == HIST ? 0 : hist_idx + 1;
        for (k = 0; k < 3; k++)
            hist[n].pos[k] = hist[hist_idx].pos[k] * (1 - f) + f * m->pos[k];
        for (k = 0; k < 3; k++)
            hist[n].hpr[k] = wrap180(hist[hist_idx].hpr[k] - m->hpr[k]) * (1 - f) + m->hpr[k];
        hist_idx = n;
        d -= 5;
    }

    switch (camera_mode) {
    case 0: camera_in_mek(m); break;
    case 1: camera_wingman(m, d); break;
    case 2: camera_follow(m, d); break;
    case 3: camera_watch(m); break;
    case 4: camera_tether(m); break;
    case 5: camera_spin(m); break;
    case 6: camera_drop(m); break;
    case 7: camera_dropped(m); break;
    }
    if (shadow && !number_mektons) {
        pfSetVec3(camera.xyz, 0, 1000, 0);
        pfSetVec3(camera.hpr, 0, 0, 0);
    }
    pfCopyVec3(cd_app->eye, camera.xyz);
}
