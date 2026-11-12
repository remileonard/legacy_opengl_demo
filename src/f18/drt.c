/*
 * drt.c - the runtime files of Designer's Workbench (.drt), rebuilt from
 * the files of the F-18 demo: f18hud.drt and its ASCII version
 * f18hud.drta give the meaning of the records, myf18.drt is the HUD of the
 * demo.
 *
 * The geometry is the one of the .dwb files (see dwb.c): groups, faces
 * (0x1775, +0x0a: 1 filled, 2 closed outline, 3 open line) and their
 * vertices, and strings:
 *
 *  0x17c0 string      +0x04 color index (high 16 bits), +0x10 font width,
 *                     +0x14 font height, +0x20 horizontal alignment
 *                     (2 center), +0x58 position x y z
 *  0x179a             its text, 0x1799 its font
 *
 * The links follow the record they change:
 *
 *  0x2713 link table  +0x00 number of links
 *  0x2787 rotate      +0x00 origin, +0x14 lower and upper limits of the
 *                     value, +0x1c start and end angles, +0x24 flags;
 *                     rotates in the XY plane (rotationPlane -XY)
 *  0x278f translate   +0x0c direction, +0x18 start and end offsets,
 *                     +0x20 lower and upper limits
 *  0x2790 visibility  shown while the value is not negative
 *  0x278a string      +0x0c type: 0x10000 float, else integer; low bit
 *                     padding with spaces, else zeros; 0x2714 its format
 *  0x2788 scale       +0x00 origin, +0x0c axis: the value scales along it
 *  0x2776 color       the value is the color index
 *
 * Each link maps a function of a variable (mapFunc): 0x2718 records of 80
 * bytes, from the outer function to the inner one: +0x00 offset of the
 * variable, +0x08 function (high 16 bits), +0x14 a, +0x18 b, +0x20 2 for a
 * function of the next record, 3 for a function of the variable:
 *
 *   16 x   7 linear: a x + b   4 div: x / a + b
 *   11 mod2: a x modulo b      15 trunc: trunc(x / a) + b
 *
 * The colors are indexes in the color table (0x1790: red, green and blue
 * shorts): index c is the color c / 128 at the intensity c % 128 / 127.
 * The strings are drawn with the vector font of Designer's Workbench
 * (olddwb.vfn: polylines in a cell).
 */
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pf.h"
#include "drt.h"

enum { N_GROUP, N_FACE, N_STRING };
enum { L_ROTATE, L_TRANSLATE, L_VISIBLE, L_STRING, L_SCALE, L_COLOR };

typedef struct Expr {
    int var, fn, leaf;
    float a, b;
} Expr;

typedef struct Link {
    int type;
    float f[16];
    int w[16];
    char fmt[32];
    Expr *expr;
    int nexpr;
} Link;

typedef struct Node {
    int kind;
    struct Node **kids;
    int nkids;
    Link *links;
    int nlinks;
    /* faces */
    float *v;
    int nv, mode;
    float rgb[3];
    /* strings */
    char text[64];
    float fw, fh, pos[3];
    int halign;
} Node;

struct Drt {
    Node *root;
    float palette[160][3];
    int npalette;
};

/* --- the vector font --------------------------------------------------------- */

typedef struct Glyph {
    float adv;
    int nstrokes;
    int *len;
    float *pts;
} Glyph;

static Glyph font[128];
static int font_loaded;

static void load_font(void)
{
    char path[4096], line[8192];
    FILE *fp;
    int code = -1;
    font_loaded = 1;
    if (!pfFindFile("olddwb.vfn", path, 4) || (fp = fopen(path, "r")) == NULL) {
        fprintf(stderr, "drt: font olddwb.vfn not found\n");
        return;
    }
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Index:", 6) == 0) {
            code = atoi(line + 6);
        } else if (strncmp(line, "Adjustments:", 12) == 0 && code >= 0 && code < 128) {
            font[code].adv = (float)atof(line + 12);
        } else if (line[0] == '\t' && line[1] == '\t' && code >= 0 && code < 128) {
            Glyph *g = &font[code];
            char *s = line;
            int n = (int)strtol(s, &s, 10), k, base = 0;
            if (n <= 0)
                continue;
            for (k = 0; k < g->nstrokes; k++)
                base += g->len[k];
            g->len = (int *)realloc(g->len, sizeof(int) * (g->nstrokes + 1));
            g->len[g->nstrokes++] = n;
            g->pts = (float *)realloc(g->pts, sizeof(float) * 2 * (base + n));
            for (k = 0; k < 2 * n; k++)
                g->pts[2 * base + k] = (float)strtod(s, &s);
        }
    }
    fclose(fp);
}

/* a string with its baseline at the origin, characters of w x h */
static void draw_text(const char *s, float w, float h, int center)
{
    float x = 0, total = 0;
    const char *c;
    if (!font_loaded)
        load_font();
    for (c = s; *c; c++)
        total += ((unsigned char)*c < 128 && font[(unsigned char)*c].adv > 0 ? font[(unsigned char)*c].adv : 0.833f) * w;
    if (center)
        x = -total / 2;
    for (c = s; *c; c++) {
        const Glyph *g = (unsigned char)*c < 128 ? &font[(unsigned char)*c] : NULL;
        int k, i, base = 0;
        if (g) {
            for (k = 0; k < g->nstrokes; k++) {
                glBegin(GL_LINE_STRIP);
                for (i = 0; i < g->len[k]; i++)
                    glVertex2f(x + g->pts[2 * (base + i)] * w, g->pts[2 * (base + i) + 1] * h);
                glEnd();
                base += g->len[k];
            }
        }
        x += (g && g->adv > 0 ? g->adv : 0.833f) * w;
    }
}

/* --- reading -------------------------------------------------------------------- */

static unsigned int be16(const unsigned char *p) { return (p[0] << 8) | p[1]; }
static int sbe16(const unsigned char *p) { return (short)be16(p); }
static unsigned int be32(const unsigned char *p)
{
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) | ((unsigned int)p[2] << 8) | p[3];
}
static float bef(const unsigned char *p)
{
    unsigned int u = be32(p);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static Node *new_node(Node *parent, int kind)
{
    Node *n = (Node *)calloc(1, sizeof(Node));
    n->kind = kind;
    n->rgb[0] = n->rgb[1] = n->rgb[2] = 1;
    if (parent) {
        parent->kids = (Node **)realloc(parent->kids, sizeof(Node *) * (parent->nkids + 1));
        parent->kids[parent->nkids++] = n;
    }
    return n;
}

static void index_color(const Drt *d, int c, float rgb[3])
{
    int pal = c / 128, k;
    float in = (c % 128) / 127.0f;
    if (c < 0 || pal >= d->npalette || c >= 4096) {
        /* 0x1000 and up: the color of the material, white here */
        rgb[0] = rgb[1] = rgb[2] = 243 / 255.0f;
        return;
    }
    for (k = 0; k < 3; k++)
        rgb[k] = d->palette[pal][k] * in;
}

#define MAX_DEPTH 128

Drt *drtLoad(const char *file)
{
    FILE *fp = fopen(file, "rb");
    unsigned char *data;
    long size, p;
    Drt *d;
    Node *stack[MAX_DEPTH], *last = NULL, *face = NULL;
    Link *link = NULL;
    int depth = 0, in_face[MAX_DEPTH];

    if (!fp)
        return NULL;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = (unsigned char *)malloc(size > 0 ? size : 1);
    if (size < 8 || fread(data, 1, size, fp) != (size_t)size || be16(data) != 0x1771) {
        fclose(fp);
        free(data);
        return NULL;
    }
    fclose(fp);
    d = (Drt *)calloc(1, sizeof(Drt));
    d->root = new_node(NULL, N_GROUP);
    stack[0] = d->root;
    in_face[0] = 0;
    for (p = 0; p + 4 <= size;) {
        int op = be16(data + p), len = be16(data + p + 2), k;
        const unsigned char *b = data + p + 4;
        Node *cur = stack[depth];
        if (p + 4 + len > size)
            break;
        p += 4 + len;
        switch (op) {
        case 0x1790: /* color table */
            for (k = 0; k + 6 <= len && d->npalette < 160; k += 6, d->npalette++) {
                d->palette[d->npalette][0] = be16(b + k) / 255.0f;
                d->palette[d->npalette][1] = be16(b + k + 2) / 255.0f;
                d->palette[d->npalette][2] = be16(b + k + 4) / 255.0f;
            }
            break;
        case 0x1772:
            last = new_node(cur, N_GROUP);
            face = NULL;
            break;
        case 0x1775:
            if (len < 0x30)
                break;
            last = face = new_node(cur, N_FACE);
            face->nv = sbe16(b + 4);
            face->mode = sbe16(b + 0x0a);
            index_color(d, (int)be32(b), face->rgb);
            break;
        case 0x177a:
        case 0x177c:
            if (face && !face->v) {
                int n = len / 44;
                if (face->nv <= 0 || face->nv > n)
                    face->nv = n;
                face->v = (float *)malloc(sizeof(float) * 3 * (face->nv + 1));
                for (k = 0; k < face->nv; k++) {
                    face->v[3 * k] = bef(b + 44 * k + 4);
                    face->v[3 * k + 1] = bef(b + 44 * k + 8);
                    face->v[3 * k + 2] = bef(b + 44 * k + 12);
                }
            }
            break;
        case 0x17c0:
            if (len < 0x64)
                break;
            last = new_node(cur, N_STRING);
            face = NULL;
            index_color(d, (int)(be32(b + 4) >> 16), last->rgb);
            last->fw = bef(b + 0x10);
            last->fh = bef(b + 0x14);
            last->halign = (int)be32(b + 0x20);
            last->pos[0] = bef(b + 0x58);
            last->pos[1] = bef(b + 0x5c);
            last->pos[2] = bef(b + 0x60);
            break;
        case 0x179a:
            if (last && last->kind == N_STRING) {
                int n = len < 63 ? len : 63;
                memcpy(last->text, b, n);
                last->text[n] = 0;
            }
            break;
        case 0x2787: case 0x278f: case 0x2790: case 0x278a: case 0x2788: case 0x2776:
            if (!last)
                break;
            last->links = (Link *)realloc(last->links, sizeof(Link) * (last->nlinks + 1));
            link = &last->links[last->nlinks++];
            memset(link, 0, sizeof(*link));
            link->type = op == 0x2787 ? L_ROTATE : op == 0x278f ? L_TRANSLATE : op == 0x2790 ? L_VISIBLE
                       : op == 0x278a ? L_STRING : op == 0x2788 ? L_SCALE : L_COLOR;
            for (k = 0; k < 16 && 4 * k + 4 <= len; k++) {
                link->f[k] = bef(b + 4 * k);
                link->w[k] = (int)be32(b + 4 * k);
            }
            strcpy(link->fmt, "%g");
            break;
        case 0x2714:
            if (link) {
                int n = len < 31 ? len : 31;
                memcpy(link->fmt, b, n);
                link->fmt[n] = 0;
            }
            break;
        case 0x2718:
            if (link && len >= 0x24) {
                Expr *e;
                link->expr = (Expr *)realloc(link->expr, sizeof(Expr) * (link->nexpr + 1));
                e = &link->expr[link->nexpr++];
                e->var = (int)be32(b);
                e->fn = (int)(be32(b + 8) >> 16);
                e->a = bef(b + 0x14);
                e->b = bef(b + 0x18);
                e->leaf = be32(b + 0x20) == 3;
            }
            break;
        case 0x17ca:
            if (depth + 1 >= MAX_DEPTH)
                break;
            depth++;
            if (last && last->kind == N_GROUP) {
                stack[depth] = last;
                in_face[depth] = 0;
            } else {
                /* the vertices of a face, the strings: no children */
                stack[depth] = cur;
                in_face[depth] = 1;
            }
            last = NULL;
            break;
        case 0x17cb:
            if (depth > 0)
                depth--;
            last = NULL;
            face = NULL;
            link = NULL;
            break;
        default:
            break;
        }
        (void)in_face;
    }
    free(data);
    return d;
}

/* --- drawing -------------------------------------------------------------------- */

static float apply(int fn, float a, float x, float b)
{
    switch (fn) {
    case 7: return a * x + b;
    case 4: return a != 0 ? x / a + b : b;
    case 11: {
        float v = b != 0 ? (float)fmod(a * x, b) : a * x;
        return v < 0 ? v + b : v;
    }
    case 15: return a != 0 ? (float)(int)(x / a) + b : b;
    default: return x;
    }
}

static float eval(const Link *l, const float *vars, int nvars)
{
    float v = 0;
    int i;
    if (l->nexpr == 0)
        return 0;
    for (i = l->nexpr - 1; i >= 0; i--) {
        const Expr *e = &l->expr[i];
        float x = v;
        if (e->leaf) {
            int k = e->var / 4;
            x = k >= 0 && k < nvars ? vars[k] : 0;
        }
        v = apply(e->fn, e->a, x, e->b);
    }
    return v;
}

/* from the limits of the value to the start and end of the link */
static float map(float v, float lo, float hi, float start, float end, int limits)
{
    if (hi == lo)
        return v;
    if (limits) {
        if (v < lo) v = lo;
        if (v > hi) v = hi;
    }
    return start + (v - lo) / (hi - lo) * (end - start);
}

static void draw_node(const Drt *d, const Node *n, const float *vars, int nvars, int depth)
{
    char text[64];
    float rgb[3];
    int i;
    if (depth > 64)
        return;
    memcpy(rgb, n->rgb, sizeof(rgb));
    strcpy(text, n->text);
    glPushMatrix();
    for (i = 0; i < n->nlinks; i++) {
        const Link *l = &n->links[i];
        float v = eval(l, vars, nvars);
        switch (l->type) {
        case L_ROTATE: {
            float a = map(v, l->f[5], l->f[6], l->f[7], l->f[8], (l->w[9] & 0x10000) != 0);
            glTranslatef(l->f[0], l->f[1], l->f[2]);
            glRotatef(-a, 0, 0, 1);
            glTranslatef(-l->f[0], -l->f[1], -l->f[2]);
            break;
        }
        case L_TRANSLATE: {
            float o = map(v, l->f[8], l->f[9], l->f[6], l->f[7], (l->w[10] & 0x10000) != 0);
            glTranslatef(l->f[3] * o, l->f[4] * o, l->f[5] * o);
            break;
        }
        case L_VISIBLE:
            if (v < 0) {
                glPopMatrix();
                return;
            }
            break;
        case L_STRING: {
            char fmt[40];
            const char *f = l->fmt;
            /* zero padding: "%3d" is "%03d" */
            if (!(l->w[3] & 1) && f[0] == '%' && f[1] >= '1' && f[1] <= '9')
                sprintf(fmt, "%%0%s", f + 1);
            else
                strcpy(fmt, f);
            if (l->w[3] & 0x10000)
                snprintf(text, sizeof(text), fmt, (double)v);
            else
                snprintf(text, sizeof(text), fmt, (int)floor(v + 0.5f));
            break;
        }
        case L_SCALE: {
            float s[3];
            int k;
            for (k = 0; k < 3; k++)
                s[k] = l->f[3 + k] != 0 ? v : 1;
            glTranslatef(l->f[0], l->f[1], l->f[2]);
            glScalef(s[0], s[1], s[2]);
            glTranslatef(-l->f[0], -l->f[1], -l->f[2]);
            break;
        }
        case L_COLOR:
            index_color(d, (int)v, rgb);
            break;
        }
    }
    glColor3fv(rgb);
    if (n->kind == N_FACE && n->v && n->nv > 0) {
        glBegin(n->nv < 3 || n->mode == 3 ? GL_LINE_STRIP : n->mode == 2 ? GL_LINE_LOOP : GL_POLYGON);
        for (i = 0; i < n->nv; i++)
            glVertex3fv(n->v + 3 * i);
        glEnd();
    } else if (n->kind == N_STRING && text[0]) {
        glPushMatrix();
        glTranslatef(n->pos[0], n->pos[1], n->pos[2]);
        /* the font sizes are those of the modeler, in quarters of the HUD */
        draw_text(text, 4 * n->fw, 4 * n->fh, n->halign == 2);
        glPopMatrix();
    }
    for (i = 0; i < n->nkids; i++)
        draw_node(d, n->kids[i], vars, nvars, depth + 1);
    glPopMatrix();
}

void drtDraw(Drt *d, const float *vars, int nvars)
{
    if (!d)
        return;
    draw_node(d, d->root, vars, nvars, 0);
}
