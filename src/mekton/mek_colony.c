/*
 * mek_colony.c - the space colony: a cylinder of radius 4900 turning
 * around the X axis. Colony coordinates are (x along the axis, distance
 * along the circumference, height above the ground).
 */
#include <math.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "mekton.h"

#define COLONY_RADIUS 4900.0f
#define DEG_PER_UNIT 0.0116930166f   /* 360 / (2 pi 4900) */
#define UNIT_PER_DEG 85.5211334f

static pfLightSource *sun, *fill;
static float light_level = 1.0f;

void update_colony(void)
{
    float f, w[2];
    cd_app->colony_angle += 1.63636363f / tps;
    if (360.0f < cd_app->colony_angle)
        cd_app->colony_angle -= 360.0f;
    position_lights(cd_app->colony_angle);
    update_doors();
    /* the waterfall scrolls with a morph between two texture offsets */
    f = game_time / 5.0f;
    w[1] = f - (float)(int)f;
    w[0] = 1.0f - w[1];
    pfMorphWeights(waterfall->model, 0, w);
}

void colony2world(const pfVec3 cpos, const pfVec3 chpr, const pfVec3 cvec,
                  pfVec3 pos, pfVec3 hpr, pfVec3 vec)
{
    float a = cpos[1] * DEG_PER_UNIT;
    pfMatrix m;
    if (pos) {
        float r = COLONY_RADIUS - cpos[2], s, c;
        pos[0] = cpos[0];
        pfSinCos(a, &s, &c);
        pos[1] = s * r;
        pos[2] = c * -r;
    }
    if (hpr) {
        pfMakeEulerMat(m, chpr[0], chpr[1], chpr[2]);
        matrix_post_rotate(m, a, 'x');
        mat2hpr(m, hpr);
    }
    if (vec) {
        identify_matrix(m);
        matrix_post_rotate(m, a, 'x');
        pfXformVec3(vec, cvec, m);
    }
}

void world2colony(const pfVec3 pos, const pfVec3 hpr, const pfVec3 vec,
                  pfVec3 cpos, pfVec3 chpr, pfVec3 cvec)
{
    float a = pfArcTan2(pos[1], -pos[2]);
    pfMatrix m;
    if (cpos) {
        cpos[0] = pos[0];
        cpos[2] = COLONY_RADIUS - pfSqrt(pos[2] * pos[2] + pos[1] * pos[1]);
        cpos[1] = a * UNIT_PER_DEG;
    }
    if (chpr) {
        pfMakeEulerMat(m, hpr[0], hpr[1], hpr[2]);
        matrix_post_rotate(m, -a, 'x');
        mat2hpr(m, chpr);
    }
    if (cvec) {
        identify_matrix(m);
        matrix_post_rotate(m, -a, 'x');
        pfXformVec3(cvec, vec, m);
    }
}

/* --- lights ------------------------------------------------------------- */

void init_lighting(void)
{
    static const float a[2][3] = { { 0.15f, 0.1f, 0.04f }, { 0.75f, 0.5f, 0.2f } };
    static const float b[2][3] = { { 0.06f, 0.06f, 0.06f }, { 0.3f, 0.3f, 0.5f } };
    float ca[2][3], cb[2][3];
    int i, j;

    if (!sun) {
        sun = pfNewLSource();
        pfAddChild(scene, pfLSourceNode(sun));
        fill = pfNewLSource();
        pfAddChild(scene, pfLSourceNode(fill));
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3; j++) {
            ca[i][j] = a[i][j] * light_level;
            if (1 < ca[i][j]) ca[i][j] = 1;
            cb[i][j] = b[i][j] * light_level;
            if (1 < cb[i][j]) cb[i][j] = 1;
        }
    }
    pfLSourceColor(sun, PFLT_AMBIENT, ca[0][0], ca[0][1], ca[0][2]);
    pfLSourceColor(sun, PFLT_DIFFUSE, ca[1][0], ca[1][1], ca[1][2]);
    pfLSourceColor(sun, PFLT_SPECULAR, ca[1][0], ca[1][1], ca[1][2]);
    pfLSourcePos(sun, 1, 0, 0, 0);
    pfLSourceOn(sun);
    pfLSourceColor(fill, PFLT_AMBIENT, cb[0][0], cb[0][1], cb[0][2]);
    pfLSourceColor(fill, PFLT_DIFFUSE, cb[1][0], cb[1][1], cb[1][2]);
    pfLSourceColor(fill, PFLT_SPECULAR, cb[1][0], cb[1][1], cb[1][2]);
    pfLSourcePos(fill, -0.3f, 1, 0, 0);
    pfLSourceOn(fill);
}

/* the lights turn with the colony */
void position_lights(float angle)
{
    float s, c;
    pfSinCos(angle, &s, &c);
    pfLSourcePos(sun, 0, c, s, 0);
    pfLSourcePos(fill, 0.8f, -c, s, 0);
}

/* V brighter, shift V darker, ctrl V reset */
void light_adjust(void)
{
    if (inp.ctrl) {
        light_level = 1;
    } else if (inp.shift) {
        light_level -= 0.1f;
        if (light_level < 0)
            light_level = 0;
    } else {
        light_level += 0.1f;
        if (2 < light_level)
            light_level = 2;
    }
    init_lighting();
}

/* --- background --------------------------------------------------------- */

static pfGeoSet *stars;

static void init_stars(void)
{
    float *pos = (float *)pfMalloc(sizeof(float) * 3 * 1024, NULL);
    float *col = (float *)pfMalloc(sizeof(float) * 4 * 1024, NULL);
    int i;
    for (i = 0; i < 1024; i++) {
        float g;
        pos[3 * i] = (float)rand() / RAND_MAX - 0.5f;
        pos[3 * i + 1] = (float)rand() / RAND_MAX - 0.5f;
        pos[3 * i + 2] = (float)rand() / RAND_MAX - 0.5f;
        pfNormalizeVec3(pos + 3 * i);
        pfScaleVec3(pos + 3 * i, 19000.0f, pos + 3 * i);
        g = (float)rand() / RAND_MAX * 0.8f + 0.2f;
        col[4 * i] = col[4 * i + 1] = col[4 * i + 2] = g;
        col[4 * i + 3] = 1;
    }
    stars = pfNewGSet(NULL);
    pfGSetNumPrims(stars, 1024);
    pfGSetPrimType(stars, PFGS_POINTS);
    pfGSetAttr(stars, PFGS_COORD3, PFGS_PER_VERTEX, pos, NULL);
    pfGSetAttr(stars, PFGS_COLOR4, PFGS_PER_VERTEX, col, NULL);
    pfGSetPntSize(stars, 2);
}

void draw_gas_giant(void);

void init_background(void)
{
    init_stars();
}

/* the stars (and the gas giant) seen through the windows of the colony */
void draw_background_scene(int state)
{
    if (!stars)
        init_background();
    if (state != STATE_PLAY)
        return;
    pfPushState();
    pfDisable(PFEN_LIGHTING);
    pfDisable(PFEN_TEXTURE);
    glPushMatrix();
    glRotatef(cd_app->colony_angle, 1, 0, 0);
    pfDrawGSet(stars);
    pfEnable(PFEN_TEXTURE);
    draw_gas_giant();
    glPopMatrix();
    pfPopState();
}
