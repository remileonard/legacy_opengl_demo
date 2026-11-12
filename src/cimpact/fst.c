/*
 * fst.c - loader for the Vega "fast" files (.fst) of Certain Impact.
 *
 * The format was never documented. It was rebuilt from vgLoadFast and
 * the vgFast*::Read methods linked into the IRIX cimpact_ogl executable.
 *
 * A file is a memory image written by Vega 2.0:
 *
 *   0    header: magic 0xf1d05bed, version 2.0 (float), then offsets and
 *        sizes; word 5 is the offset of the directory, word 6 its size,
 *        word 7 the start of the records (all record offsets are relative
 *        to it)
 *   dir  entries of 8 bytes { type, offset }, one per record read, in the
 *        order the reader walks the scene graph
 *
 * Each vgFast*::Read takes the next directory entry, checks its type and
 * reads its record from its offset. Records are big-endian words. A
 * record that was already read (a node or a geostate referenced twice)
 * is found again by its offset and shared.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 2       /* popen */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

#include "fst.h"

enum {
    FST_OBJECT = 1, FST_BILLBOARD = 3, FST_COLORTABLE, FST_FOG, FST_HIGHLIGHT,
    FST_LIGHTARRAY, FST_LIGHTMODEL, FST_SCS, FST_DCS, FST_GSET, FST_GSTATE,
    FST_GEODE, FST_GROUP, FST_LAYER, FST_LPOINT, FST_LOD, FST_SEQ, FST_SWITCH,
    FST_TEX, FST_TENV, FST_MTL, FST_LPSTATE
};

/* Performer geostate modes, in the order of vgFastGeoState::modes */
#define MODE_TRANSPARENCY 0
#define MODE_ALPHAFUNC 3
#define MODE_ENLIGHTING 4
#define MODE_ENTEXTURE 5
#define MODE_CULLFACE 7
static const unsigned int mode_bits[12] = {
    0x10, 0x100, 0x4000, 0x20, 0x2, 0x4, 0x40, 0x200, 0x10000, 0x400, 0x100000, 0x400000
};
/* attributes, in the order of vgFastGeoState::attrs */
#define ATTR_FRONTMTL 0
#define ATTR_TEXTURE 2
#define ATTR_TEXENV 3
#define NATTRS 10

typedef struct Mark {
    unsigned int type, off;
    int x, kind;
    void *ptr;
} Mark;

typedef struct Fst {
    const unsigned char *d;
    size_t size;
    unsigned int dir_off, ndir, r2, di;
    size_t cur;
    int error;
    Mark *marks;
    int nmarks, maxmarks;
} Fst;

static unsigned int rd_u32(Fst *f, size_t o)
{
    const unsigned char *p;
    if (o + 4 > f->size) {
        f->error = 1;
        return 0;
    }
    p = f->d + o;
    return (unsigned int)p[0] << 24 | (unsigned int)p[1] << 16 | (unsigned int)p[2] << 8 | p[3];
}

static int w(Fst *f)
{
    int v = (int)rd_u32(f, f->cur);
    f->cur += 4;
    return v;
}

static float fl(Fst *f)
{
    union { unsigned int u; float f; } u;
    u.u = rd_u32(f, f->cur);
    f->cur += 4;
    return u.f;
}

static double dbl(Fst *f)
{
    union { unsigned char b[8]; double d; } u;
    unsigned int hi = rd_u32(f, f->cur), lo = rd_u32(f, f->cur + 4);
    int i, little = 1;
    f->cur += 8;
    if (*(const char *)&little) {
        for (i = 0; i < 4; i++) {
            u.b[i] = (unsigned char)(lo >> (8 * i));
            u.b[4 + i] = (unsigned char)(hi >> (8 * i));
        }
    } else {
        for (i = 0; i < 4; i++) {
            u.b[3 - i] = (unsigned char)(hi >> (8 * i));
            u.b[7 - i] = (unsigned char)(lo >> (8 * i));
        }
    }
    return u.d;
}

static void floats(Fst *f, float *dst, int n)
{
    int i;
    for (i = 0; i < n; i++)
        dst[i] = fl(f);
}

static void *find_mark(Fst *f, unsigned int type, unsigned int off, int x, int kind)
{
    int i;
    for (i = f->nmarks - 1; i >= 0; i--) {
        Mark *m = &f->marks[i];
        if (m->type == type && m->off == off && m->x == x && m->kind == kind)
            return m->ptr;
    }
    return NULL;
}

static void add_mark(Fst *f, unsigned int type, unsigned int off, int x, int kind, void *ptr)
{
    if (f->nmarks == f->maxmarks) {
        f->maxmarks = f->maxmarks ? 2 * f->maxmarks : 256;
        f->marks = (Mark *)realloc(f->marks, sizeof(Mark) * f->maxmarks);
    }
    f->marks[f->nmarks].type = type;
    f->marks[f->nmarks].off = off;
    f->marks[f->nmarks].x = x;
    f->marks[f->nmarks].kind = kind;
    f->marks[f->nmarks].ptr = ptr;
    f->nmarks++;
}

/* type of the next directory entry */
static unsigned int peek_dir(Fst *f)
{
    if (f->di >= f->ndir)
        return 0;
    return rd_u32(f, f->dir_off + 8 * f->di);
}

/* readDirSeekLoc: takes the next directory entry, moves to its record */
static int seek_dir(Fst *f, unsigned int type, unsigned int *off)
{
    unsigned int t, o;
    if (f->di >= f->ndir) {
        f->error = 1;
        return -1;
    }
    t = rd_u32(f, f->dir_off + 8 * f->di);
    o = rd_u32(f, f->dir_off + 8 * f->di + 4);
    f->di++;
    if (t != type) {
        fprintf(stderr, "fst: found %u in directory, expected %u\n", t, type);
        f->error = 1;
        return -1;
    }
    f->cur = f->r2 + o;
    if (off)
        *off = o;
    return 0;
}

/* a string: a flag, its length and the characters padded to 4 bytes */
static char *read_string(Fst *f)
{
    int n;
    char *s;
    if (w(f) != 1)
        return NULL;
    n = w(f);
    if (n <= 0 || f->cur + n > f->size) {
        f->error = n < 0 || f->cur + n > f->size;
        return NULL;
    }
    s = (char *)malloc(n + 1);
    memcpy(s, f->d + f->cur, n);
    s[n] = 0;
    f->cur += (n + 3) & ~3;
    return s;
}

static void node_head(Fst *f, pfNode *n)
{
    float bs[4];
    n->name = read_string(f);
    floats(f, bs, 4);       /* bounding sphere */
}

/* --- states ------------------------------------------------------------- */

static pfMaterial *read_mtl(Fst *f)
{
    pfMaterial *m = (pfMaterial *)calloc(1, sizeof(pfMaterial));
    if (seek_dir(f, FST_MTL, NULL))
        return m;
    w(f);                   /* side */
    m->alpha = fl(f);
    m->shininess = fl(f);
    floats(f, m->ambient, 3);
    floats(f, m->diffuse, 3);
    floats(f, m->emission, 3);
    floats(f, m->specular, 3);
    m->colormode = w(f) == GL_AMBIENT_AND_DIFFUSE;
    return m;
}

static pfTexEnv *read_tenv(Fst *f)
{
    pfTexEnv *t = (pfTexEnv *)calloc(1, sizeof(pfTexEnv));
    if (seek_dir(f, FST_TENV, NULL))
        return t;
    w(f);                   /* component */
    t->mode = w(f);
    floats(f, t->blend, 4);
    return t;
}

/* the texels: comps bytes each, bottom row first, converted to RGBA */
static pfTexture *read_tex(Fst *f)
{
    pfTexture *t = (pfTexture *)calloc(1, sizeof(pfTexture));
    int comps, xs, ys, zs, i, wrap[3];
    unsigned int off;
    size_t size;
    const unsigned char *src;
    unsigned char *dst;

    if (seek_dir(f, FST_TEX, NULL))
        return t;
    t->name = read_string(f);
    comps = w(f);
    xs = w(f);
    ys = w(f);
    zs = w(f);
    for (i = 0; i < 3; i++)
        w(f);               /* filters */
    for (i = 0; i < 3; i++)
        wrap[i] = w(f);
    for (i = 0; i < 4; i++)
        w(f);
    if (w(f) == 1) {
        fprintf(stderr, "fst: detail textures are not supported\n");
        f->error = 1;
        return t;
    }
    w(f);
    off = (unsigned int)w(f);
    size = (size_t)comps * xs * ys * (zs > 0 ? zs : 1);
    if (comps < 1 || comps > 4 || xs <= 0 || ys <= 0 || f->r2 + off + size > f->size) {
        f->error = 1;
        return t;
    }
    src = f->d + f->r2 + off;
    t->xsize = xs;
    t->ysize = ys;
    t->comps = comps;
    /* the wraps are stored for s, t and r */
    t->wrap_s = wrap[0] == GL_CLAMP ? GL_CLAMP : GL_REPEAT;
    t->wrap_t = wrap[1] == GL_CLAMP ? GL_CLAMP : GL_REPEAT;
    t->image = (unsigned int *)malloc((size_t)xs * ys * 4);
    dst = (unsigned char *)t->image;
    for (i = 0; i < xs * ys; i++, src += comps, dst += 4) {
        switch (comps) {
        case 1: dst[0] = dst[1] = dst[2] = src[0]; dst[3] = 255; break;
        case 2: dst[0] = dst[1] = dst[2] = src[0]; dst[3] = src[1]; break;
        case 3: dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; dst[3] = 255; break;
        default: memcpy(dst, src, 4); break;
        }
    }
    return t;
}

static pfGeoState *read_gstate(Fst *f)
{
    pfGeoState *gs, *old;
    size_t save = f->cur;
    unsigned int off, inherit;
    int modes[12], i;
    float alpharef;

    if (seek_dir(f, FST_GSTATE, &off))
        return NULL;
    if ((old = (pfGeoState *)find_mark(f, FST_GSTATE, off, 0, -1)) != NULL) {
        f->cur = save;
        return old;
    }
    gs = (pfGeoState *)calloc(1, sizeof(pfGeoState));
    inherit = (unsigned int)w(f);
    for (i = 0; i < 12; i++)
        modes[i] = w(f);
    alpharef = fl(f);

    /* inherited modes take the defaults of Vega's global state */
#define INH(m) (inherit & mode_bits[m])
    gs->lighting = INH(MODE_ENLIGHTING) ? 1 : modes[MODE_ENLIGHTING] != 0;
    gs->texturing = INH(MODE_ENTEXTURE) ? 1 : modes[MODE_ENTEXTURE] != 0;
    gs->transparency = INH(MODE_TRANSPARENCY) ? 0 : modes[MODE_TRANSPARENCY] != 0;
    gs->lpoint = 0;
    if (!INH(MODE_ALPHAFUNC) && modes[MODE_ALPHAFUNC] != GL_ALWAYS && modes[MODE_ALPHAFUNC] != 0)
        gs->alphafunc = (unsigned int)modes[MODE_ALPHAFUNC];
    gs->alpharef = alpharef;
    if (INH(MODE_CULLFACE)) {
        /* the gfx of cimpact.adf has "backface off": no culling by default */
        gs->cullface = 0;
    } else {
        static const unsigned int cull[4] = { 0, GL_BACK, GL_FRONT, GL_FRONT_AND_BACK };
        gs->cullface = modes[MODE_CULLFACE] >= 0 && modes[MODE_CULLFACE] < 4 ? cull[modes[MODE_CULLFACE]] : 0;
    }
#undef INH

    for (i = 0; i < NATTRS && !f->error; i++) {
        unsigned int t;
        if (w(f) != 1)
            continue;
        t = peek_dir(f);
        if (t == FST_MTL) {
            pfMaterial *m = read_mtl(f);
            if (i == ATTR_FRONTMTL)
                gs->frontmtl = m;
        } else if (t == FST_TEX) {
            pfTexture *x = read_tex(f);
            if (i == ATTR_TEXTURE)
                gs->texture = x;
        } else if (t == FST_TENV) {
            pfTexEnv *x = read_tenv(f);
            if (i == ATTR_TEXENV)
                gs->texenv = x;
        } else {
            fprintf(stderr, "fst: geostate attribute of type %u not supported\n", t);
            f->error = 1;
        }
    }
    add_mark(f, FST_GSTATE, off, 0, -1, gs);
    return gs;
}

/* --- geosets ------------------------------------------------------------ */

static int num_items(const pfGeoSet *g)
{
    int i, n = 0;
    switch (g->prim) {
    case PFGS_POINTS: return g->nprims;
    case PFGS_LINES: return 2 * g->nprims;
    case PFGS_TRIS: return 3 * g->nprims;
    case PFGS_QUADS: return 4 * g->nprims;
    case PFGS_LINESTRIPS: case PFGS_TRISTRIPS:
    case PFGS_FLAT_LINESTRIPS: case PFGS_FLAT_TRISTRIPS:
        for (i = 0; i < g->nprims && g->lengths; i++)
            n += g->lengths[i];
        return n;
    }
    return 0;
}

static pfGeoSet *read_gset(Fst *f)
{
    static const int comps[4] = { 4, 3, 2, 3 };
    pfGeoSet *g = pfNewGSet(NULL);
    int i, k, n;

    if (seek_dir(f, FST_GSET, NULL))
        return g;
    g->prim = w(f);
    g->nprims = w(f);
    if (g->nprims < 0 || g->nprims > 10000000) {
        f->error = 1;
        g->nprims = 0;
        return g;
    }
    if (w(f) == 1) {
        g->lengths = (int *)malloc(sizeof(int) * (g->nprims + 1));
        for (i = 0; i < g->nprims; i++)
            g->lengths[i] = w(f);
    }
    g->flatshade = w(f) != 0;   /* draw modes: flat shading, wireframe, compile */
    w(f);
    w(f);
    g->linewidth = fl(f);
    g->pntsize = fl(f);
    if (w(f) == 1)
        g->gstate = read_gstate(f);
    n = num_items(g);
    /* the attributes follow at the offset of the next word pair */
    w(f);
    f->cur = f->r2 + (unsigned int)w(f);
    for (k = 0; k < 4 && !f->error; k++) {
        int bind = w(f);
        g->bind[k] = bind;
        if (bind) {
            unsigned int t = (unsigned int)w(f), off = (unsigned int)w(f);
            int x = w(f);
            float *a = (float *)find_mark(f, t, off, x, 0);
            if (!a) {
                a = (float *)malloc(sizeof(float) * comps[k] * (n > 0 ? n : 1));
                floats(f, a, comps[k] * n);
                add_mark(f, t, off, x, 0, a);
            }
            g->attr[k] = a;
        }
        if (w(f) == 1) {
            /* index lists, not used by Certain Impact */
            unsigned int t = (unsigned int)w(f), off = (unsigned int)w(f);
            int x = w(f);
            if (!find_mark(f, t, off, x, 1)) {
                int size = w(f);
                fprintf(stderr, "fst: indexed geosets are not supported\n");
                f->cur += size;
                add_mark(f, t, off, x, 1, g);
            }
        }
    }
    g->nverts = n;
    return g;
}

/* --- nodes -------------------------------------------------------------- */

static pfNode *read_child(Fst *f);

static void read_children(Fst *f, pfNode *n)
{
    int i, count = w(f);
    for (i = 0; i < count && !f->error; i++) {
        pfNode *c = read_child(f);
        if (c)
            pfAddChild(n, c);
    }
}

static pfNode *read_group(Fst *f)
{
    pfNode *n = pfNewGroup();
    if (seek_dir(f, FST_GROUP, NULL))
        return n;
    w(f);
    node_head(f, n);
    read_children(f, n);
    return n;
}

static void read_mat(Fst *f, pfNode *n)
{
    pfMatrix m;
    floats(f, &m[0][0], 16);
    memcpy(n->mat, m, sizeof(pfMatrix));
}

static pfNode *read_dcs(Fst *f, unsigned int type)
{
    pfNode *n = pfNewDCS();
    if (seek_dir(f, type, NULL))
        return n;
    node_head(f, n);
    read_mat(f, n);
    if (type == FST_SCS)
        n->type = PFNODE_SCS;
    else
        pfDCSMat(n, n->mat);
    read_children(f, n);
    return n;
}

static pfNode *read_layer(Fst *f)
{
    pfNode *n = pfNewGroup();
    if (seek_dir(f, FST_LAYER, NULL))
        return n;
    node_head(f, n);
    w(f);                   /* mode */
    n->type = PFNODE_LAYER;
    read_children(f, n);
    return n;
}

static pfNode *read_switch(Fst *f)
{
    pfNode *n = pfNewSwitch();
    if (seek_dir(f, FST_SWITCH, NULL))
        return n;
    node_head(f, n);
    n->switchval = w(f);
    read_children(f, n);
    return n;
}

static pfNode *read_lod(Fst *f)
{
    pfNode *n = pfNewGroup();
    int i, k;
    float c[3];
    if (seek_dir(f, FST_LOD, NULL))
        return n;
    n->type = PFNODE_LOD;
    node_head(f, n);
    floats(f, c, 3);        /* center */
    k = w(f);
    for (i = 0; i < k; i++)
        fl(f);              /* ranges */
    read_children(f, n);
    return n;
}

static pfNode *read_seq(Fst *f)
{
    pfNode *n = pfNewGroup();
    int i;
    if (seek_dir(f, FST_SEQ, NULL))
        return n;
    n->type = PFNODE_SEQUENCE;
    node_head(f, n);
    n->nframes = w(f);
    n->seqmode = w(f);
    n->interval = w(f);
    n->begin = w(f);
    n->end = w(f);
    n->speed = fl(f);
    n->nreps = w(f);
    if (n->nframes < 0 || n->nframes > 100000) {
        f->error = 1;
        return n;
    }
    n->times = (float *)calloc(n->nframes + 1, sizeof(float));
    if (w(f) == 1)
        for (i = 0; i < n->nframes; i++)
            n->times[i] = (float)dbl(f);
    n->frame = n->begin;
    n->dir = 1;
    n->frame_start = pfGetTime();
    read_children(f, n);
    return n;
}

/* geodes and billboards: shared when read twice */
static pfNode *read_geode(Fst *f, unsigned int type)
{
    size_t save = f->cur;
    unsigned int off;
    pfNode *n, *old;
    int first, i, count;

    if (seek_dir(f, type, &off))
        return NULL;
    if ((old = (pfNode *)find_mark(f, type, off, 0, -1)) != NULL) {
        f->cur = save;
        return old;
    }
    n = pfNewGeode();
    if (type == FST_BILLBOARD)
        n->type = PFNODE_BBOARD;
    first = w(f);
    node_head(f, n);
    if (type == FST_BILLBOARD) {
        n->bbmode = w(f);
        if (w(f) == 1 && first > 0 && first < 100000) {
            n->bbpos = (pfVec3 *)calloc(first, sizeof(pfVec3));
            floats(f, &n->bbpos[0][0], 3 * first);
        }
    }
    count = w(f);
    for (i = 0; i < count && !f->error; i++)
        pfAddGSet(n, read_gset(f));
    if (type == FST_BILLBOARD && !n->bbpos)
        n->type = PFNODE_GEODE;
    add_mark(f, type, off, 0, -1, n);
    return n;
}

/* light points are skipped */
static pfNode *read_lpoint(Fst *f)
{
    pfNode *n = pfNewGroup();
    int k;
    if (seek_dir(f, FST_LPOINT, NULL))
        return n;
    node_head(f, n);
    k = w(f);
    w(f);
    if (w(f) == 1)
        f->cur += 16 * (size_t)k;
    if (w(f) == 1)
        f->cur += 12 * (size_t)k;
    f->cur += 36;
    return n;
}

static pfNode *read_child(Fst *f)
{
    switch (peek_dir(f)) {
    case FST_BILLBOARD: return read_geode(f, FST_BILLBOARD);
    case FST_SCS: return read_dcs(f, FST_SCS);
    case FST_DCS: return read_dcs(f, FST_DCS);
    case FST_GEODE: return read_geode(f, FST_GEODE);
    case FST_GROUP: return read_group(f);
    case FST_LAYER: return read_layer(f);
    case FST_LPOINT: return read_lpoint(f);
    case FST_LOD: return read_lod(f);
    case FST_SEQ: return read_seq(f);
    case FST_SWITCH: return read_switch(f);
    }
    fprintf(stderr, "fst: can't read node of type %u\n", peek_dir(f));
    f->error = 1;
    return NULL;
}

/* the codes come by groups of 8: rounds a bit position up to the next group */
static size_t align_group(size_t bits, int n_bits)
{
    size_t g = (size_t)n_bits << 3;
    return bits == 0 ? 0 : (bits - 1) + (g - (bits - 1 + g) % g);
}

/* decompresses a file of the Unix compress program (.Z, LZW) */
static unsigned char *unlzw(const unsigned char *in, size_t inlen, size_t *outlen)
{
    static unsigned short prefix[65536];
    static unsigned char suffix[65536], stack[65536];
    unsigned char *out = NULL, finchar = 0;
    size_t n = 0, cap = 0, posbits = 24, base = 24, nbits_total = inlen * 8;
    int maxbits, block, n_bits = 9, bitmask = 0x1ff;
    long maxcode = 511, maxmaxcode, free_ent, oldcode = -1, code, incode;
    if (inlen < 3 || in[0] != 0x1f || in[1] != 0x9d)
        return NULL;
    maxbits = in[2] & 0x1f;
    block = in[2] & 0x80;
    if (maxbits < 9 || maxbits > 16)
        return NULL;
    maxmaxcode = 1L << maxbits;
    free_ent = block ? 257 : 256;
    for (code = 0; code < 256; code++) {
        prefix[code] = 0;
        suffix[code] = (unsigned char)code;
    }
    for (;;) {
        unsigned char *sp;
        size_t b;
        if (free_ent > maxcode) {
            /* the codes grow by one bit: skip to the end of the group of 8 codes */
            posbits = base + align_group(posbits - base, n_bits);
            base = posbits;
            n_bits++;
            maxcode = n_bits == maxbits ? maxmaxcode : (1L << n_bits) - 1;
            bitmask = (1 << n_bits) - 1;
            continue;
        }
        if (posbits + n_bits > nbits_total)
            break;
        b = posbits >> 3;
        code = (long)in[b] | (long)(b + 1 < inlen ? in[b + 1] : 0) << 8 | (long)(b + 2 < inlen ? in[b + 2] : 0) << 16;
        code = (code >> (posbits & 7)) & bitmask;
        posbits += n_bits;
        if (n + 65536 > cap) {
            cap = cap ? 2 * cap : 1 << 20;
            out = (unsigned char *)realloc(out, cap);
        }
        if (oldcode == -1) {
            if (code >= 256)
                break;
            out[n++] = finchar = (unsigned char)code;
            oldcode = code;
            continue;
        }
        if (code == 256 && block) {
            free_ent = 256;
            posbits = base + align_group(posbits - base, n_bits);
            base = posbits;
            n_bits = 9;
            maxcode = 511;
            bitmask = 0x1ff;
            continue;
        }
        incode = code;
        sp = stack + sizeof(stack);
        if (code >= free_ent) {
            if (code > free_ent)
                break;
            *--sp = finchar;
            code = oldcode;
        }
        while (code >= 256 && sp > stack) {
            *--sp = suffix[code];
            code = prefix[code];
        }
        *--sp = finchar = suffix[code];
        while (sp < stack + sizeof(stack))
            out[n++] = *sp++;
        if (free_ent < maxmaxcode) {
            prefix[free_ent] = (unsigned short)oldcode;
            suffix[free_ent] = finchar;
            free_ent++;
        }
        oldcode = incode;
    }
    *outlen = n;
    return out;
}

static unsigned char *read_whole(const char *path, size_t *size)
{
    FILE *fp;
    unsigned char *buf = NULL;
    size_t n = 0, cap = 0, r;
    int piped = 0;
    size_t len = strlen(path);

    if (len > 3 && strcmp(path + len - 3, ".gz") == 0) {
        char cmd[4200];
        sprintf(cmd, "gzip -dc \"%.4096s\"", path);
        fp = popen(cmd, "r");
        piped = 1;
    } else {
        fp = fopen(path, "rb");
    }
    if (!fp)
        return NULL;
    for (;;) {
        if (n == cap) {
            cap = cap ? 2 * cap : 1 << 20;
            buf = (unsigned char *)realloc(buf, cap);
        }
        r = fread(buf + n, 1, cap - n, fp);
        if (r == 0)
            break;
        n += r;
    }
    if (piped)
        pclose(fp);
    else
        fclose(fp);
    if (n >= 3 && buf[0] == 0x1f && buf[1] == 0x9d) {
        unsigned char *out = unlzw(buf, n, &n);
        free(buf);
        buf = out;
    }
    *size = n;
    return buf;
}

pfNode *fstLoadFile(const char *path)
{
    Fst f;
    pfNode *root = NULL;
    unsigned char *data;
    union { unsigned int u; float f; } version;

    memset(&f, 0, sizeof(f));
    if ((data = read_whole(path, &f.size)) == NULL) {
        fprintf(stderr, "fst: can't read %s\n", path);
        return NULL;
    }
    f.d = data;
    if (f.size < 556 || rd_u32(&f, 0) != 0xf1d05bed) {
        fprintf(stderr, "fst: %s is not a fast file\n", path);
        free(data);
        return NULL;
    }
    version.u = rd_u32(&f, 4);
    if (version.f != 2.0f) {
        fprintf(stderr, "fst: can't read fast file version %g\n", version.f);
        free(data);
        return NULL;
    }
    f.dir_off = rd_u32(&f, 20);
    f.ndir = rd_u32(&f, 24) / 8;
    f.r2 = rd_u32(&f, 28);
    if (f.dir_off + 8 * (size_t)f.ndir > f.size)
        f.error = 1;
    if (!f.error && !seek_dir(&f, FST_OBJECT, NULL)) {
        root = pfNewGroup();
        if (peek_dir(&f) == FST_GROUP) {
            pfAddChild(root, read_group(&f));
        } else {
            pfNode *g = read_geode(&f, FST_GEODE);
            if (g)
                pfAddChild(root, g);
        }
    }
    if (f.error)
        fprintf(stderr, "fst: %s: read error near directory entry %u\n", path, f.di);
    free(f.marks);
    free(data);
    return root;
}
