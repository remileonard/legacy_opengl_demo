/*
 * mekton_view.c - viewer for the IRIS Performer scenes of Mekton (SGI, 1996).
 *
 * Loads the .pfb models of the demo and draws them with OpenGL 1.x the way
 * Performer would: geosets, gstates (lighting, texturing, culling,
 * transparency, alpha function), materials, texture environments, DCS/SCS
 * transforms, animated sequences and morphs.
 *
 * Usage: mekton_view [data/mekton | model.pfb ...]
 */
#ifndef _WIN32
#define _DEFAULT_SOURCE
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#include <GL/freeglut.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pfb.h"

#define MAX_MODELS 256
#define PI 3.14159265358979

static char *models[MAX_MODELS];
static int nmodels, cur_model;
static char texdir[1024];
static PfbFile *scene;
static float bmin[3], bmax[3];

static int win_w = 800, win_h = 600;
static float azim = 30.0f, elev = 20.0f, dist = 10.0f, fov = 60.0f;
static float target[3];
static int drag_button = -1, drag_x, drag_y, drag_shift;
static int wireframe, no_texture, no_lighting, paused, show_help = 1;
static double anim_time, last_time;
/* nodes visited by the current traversal: a damaged file may loop */
static long visits;
#define MAX_VISITS 200000L

/* --- SGI .rgb images ---------------------------------------------------- */

static unsigned int be16(const unsigned char *p) { return (p[0] << 8) | p[1]; }
static unsigned long be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | p[3];
}

/* returns RGBA rows, bottom row first */
static unsigned char *load_rgb(const char *path, int *w, int *h, int *comps)
{
    FILE *fp = fopen(path, "rb");
    unsigned char *file, *img;
    long size;
    int rle, bpc, xs, ys, zs, x, y, z;

    if (!fp)
        return NULL;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size < 512) {
        fclose(fp);
        return NULL;
    }
    file = (unsigned char *)malloc(size);
    if (fread(file, 1, size, fp) != (size_t)size || be16(file) != 474) {
        fclose(fp);
        free(file);
        return NULL;
    }
    fclose(fp);
    rle = file[2];
    bpc = file[3];
    xs = be16(file + 6);
    ys = be16(file + 8);
    zs = be16(file + 10);
    if (zs < 1)
        zs = 1;
    if (bpc != 1 || xs == 0 || ys == 0 || zs > 4 ||
        (!rle && 512L + (long)xs * ys * zs > size) ||
        (rle && 512L + 8L * ys * zs > size)) {
        free(file);
        return NULL;
    }
    img = (unsigned char *)malloc((size_t)xs * ys * 4);
    for (y = 0; y < ys; y++) {
        for (x = 0; x < xs; x++) {
            unsigned char *p = img + ((size_t)y * xs + x) * 4;
            p[0] = p[1] = p[2] = 0;
            p[3] = 255;
        }
    }
    for (z = 0; z < zs; z++) {
        for (y = 0; y < ys; y++) {
            unsigned char row[65536];
            if (rle) {
                unsigned long start = be32(file + 512 + 4L * (z * ys + y));
                unsigned long len = be32(file + 512 + 4L * ys * zs + 4L * (z * ys + y));
                const unsigned char *s = file + start, *e = file + start + len;
                int n = 0;
                if (start + len > (unsigned long)size)
                    continue;
                while (s < e && n < xs) {
                    int c = *s++, cnt = c & 0x7f;
                    if (!cnt)
                        break;
                    if (c & 0x80) {
                        while (cnt-- && s < e && n < xs)
                            row[n++] = *s++;
                    } else if (s < e) {
                        unsigned char v = *s++;
                        while (cnt-- && n < xs)
                            row[n++] = v;
                    }
                }
                while (n < xs)
                    row[n++] = 0;
            } else {
                memcpy(row, file + 512 + ((size_t)z * ys + y) * xs, xs);
            }
            for (x = 0; x < xs; x++) {
                unsigned char *p = img + ((size_t)y * xs + x) * 4;
                if (zs < 3) {
                    if (z == 0)
                        p[0] = p[1] = p[2] = row[x];
                    else
                        p[3] = row[x];
                } else {
                    p[z] = row[x];
                }
            }
        }
    }
    free(file);
    *w = xs;
    *h = ys;
    *comps = zs;
    return img;
}

static void load_textures(PfbFile *f)
{
    int i;
    for (i = 0; i < f->ntex; i++) {
        PfbTex *t = &f->tex[i];
        char path[2048];
        unsigned char *img;
        const char *base;
        if (!t->name)
            continue;
        base = strrchr(t->name, '/');
        base = base ? base + 1 : t->name;
        sprintf(path, "%.1000s/%.1000s", texdir, base);
        img = load_rgb(path, &t->width, &t->height, &t->comps);
        if (!img) {
            fprintf(stderr, "mekton_view: texture %s not found\n", base);
            continue;
        }
        glGenTextures(1, &t->gl_id);
        glBindTexture(GL_TEXTURE_2D, t->gl_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, t->width, t->height,
                          GL_RGBA, GL_UNSIGNED_BYTE, img);
        free(img);
    }
}

/* --- models ------------------------------------------------------------- */

static int is_pfb(const char *name)
{
    size_t n = strlen(name);
    return n > 4 && (strcmp(name + n - 4, ".pfb") == 0 || strcmp(name + n - 4, ".PFB") == 0);
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static void add_model(const char *path)
{
    if (nmodels < MAX_MODELS) {
        models[nmodels] = (char *)malloc(strlen(path) + 1);
        strcpy(models[nmodels++], path);
    }
}

static void scan_models(const char *dir)
{
    char path[2048];
    int first = nmodels;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h;
    sprintf(path, "%.1000s\\*.pfb", dir);
    h = FindFirstFileA(path, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            sprintf(path, "%.1000s/%.1000s", dir, fd.cFileName);
            add_model(path);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR *d = opendir(dir);
    struct dirent *e;
    if (d) {
        while ((e = readdir(d)) != NULL) {
            if (is_pfb(e->d_name)) {
                sprintf(path, "%.1000s/%.1000s", dir, e->d_name);
                add_model(path);
            }
        }
        closedir(d);
    }
#endif
    qsort(models + first, nmodels - first, sizeof(char *), cmp_str);
}

static void bounds_node(PfbFile *f, int idx, const float *m, int depth)
{
    PfbNode *nd;
    float mm[16];
    int i, j, k;

    if (idx < 0 || idx >= f->nnode || depth > 64 || ++visits > MAX_VISITS)
        return;
    nd = &f->node[idx];
    if (nd->type == PFB_NODE_DCS || nd->type == PFB_NODE_SCS) {
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++) {
                float s = 0;
                for (k = 0; k < 4; k++)
                    s += nd->matrix[i * 4 + k] * m[k * 4 + j];
                mm[i * 4 + j] = s;
            }
        m = mm;
    }
    for (i = 0; i < nd->ngsets; i++) {
        PfbGSet *g;
        PfbList *c;
        if (nd->gsets[i] < 0 || nd->gsets[i] >= f->ngset)
            continue;
        g = &f->gset[nd->gsets[i]];
        if (g->attr[PFB_COORD].list < 0 || g->attr[PFB_COORD].list >= f->nlist[5])
            continue;
        c = &f->list[5][g->attr[PFB_COORD].list];
        for (j = 0; j < c->count; j++) {
            const float *v = c->f + 3 * j;
            for (k = 0; k < 3; k++) {
                float w = v[0] * m[k] + v[1] * m[4 + k] + v[2] * m[8 + k] + m[12 + k];
                if (w < bmin[k]) bmin[k] = w;
                if (w > bmax[k]) bmax[k] = w;
            }
        }
    }
    for (i = 0; i < nd->nchildren; i++)
        bounds_node(f, nd->children[i], m, depth + 1);
}

static void reset_view(void)
{
    float d = 0;
    int k;
    for (k = 0; k < 3; k++) {
        target[k] = (bmin[k] + bmax[k]) * 0.5f;
        d += (bmax[k] - bmin[k]) * (bmax[k] - bmin[k]);
    }
    d = (float)sqrt(d);
    if (d <= 0)
        d = 10;
    dist = d * 0.9f;
    azim = 30.0f;
    elev = 25.0f;
}

static void load_model(int i)
{
    static const float ident[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    char title[1100];
    const char *base;
    int k, t;

    if (scene) {
        for (t = 0; t < scene->ntex; t++)
            if (scene->tex[t].gl_id)
                glDeleteTextures(1, &scene->tex[t].gl_id);
        pfb_free(scene);
    }
    cur_model = (i + nmodels) % nmodels;
    scene = pfb_load(models[cur_model]);
    if (!scene)
        fprintf(stderr, "mekton_view: cannot load %s\n", models[cur_model]);
    for (k = 0; k < 3; k++) {
        bmin[k] = 1e30f;
        bmax[k] = -1e30f;
    }
    if (scene) {
        load_textures(scene);
        visits = 0;
        bounds_node(scene, 0, ident, 0);
    }
    if (bmin[0] > bmax[0])
        for (k = 0; k < 3; k++)
            bmin[k] = -1, bmax[k] = 1;
    reset_view();
    base = strrchr(models[cur_model], '/');
    base = base ? base + 1 : models[cur_model];
    sprintf(title, "Mekton - %.1000s", base);
    glutSetWindowTitle(title);
    anim_time = 0;
}

/* --- drawing ------------------------------------------------------------ */

typedef struct {
    int lighting, texturing, transparent, cull, alphafunc, lpoint;
    PfbMtl *mtl;
    PfbTex *tex;
    PfbTEnv *tenv;
} State;

static void resolve_state(const PfbGSet *g, State *s)
{
    static const GLenum cull_tab[5] = { 0, 0, GL_BACK, GL_FRONT, GL_FRONT_AND_BACK };
    static const GLenum afunc_tab[10] = {
        GL_ALWAYS, GL_ALWAYS, GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL,
        GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS
    };
    const PfbGState *gs = NULL;

    memset(s, 0, sizeof(*s));
    if (g->gstate >= 0 && g->gstate < scene->ngstate)
        gs = &scene->gstate[g->gstate];
    if (!gs)
        return;
    /* boolean modes: 1 = off, 2 = on */
    s->lighting = gs->lighting == 2;
    s->texturing = gs->texturing == 2;
    s->lpoint = gs->lpoint == 2;
    s->transparent = gs->transparency != PFB_INHERIT && gs->transparency >= 2;
    if (gs->cullface >= 0 && gs->cullface < 5)
        s->cull = (int)cull_tab[gs->cullface];
    if (gs->alphafunc >= 0 && gs->alphafunc < 10)
        s->alphafunc = (int)afunc_tab[gs->alphafunc];
    if (gs->frontmtl >= 0 && gs->frontmtl < scene->nmtl)
        s->mtl = &scene->mtl[gs->frontmtl];
    if (gs->texture >= 0 && gs->texture < scene->ntex && scene->tex[gs->texture].gl_id)
        s->tex = &scene->tex[gs->texture];
    if (gs->texenv >= 0 && gs->texenv < scene->ntenv)
        s->tenv = &scene->tenv[gs->texenv];
}

static void apply_state(const State *s)
{
    static const GLenum tenv_tab[5] = { GL_MODULATE, GL_MODULATE, GL_BLEND, GL_DECAL, GL_REPLACE };

    if (s->lighting && !no_lighting && !s->lpoint) {
        glEnable(GL_LIGHTING);
        if (s->mtl) {
            float c[4];
            const PfbMtl *m = s->mtl;
            c[3] = m->alpha;
            memcpy(c, m->ambient, sizeof(float) * 3);
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);
            memcpy(c, m->diffuse, sizeof(float) * 3);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);
            memcpy(c, m->specular, sizeof(float) * 3);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
            memcpy(c, m->emission, sizeof(float) * 3);
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m->shininess > 128 ? 128 : m->shininess);
            if (m->cmode == 2) {
                glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
                glEnable(GL_COLOR_MATERIAL);
            } else {
                glDisable(GL_COLOR_MATERIAL);
            }
        } else {
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glEnable(GL_COLOR_MATERIAL);
        }
    } else {
        glDisable(GL_LIGHTING);
        glDisable(GL_COLOR_MATERIAL);
    }
    if (s->texturing && s->tex && !no_texture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, s->tex->gl_id);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
                  (int)tenv_tab[s->tenv && s->tenv->mode >= 0 && s->tenv->mode < 5 ? s->tenv->mode : 1]);
        if (s->tenv)
            glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, s->tenv->blend);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
    if (s->cull && !wireframe) {
        glEnable(GL_CULL_FACE);
        glCullFace((GLenum)s->cull);
    } else {
        glDisable(GL_CULL_FACE);
    }
    if (s->alphafunc) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc((GLenum)s->alphafunc, 0.0f);
    } else {
        glDisable(GL_ALPHA_TEST);
    }
    if (s->lpoint) {
        glEnable(GL_POINT_SMOOTH);
        glPointSize(4.0f);
    } else {
        glDisable(GL_POINT_SMOOTH);
    }
}

static const float *attr_ptr(const PfbAttr *a, int type, int idx, int comps)
{
    PfbList *l;
    if (a->bind == PFB_OFF || a->bind == PFB_BIND_NONE || a->list < 0 ||
        a->list >= scene->nlist[type])
        return NULL;
    l = &scene->list[type][a->list];
    if (a->bind == PFB_OVERALL)
        idx = 0;
    if (idx < 0 || idx >= l->count || l->comps != comps)
        return NULL;
    return l->f + idx * comps;
}

static void draw_gset(const PfbGSet *g)
{
    static const float white[4] = { 1, 1, 1, 1 };
    const PfbAttr *ca = &g->attr[PFB_COLOR], *na = &g->attr[PFB_NORMAL];
    const PfbList *lens = NULL;
    int flat = g->prim == PFB_FLAT_TRISTRIPS || g->prim == PFB_FLAT_LINESTRIPS;
    int skip = g->prim == PFB_FLAT_TRISTRIPS ? 2 : 1;
    int vtx = 0, fvtx = 0, p, k;
    GLenum mode;

    switch (g->prim) {
    case PFB_TRIFANS: mode = GL_TRIANGLE_FAN; break;
    case PFB_TRISTRIPS: case PFB_FLAT_TRISTRIPS: mode = GL_TRIANGLE_STRIP; break;
    case PFB_TRIS: mode = GL_TRIANGLES; break;
    case PFB_POINTS: mode = GL_POINTS; break;
    case PFB_LINES: mode = GL_LINES; break;
    case PFB_LINESTRIPS: case PFB_FLAT_LINESTRIPS: mode = GL_LINE_STRIP; break;
    case PFB_QUADS: mode = GL_QUADS; break;
    case PFB_POLYS: mode = GL_POLYGON; break;
    default: return;
    }
    if (g->lengths >= 0 && g->lengths < scene->nlist[4])
        lens = &scene->list[4][g->lengths];
    if (!attr_ptr(&g->attr[PFB_COORD], 5, 0, 3))
        return;
    glShadeModel(g->flatshade || flat ? GL_FLAT : GL_SMOOTH);
    if (ca->bind == PFB_OFF || ca->bind == PFB_BIND_NONE)
        glColor4fv(white);
    if (g->linewidth > 0)
        glLineWidth(g->linewidth);
    if (g->pointsize > 0)
        glPointSize(g->pointsize);

    for (p = 0; p < g->nprims; p++) {
        int n;
        const float *c;
        switch (g->prim) {
        case PFB_POINTS: n = 1; break;
        case PFB_LINES: n = 2; break;
        case PFB_TRIS: n = 3; break;
        case PFB_QUADS: n = 4; break;
        default: n = lens && p < lens->count ? lens->i[p] : 0; break;
        }
        if (p == 0 || (mode != GL_POINTS && mode != GL_LINES &&
                       mode != GL_TRIANGLES && mode != GL_QUADS))
            glBegin(mode);
        if (ca->bind == PFB_OVERALL || ca->bind == PFB_PER_PRIM) {
            c = attr_ptr(ca, 6, p, 4);
            if (c) glColor4fv(c);
        }
        if (na->bind == PFB_OVERALL || na->bind == PFB_PER_PRIM) {
            c = attr_ptr(na, 7, p, 3);
            if (c) glNormal3fv(c);
        }
        for (k = 0; k < n; k++, vtx++) {
            /* flat strips carry no color/normal for their first vertices */
            int ai = flat ? fvtx + (k < skip ? 0 : k - skip) : vtx;
            if (ca->bind == PFB_PER_VERTEX && (c = attr_ptr(ca, 6, ai, 4)) != NULL)
                glColor4fv(c);
            if (na->bind == PFB_PER_VERTEX && (c = attr_ptr(na, 7, ai, 3)) != NULL)
                glNormal3fv(c);
            if ((c = attr_ptr(&g->attr[PFB_TEXCOORD], 8, vtx, 2)) != NULL)
                glTexCoord2fv(c);
            if ((c = attr_ptr(&g->attr[PFB_COORD], 5, vtx, 3)) != NULL)
                glVertex3fv(c);
        }
        if (flat)
            fvtx += n > skip ? n - skip : 0;
        if (mode != GL_POINTS && mode != GL_LINES && mode != GL_TRIANGLES &&
            mode != GL_QUADS)
            glEnd();
    }
    if (mode == GL_POINTS || mode == GL_LINES || mode == GL_TRIANGLES || mode == GL_QUADS)
        glEnd();
    glLineWidth(1.0f);
    glPointSize(1.0f);
}

static int seq_frame(const PfbNode *nd)
{
    double total = 0, t;
    int i, b = nd->seq_begin, e = nd->seq_end;
    if (b < 0) b = 0;
    if (e < 0 || e >= nd->nchildren) e = nd->nchildren - 1;
    if (e < b || nd->nframes <= 0)
        return 0;
    for (i = b; i <= e; i++)
        total += i < nd->nframes && nd->times[i] > 0 ? nd->times[i] : 0.03;
    t = fmod(anim_time * (nd->seq_speed > 0 ? nd->seq_speed : 1.0), total);
    for (i = b; i <= e; i++) {
        t -= i < nd->nframes && nd->times[i] > 0 ? nd->times[i] : 0.03;
        if (t < 0)
            return i;
    }
    return e;
}

static void draw_node(int idx, int pass, int depth)
{
    PfbNode *nd;
    int i;

    if (idx < 0 || idx >= scene->nnode || depth > 64 || ++visits > MAX_VISITS)
        return;
    nd = &scene->node[idx];
    if (nd->type == PFB_NODE_DCS || nd->type == PFB_NODE_SCS) {
        glPushMatrix();
        glMultMatrixf(nd->matrix);
    }
    for (i = 0; i < nd->ngsets; i++) {
        State s;
        const PfbGSet *g;
        if (nd->gsets[i] < 0 || nd->gsets[i] >= scene->ngset)
            continue;
        g = &scene->gset[nd->gsets[i]];
        resolve_state(g, &s);
        if ((s.transparent || s.lpoint) != pass)
            continue;
        apply_state(&s);
        draw_gset(g);
    }
    if (nd->type == PFB_NODE_SEQUENCE) {
        if (nd->nchildren > 0)
            draw_node(nd->children[seq_frame(nd)], pass, depth + 1);
    } else if (nd->type == PFB_NODE_SWITCH) {
        if (nd->switch_val == -1) {           /* PFSWITCH_ON */
            for (i = 0; i < nd->nchildren; i++)
                draw_node(nd->children[i], pass, depth + 1);
        } else if (nd->switch_val >= 0 && nd->switch_val < nd->nchildren) {
            draw_node(nd->children[nd->switch_val], pass, depth + 1);
        }
    } else if (nd->type == PFB_NODE_LOD) {
        if (nd->nchildren > 0)                /* finest level */
            draw_node(nd->children[0], pass, depth + 1);
    } else {
        for (i = 0; i < nd->nchildren; i++)
            draw_node(nd->children[i], pass, depth + 1);
    }
    if (nd->type == PFB_NODE_DCS || nd->type == PFB_NODE_SCS)
        glPopMatrix();
}

static void draw_text(int x, int y, const char *s)
{
    glRasterPos2i(x, y);
    while (*s)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s++);
}

static void display(void)
{
    static const float sun[4] = { 0.4f, -0.5f, 0.8f, 0.0f };
    static const float amb[4] = { 0.35f, 0.35f, 0.35f, 1.0f };
    static const float dif[4] = { 0.85f, 0.85f, 0.85f, 1.0f };
    float size = 0, ex, ey, ez, ce, se;
    int k;
    char line[256];

    for (k = 0; k < 3; k++)
        if (bmax[k] - bmin[k] > size)
            size = bmax[k] - bmin[k];
    glClearColor(0.25f, 0.35f, 0.55f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fov, (double)win_w / (win_h ? win_h : 1),
                   dist * 0.01 > size * 0.0005 ? dist * 0.01 : size * 0.0005,
                   dist + size * 2.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    /* Performer is Z up */
    ce = (float)cos(elev * PI / 180);
    se = (float)sin(elev * PI / 180);
    ex = target[0] + dist * ce * (float)sin(azim * PI / 180);
    ey = target[1] - dist * ce * (float)cos(azim * PI / 180);
    ez = target[2] + dist * se;
    gluLookAt(ex, ey, ez, target[0], target[1], target[2], 0, 0, 1);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, sun);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, dif);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

    if (scene) {
        /* Mekton scrolls the waterfall with a two-source morph whose
         * weights are (1 - f, f), f = fractional part of time / 5
         * (update_colony) */
        for (k = 0; k < scene->nmorph; k++) {
            if (pfb_morph_sources(scene, k) == 2) {
                float w[2];
                w[1] = (float)(anim_time / 5.0 - floor(anim_time / 5.0));
                w[0] = 1.0f - w[1];
                pfb_morph(scene, k, w);
            }
        }
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        visits = 0;
        draw_node(0, 0, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        visits = 0;
        draw_node(0, 1, 0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, win_w, 0, win_h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(1, 1, 1);
    {
        const char *base = strrchr(models[cur_model], '/');
        base = base ? base + 1 : models[cur_model];
        sprintf(line, "%.100s  (%d/%d)  %d nodes, %d geosets, %d textures",
                base, cur_model + 1, nmodels, scene ? scene->nnode : 0,
                scene ? scene->ngset : 0, scene ? scene->ntex : 0);
        draw_text(8, win_h - 18, line);
    }
    if (show_help)
        draw_text(8, 8, "n/p: next/prev  left drag: turn  right drag or +/-: zoom  "
                        "shift+left: pan  w: wire  t: tex  l: light  space: pause  r: reset  h: help");
    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    win_w = w;
    win_h = h;
    glViewport(0, 0, w, h);
}

static void keyboard(unsigned char key, int x, int y)
{
    (void)x; (void)y;
    switch (key) {
    case 27: case 'q': exit(0);
    case 'n': load_model(cur_model + 1); break;
    case 'p': load_model(cur_model - 1); break;
    case 'w': wireframe = !wireframe; break;
    case 't': no_texture = !no_texture; break;
    case 'l': no_lighting = !no_lighting; break;
    case ' ': paused = !paused; break;
    case 'r': reset_view(); break;
    case 'h': show_help = !show_help; break;
    case '+': case '=': dist *= 0.8f; break;
    case '-': dist *= 1.25f; break;
    }
    glutPostRedisplay();
}

static void special(int key, int x, int y)
{
    (void)x; (void)y;
    switch (key) {
    case GLUT_KEY_LEFT: azim -= 5; break;
    case GLUT_KEY_RIGHT: azim += 5; break;
    case GLUT_KEY_UP: elev += 5; break;
    case GLUT_KEY_DOWN: elev -= 5; break;
    case GLUT_KEY_PAGE_UP: load_model(cur_model - 1); break;
    case GLUT_KEY_PAGE_DOWN: load_model(cur_model + 1); break;
    }
    if (elev > 89) elev = 89;
    if (elev < -89) elev = -89;
    glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        if (button == 3 || button == 4) {     /* wheel */
            dist *= button == 3 ? 0.9f : 1.1f;
            glutPostRedisplay();
            return;
        }
        drag_button = button;
        drag_shift = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) != 0;
        drag_x = x;
        drag_y = y;
    } else {
        drag_button = -1;
    }
}

static void motion(int x, int y)
{
    int dx = x - drag_x, dy = y - drag_y;
    drag_x = x;
    drag_y = y;
    if (drag_button == GLUT_LEFT_BUTTON && !drag_shift) {
        azim += dx * 0.4f;
        elev += dy * 0.4f;
        if (elev > 89) elev = 89;
        if (elev < -89) elev = -89;
    } else if (drag_button == GLUT_RIGHT_BUTTON) {
        dist *= (float)pow(1.01, dy);
    } else if (drag_button == GLUT_MIDDLE_BUTTON || drag_button == GLUT_LEFT_BUTTON) {
        float s = dist * 0.0015f;
        float ca = (float)cos(azim * PI / 180), sa = (float)sin(azim * PI / 180);
        target[0] -= dx * ca * s;
        target[1] -= dx * sa * s;
        target[2] += dy * s;
    }
    glutPostRedisplay();
}

static double now(void)
{
#ifdef _WIN32
    return GetTickCount() / 1000.0;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
#endif
}

static void idle(void)
{
    double t = now();
    if (!paused)
        anim_time += t - last_time;
    last_time = t;
    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    int i;
    const char *data = "data/mekton";

    glutInit(&argc, argv);
    for (i = 1; i < argc; i++) {
        if (is_pfb(argv[i]))
            add_model(argv[i]);
        else
            data = argv[i];
    }
    if (nmodels == 0) {
        char dir[1100];
        sprintf(dir, "%.1000s/models", data);
        scan_models(dir);
        sprintf(texdir, "%.1000s/textures", data);
    } else {
        /* models/x.pfb -> models/../textures */
        char *s;
        strncpy(texdir, models[0], sizeof(texdir) - 20);
        texdir[sizeof(texdir) - 20] = 0;
        s = strrchr(texdir, '/');
        if (s) {
            *s = 0;
            strcat(texdir, "/../textures");
        } else {
            strcpy(texdir, "../textures");
        }
    }
    if (nmodels == 0) {
        fprintf(stderr, "usage: mekton_view [data/mekton | model.pfb ...]\n"
                        "no .pfb found in %s/models\n", data);
        return 1;
    }
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(win_w, win_h);
    glutCreateWindow("Mekton");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);
    last_time = now();
    load_model(0);
    glutMainLoop();
    return 0;
}
