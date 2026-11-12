/*
 * effects.c - the visual effects of the F-18 demo
 *
 * The demo has three systems, all drawn with the texture steam.inta (an
 * intensity and alpha puff):
 *
 *  - the effects of esAddEffect(type, position, scale), kept in an array of
 *    20 per channel and updated by the module "effects" (checkEffects,
 *    updateExplosion, updateDebris, updateFlak, updateWhiteFlash): 1 an
 *    explosion (a ball of puffs growing and fading from yellow to dark
 *    smoke, its puffs moving out), 2 debris thrown up and falling, 3 flak,
 *    4 a white flash of the screen.  The demo adds 1 and 2 together: scale
 *    2 and 1 when a missile explodes or an airplane crashes, 0.5 and 1
 *    while an airplane hit falls, and 4 (scale 3) when the player crashes;
 *  - the fumes of esAddFume: puffs left behind an object, from a size to
 *    another in a number of frames (esSetFumeAttr 0, 1 and 3): the exhaust
 *    of the missiles (1, 10, 75) and the black smoke of the airplanes hit
 *    (1, 10, 100);
 *  - the smoke of esAddSmoke: a column rising from the wreck of an airplane,
 *    as large as twice the airplane, of density 0.8.
 *
 * The constants of the explosions (sizes, speeds, durations) are read in
 * double precision by the IRIX code and are not all recovered: the ones
 * below are chosen to look like the demo.
 */
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "effects.h"

#define MAX_PARTICLES 6000
#define MAX_FUMES 16
#define MAX_SMOKES 16

typedef struct Particle {
    float pos[3], vel[3];
    float size0, size1;
    float age, life;            /* seconds */
    float c0[4], cm[4], c1[4];  /* colors at the start, at mid, at the end */
    float mid;                  /* the fraction of the life at mid color */
    float gravity, drag;
    float rot, spin;
    int debris;                 /* a shard, not a puff */
} Particle;

typedef struct Fume {
    esObject *obj;
    int used, on;
    float rgb[3];
    float size0, size1, life;
    float last[3];
    int has_last;
} Fume;

typedef struct Smoke {
    esObject *obj;
    int used;
    float size, density;
    int count;
} Smoke;

static Particle parts[MAX_PARTICLES];
static int nparts;
static Fume fumes[MAX_FUMES];
static Smoke smokes[MAX_SMOKES];
static float flash_time, flash_len;
static pfTexture *puff_tex;
static unsigned int fallback_tex;
static int inited;

static float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }

static Particle *new_particle(void)
{
    Particle *p;
    if (nparts >= MAX_PARTICLES)
        return NULL;
    p = &parts[nparts++];
    memset(p, 0, sizeof(*p));
    p->mid = 0.5f;
    return p;
}

static void set4(float *c, float r, float g, float b, float a)
{
    c[0] = r; c[1] = g; c[2] = b; c[3] = a;
}

static void random_dir(float d[3], float up_bias)
{
    float l;
    do {
        d[0] = frand(-1, 1);
        d[1] = frand(-1, 1);
        d[2] = frand(-1, 1) + up_bias;
        l = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    } while (l < 1e-4f || l > (1 + up_bias) * (1 + up_bias));
    l = (float)sqrt(l);
    d[0] /= l; d[1] /= l; d[2] /= l;
}

/* --- the effects of esAddEffect ----------------------------------------------- */

static void explosion(const float pos[3], float scale)
{
    int i, k;
    /* the fire ball */
    for (i = 0; i < 24; i++) {
        Particle *p = new_particle();
        float d[3], v = frand(6, 18) * scale;
        if (!p)
            return;
        random_dir(d, 0.2f);
        for (k = 0; k < 3; k++) {
            p->pos[k] = pos[k] + d[k] * frand(0, 2) * scale;
            p->vel[k] = d[k] * v;
        }
        p->drag = 1.5f;
        p->size0 = frand(3, 5) * scale;
        p->size1 = frand(10, 16) * scale;
        p->life = frand(1.2f, 2.2f);
        p->mid = 0.3f;
        set4(p->c0, 1, 0.95f, 0.6f, 1);
        set4(p->cm, 1, 0.45f, 0.1f, 0.9f);
        set4(p->c1, 0.2f, 0.18f, 0.16f, 0);
        p->rot = frand(0, 360);
        p->spin = frand(-60, 60);
    }
    /* the smoke it leaves */
    for (i = 0; i < 10; i++) {
        Particle *p = new_particle();
        float d[3];
        if (!p)
            return;
        random_dir(d, 0.5f);
        for (k = 0; k < 3; k++) {
            p->pos[k] = pos[k] + d[k] * 3 * scale;
            p->vel[k] = d[k] * 4 * scale;
        }
        p->vel[2] += 3;
        p->drag = 0.5f;
        p->size0 = 6 * scale;
        p->size1 = 22 * scale;
        p->life = frand(3, 5);
        p->mid = 0.2f;
        set4(p->c0, 0.3f, 0.28f, 0.25f, 0);
        set4(p->cm, 0.25f, 0.23f, 0.22f, 0.7f);
        set4(p->c1, 0.3f, 0.3f, 0.3f, 0);
        p->rot = frand(0, 360);
        p->spin = frand(-20, 20);
    }
}

static void debris(const float pos[3], float scale)
{
    int i, k;
    for (i = 0; i < 16; i++) {
        Particle *p = new_particle();
        float d[3], v = frand(20, 50) * scale;
        if (!p)
            return;
        random_dir(d, 0.8f);
        for (k = 0; k < 3; k++) {
            p->pos[k] = pos[k];
            p->vel[k] = d[k] * v;
        }
        p->gravity = 9.8f;
        p->debris = 1;
        p->size0 = p->size1 = frand(0.6f, 1.6f) * scale;
        p->life = frand(2, 3.5f);
        p->mid = 0.8f;
        set4(p->c0, 0.15f, 0.14f, 0.13f, 1);
        set4(p->cm, 0.15f, 0.14f, 0.13f, 1);
        set4(p->c1, 0.15f, 0.14f, 0.13f, 0);
        p->rot = frand(0, 360);
        p->spin = frand(-400, 400);
    }
}

static void flak(const float pos[3], float scale)
{
    int i, k;
    for (i = 0; i < 8; i++) {
        Particle *p = new_particle();
        float d[3];
        if (!p)
            return;
        random_dir(d, 0);
        for (k = 0; k < 3; k++) {
            p->pos[k] = pos[k] + d[k] * 2 * scale;
            p->vel[k] = d[k] * 3 * scale;
        }
        p->drag = 1;
        p->size0 = 2 * scale;
        p->size1 = 8 * scale;
        p->life = frand(2, 3);
        p->mid = 0.1f;
        set4(p->c0, 1, 0.8f, 0.4f, 1);
        set4(p->cm, 0.1f, 0.1f, 0.1f, 0.9f);
        set4(p->c1, 0.15f, 0.15f, 0.15f, 0);
    }
}

void fxAddEffect(int type, const float pos[3], float scale)
{
    if (!inited)
        fxInit();
    switch (type) {
    case FX_EXPLOSION: explosion(pos, scale); break;
    case FX_DEBRIS: debris(pos, scale); break;
    case FX_FLAK: flak(pos, scale); break;
    case FX_FLASH:
        flash_len = 0.15f * scale;
        flash_time = flash_len;
        break;
    default:
        fprintf(stderr, "Error: Unknown Effects type requested....(%d)\n", type);
        break;
    }
}

/* --- fumes and smoke ------------------------------------------------------------- */

int fxAddFume(esObject *obj, float r, float g, float b, float size0, float size1, float life)
{
    int i;
    for (i = 0; i < MAX_FUMES; i++)
        if (!fumes[i].used) {
            Fume *f = &fumes[i];
            memset(f, 0, sizeof(*f));
            f->used = 1;
            f->on = 1;
            f->obj = obj;
            f->rgb[0] = r; f->rgb[1] = g; f->rgb[2] = b;
            f->size0 = size0;
            f->size1 = size1;
            f->life = life;
            return i;
        }
    return -1;
}

void fxFumeOn(int fume, int on)
{
    if (fume >= 0 && fume < MAX_FUMES && fumes[fume].used) {
        fumes[fume].on = on;
        fumes[fume].has_last = 0;
    }
}

void fxFumeColor(int fume, float r, float g, float b)
{
    if (fume >= 0 && fume < MAX_FUMES) {
        fumes[fume].rgb[0] = r;
        fumes[fume].rgb[1] = g;
        fumes[fume].rgb[2] = b;
    }
}

int fxAddSmoke(esObject *obj, float size, float density)
{
    int i;
    for (i = 0; i < MAX_SMOKES; i++)
        if (!smokes[i].used) {
            smokes[i].used = 1;
            smokes[i].obj = obj;
            smokes[i].size = size;
            smokes[i].density = density;
            smokes[i].count = 0;
            return i;
        }
    return -1;
}

void fxRemoveSmoke(int smoke)
{
    if (smoke >= 0 && smoke < MAX_SMOKES)
        smokes[smoke].used = 0;
}

static void emit_fume(Fume *f, float dt)
{
    float d[3], dist;
    int n, i, k;
    const float *cur = f->obj->xyz;
    if (!f->has_last) {
        memcpy(f->last, cur, sizeof(f->last));
        f->has_last = 1;
    }
    for (k = 0; k < 3; k++)
        d[k] = cur[k] - f->last[k];
    dist = (float)sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    /* a puff every few meters, so that a fast missile leaves a line */
    n = (int)(dist / (f->size0 * 3 + 1)) + 1;
    if (n > 4)
        n = 4;
    for (i = 0; i < n; i++) {
        Particle *p = new_particle();
        float t = (i + 1) / (float)n;
        if (!p)
            return;
        for (k = 0; k < 3; k++) {
            p->pos[k] = f->last[k] + d[k] * t + frand(-0.3f, 0.3f) * f->size0;
            p->vel[k] = frand(-0.5f, 0.5f);
        }
        p->vel[2] += 0.5f;
        p->size0 = f->size0;
        p->size1 = f->size1;
        p->life = f->life * dt * frand(0.8f, 1.2f);
        p->mid = 0.1f;
        set4(p->c0, f->rgb[0], f->rgb[1], f->rgb[2], 0.6f);
        set4(p->cm, f->rgb[0], f->rgb[1], f->rgb[2], 0.35f);
        set4(p->c1, f->rgb[0], f->rgb[1], f->rgb[2], 0);
        p->rot = frand(0, 360);
        p->spin = frand(-30, 30);
    }
    memcpy(f->last, cur, sizeof(f->last));
}

static void emit_smoke(Smoke *s)
{
    Particle *p;
    int k;
    if (s->count++ % 4)
        return;
    if ((p = new_particle()) == NULL)
        return;
    for (k = 0; k < 2; k++)
        p->pos[k] = s->obj->xyz[k] + frand(-0.1f, 0.1f) * s->size;
    p->pos[2] = s->obj->xyz[2];
    p->vel[0] = 4 + frand(-1, 1);
    p->vel[1] = 2 + frand(-1, 1);
    p->vel[2] = 8 + frand(0, 4);
    p->size0 = 0.2f * s->size;
    p->size1 = 1.2f * s->size;
    p->life = frand(8, 12);
    p->mid = 0.15f;
    set4(p->c0, 0.15f, 0.13f, 0.12f, 0);
    set4(p->cm, 0.2f, 0.19f, 0.18f, s->density * 0.5f);
    set4(p->c1, 0.45f, 0.45f, 0.45f, 0);
    p->rot = frand(0, 360);
    p->spin = frand(-10, 10);
}

/* --- the module ------------------------------------------------------------------- */

static void fx_update(const char *arg)
{
    float dt = 1.0f / (es.framerate > 0 ? es.framerate : 60);
    int i, j, k;
    (void)arg;
    for (i = 0; i < MAX_FUMES; i++)
        if (fumes[i].used && fumes[i].on && fumes[i].obj)
            emit_fume(&fumes[i], dt);
    for (i = 0; i < MAX_SMOKES; i++)
        if (smokes[i].used && smokes[i].obj)
            emit_smoke(&smokes[i]);
    for (i = j = 0; i < nparts; i++) {
        Particle *p = &parts[i];
        p->age += dt;
        if (p->age >= p->life)
            continue;
        for (k = 0; k < 3; k++) {
            p->vel[k] -= p->vel[k] * p->drag * dt;
            p->pos[k] += p->vel[k] * dt;
        }
        p->vel[2] -= p->gravity * dt;
        p->rot += p->spin * dt;
        if (j != i)
            parts[j] = *p;
        j++;
    }
    nparts = j;
    if (flash_time > 0)
        flash_time -= dt;
}

static void color_at(const Particle *p, float t, float c[4])
{
    int k;
    if (t < p->mid) {
        float u = p->mid > 0 ? t / p->mid : 1;
        for (k = 0; k < 4; k++)
            c[k] = p->c0[k] + (p->cm[k] - p->c0[k]) * u;
    } else {
        float u = p->mid < 1 ? (t - p->mid) / (1 - p->mid) : 1;
        for (k = 0; k < 4; k++)
            c[k] = p->cm[k] + (p->c1[k] - p->cm[k]) * u;
    }
}

typedef struct Order {
    int i;
    float z;
} Order;

static int order_cmp(const void *a, const void *b)
{
    float za = ((const Order *)a)->z, zb = ((const Order *)b)->z;
    return za < zb ? -1 : za > zb;
}

static void bind_puff(void)
{
    if (puff_tex && puff_tex->image) {
        pfApplyTex(puff_tex);
        return;
    }
    if (!fallback_tex) {
        /* a soft round puff when steam.inta is missing */
        static unsigned char img[32 * 32 * 4];
        int x, y;
        for (y = 0; y < 32; y++)
            for (x = 0; x < 32; x++) {
                float dx = (x - 15.5f) / 15.5f, dy = (y - 15.5f) / 15.5f;
                float a = 1 - (dx * dx + dy * dy);
                unsigned char *q = img + 4 * (y * 32 + x);
                q[0] = q[1] = q[2] = 255;
                q[3] = (unsigned char)(a > 0 ? a * a * 255 : 0);
            }
        glGenTextures(1, &fallback_tex);
        glBindTexture(GL_TEXTURE_2D, fallback_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
    }
    glBindTexture(GL_TEXTURE_2D, fallback_tex);
}

static void fx_draw(const char *arg)
{
    static Order *order;
    static int norder;
    float mv[16], right[3], up[3];
    int i, k, n = 0;
    (void)arg;
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    right[0] = mv[0]; right[1] = mv[4]; right[2] = mv[8];
    up[0] = mv[1]; up[1] = mv[5]; up[2] = mv[9];
    if (nparts > norder) {
        norder = nparts + 256;
        order = (Order *)realloc(order, sizeof(Order) * norder);
    }
    /* back to front */
    for (i = 0; i < nparts; i++) {
        const float *q = parts[i].pos;
        order[n].i = i;
        order[n++].z = mv[2] * q[0] + mv[6] * q[1] + mv[10] * q[2] + mv[14];
    }
    qsort(order, n, sizeof(Order), order_cmp);

    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (i = 0; i < n; i++) {
        const Particle *p = &parts[order[i].i];
        float t = p->age / p->life, s = p->size0 + (p->size1 - p->size0) * t;
        float c[4], cr = (float)cos(PF_DEG2RAD(p->rot)), sr = (float)sin(PF_DEG2RAD(p->rot));
        float ax[3], ay[3];
        color_at(p, t, c);
        if (c[3] <= 0.01f)
            continue;
        /* the right and up vectors of the eye, turned by the particle */
        for (k = 0; k < 3; k++) {
            ax[k] = (right[k] * cr + up[k] * sr) * s / 2;
            ay[k] = (up[k] * cr - right[k] * sr) * s / 2;
        }
        if (p->debris) {
            glDisable(GL_TEXTURE_2D);
            glColor4fv(c);
            glBegin(GL_TRIANGLES);
            glVertex3f(p->pos[0] - ax[0] - ay[0], p->pos[1] - ax[1] - ay[1], p->pos[2] - ax[2] - ay[2]);
            glVertex3f(p->pos[0] + ax[0] - ay[0] * 0.3f, p->pos[1] + ax[1] - ay[1] * 0.3f, p->pos[2] + ax[2] - ay[2] * 0.3f);
            glVertex3f(p->pos[0] + ay[0], p->pos[1] + ay[1], p->pos[2] + ay[2]);
            glEnd();
            continue;
        }
        glEnable(GL_TEXTURE_2D);
        bind_puff();
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glColor4fv(c);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(p->pos[0] - ax[0] - ay[0], p->pos[1] - ax[1] - ay[1], p->pos[2] - ax[2] - ay[2]);
        glTexCoord2f(1, 0);
        glVertex3f(p->pos[0] + ax[0] - ay[0], p->pos[1] + ax[1] - ay[1], p->pos[2] + ax[2] - ay[2]);
        glTexCoord2f(1, 1);
        glVertex3f(p->pos[0] + ax[0] + ay[0], p->pos[1] + ax[1] + ay[1], p->pos[2] + ax[2] + ay[2]);
        glTexCoord2f(0, 1);
        glVertex3f(p->pos[0] - ax[0] + ay[0], p->pos[1] - ax[1] + ay[1], p->pos[2] - ax[2] + ay[2]);
        glEnd();
    }
    /* the white flash over the channel */
    if (flash_time > 0 && es.current_channel == 0) {
        float a = flash_time / flash_len;
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glColor4f(1, 1, 1, a);
        glBegin(GL_QUADS);
        glVertex2f(-1, -1);
        glVertex2f(1, -1);
        glVertex2f(1, 1);
        glVertex2f(-1, 1);
        glEnd();
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    glPopAttrib();
}

void fxInit(void)
{
    char path[4096];
    if (inited)
        return;
    inited = 1;
    puff_tex = pfNewTex(NULL);
    if (!pfFindFile("steam.inta", path, 4) || !pfLoadTexFile(puff_tex, path)) {
        fprintf(stderr, "Unable to Load Effects texture steam.inta.\n");
        free(puff_tex);
        puff_tex = NULL;
    }
    esAddModuleFunc("effects", ES_POSTAPP, fx_update);
    esAddModuleFunc("effects", ES_POSTDRAW, fx_draw);
}
