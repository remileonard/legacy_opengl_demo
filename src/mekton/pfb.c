/*
 * pfb.c - loader for IRIS Performer binary files (.pfb).
 *
 * File layout (all values big-endian 32-bit):
 *
 *   header   0xDB0ACE00, version 1, 1, header size 16
 *   chunks   type, record count, byte size, records...
 *
 * Chunk types used by Mekton:
 *
 *    0 materials      72 bytes each (pfMaterial)
 *    1 textures       name length, name, 224 bytes, optional arrays
 *    2 texenvs        28 bytes each
 *    3 gstates        word count, (key, value...) pairs, user data
 *    4 lengths        count, flag, user data, int[count]
 *    5 coords         same with float[3]
 *    6 colors         same with float[4]
 *    7 normals        same with float[3]
 *    8 texcoords      same with float[2]
 *   10 geosets        132 bytes each
 *   11 user data      byte count, bytes
 *   12 nodes          word count, words, name length, name
 *   14 morphs         word count, words
 *   21 light points   104 bytes each (pfLPointState)
 *
 * Indices between records are positions in the chunk of the referenced
 * type. Node 0 is the root of the scene.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfb.h"

#define PFB_MAGIC 0xDB0ACE00UL

typedef struct {
    const unsigned char *p, *end;
    int error;
} Reader;

static unsigned long rd_u32(Reader *r)
{
    unsigned long v;
    if (r->p + 4 > r->end) {
        r->error = 1;
        return 0;
    }
    v = ((unsigned long)r->p[0] << 24) | ((unsigned long)r->p[1] << 16) |
        ((unsigned long)r->p[2] << 8) | (unsigned long)r->p[3];
    r->p += 4;
    return v;
}

static int rd_int(Reader *r)
{
    unsigned long v = rd_u32(r);
    if (v & 0x80000000UL)
        return -(int)(0x7fffffffUL - (v & 0x7fffffffUL)) - 1;
    return (int)v;
}

static float bits_to_float(unsigned long v)
{
    union { unsigned int u; float f; } u;
    u.u = (unsigned int)v;
    return u.f;
}

static float rd_float(Reader *r)
{
    return bits_to_float(rd_u32(r));
}

static void rd_floats(Reader *r, float *dst, int n)
{
    int i;
    for (i = 0; i < n; i++)
        dst[i] = rd_float(r);
}

static void rd_skip(Reader *r, long n)
{
    if (n < 0 || r->p + n > r->end) {
        r->error = 1;
        r->p = r->end;
        return;
    }
    r->p += n;
}

static char *rd_name(Reader *r)
{
    int len = rd_int(r);
    char *s;
    if (len == -1 || r->error)
        return NULL;
    if (len < 0 || r->p + len > r->end) {
        r->error = 1;
        return NULL;
    }
    s = (char *)malloc(len + 1);
    memcpy(s, r->p, len);
    s[len] = 0;
    r->p += len;
    return s;
}

static int *rd_words(Reader *r, int *count)
{
    int n = rd_int(r), i, *w;
    if (n < 0 || r->error || r->p + 4L * n > r->end) {
        r->error = 1;
        *count = 0;
        return NULL;
    }
    w = (int *)malloc(sizeof(int) * (n + 1));
    for (i = 0; i < n; i++)
        w[i] = rd_int(r);
    *count = n;
    return w;
}

static float word_float(int w)
{
    return bits_to_float((unsigned long)(unsigned int)w);
}

/* --- records ------------------------------------------------------------ */

static void read_mtl(Reader *r, PfbMtl *m)
{
    m->side = rd_int(r);
    m->alpha = rd_float(r);
    m->shininess = rd_float(r);
    rd_floats(r, m->ambient, 3);
    rd_floats(r, m->diffuse, 3);
    rd_floats(r, m->specular, 3);
    rd_floats(r, m->emission, 3);
    m->cmode_side = rd_int(r);
    m->cmode = rd_int(r);
    rd_int(r);  /* user data */
}

static void read_tex(Reader *r, PfbTex *t)
{
    const unsigned char *body;
    Reader b;
    int nimages, nlevels;

    memset(t, 0, sizeof(*t));
    t->name = rd_name(r);
    body = r->p;
    rd_skip(r, 224);
    if (r->error)
        return;
    b.p = body + 0xd0;
    b.end = body + 224;
    b.error = 0;
    nimages = rd_int(&b);
    rd_int(&b);
    nlevels = rd_int(&b);
    if (nimages > 0)
        rd_skip(r, 4L * nimages);
    if (nlevels > 0)
        rd_skip(r, 8L * nlevels);
}

static void read_tenv(Reader *r, PfbTEnv *e)
{
    e->mode = rd_int(r);
    rd_int(r);  /* component */
    rd_floats(r, e->blend, 4);
    rd_int(r);  /* user data */
}

static void read_gstate(Reader *r, PfbGState *g)
{
    int n, i = 0, *w = rd_words(r, &n);

    g->transparency = g->alphafunc = g->lighting = g->texturing = PFB_INHERIT;
    g->fog = g->cullface = g->lpoint = PFB_INHERIT;
    g->frontmtl = g->backmtl = g->texture = g->texenv = g->lpstate = -1;
    if (!w)
        return;
    /* the last word is the user data index */
    while (i < n - 1) {
        int key = w[i++];
        int v = (i < n) ? w[i] : 0;
        switch (key) {
        case PFB_GS_TRANSPARENCY: g->transparency = v; i++; break;
        case PFB_GS_ALPHAFUNC:    g->alphafunc = v; i++; break;
        case PFB_GS_ENLIGHTING:   g->lighting = v; i++; break;
        case PFB_GS_ENTEXTURE:    g->texturing = v; i++; break;
        case PFB_GS_ENFOG:        g->fog = v; i++; break;
        case PFB_GS_CULLFACE:     g->cullface = v; i++; break;
        case PFB_GS_ENLPOINTSTATE: g->lpoint = v; i++; break;
        case PFB_GS_FRONTMTL:     g->frontmtl = v; i++; break;
        case PFB_GS_BACKMTL:      g->backmtl = v; i++; break;
        case PFB_GS_TEXTURE:      g->texture = v; i++; break;
        case PFB_GS_TEXENV:       g->texenv = v; i++; break;
        case PFB_GS_LPOINTSTATE:  g->lpstate = v; i++; break;
        case PFB_GS_DECAL:        i += 3; break;     /* mode, id, offset */
        case PFB_GS_LIGHTS:       i += 1 + v; break; /* count, lights */
        default:                  i++; break;
        }
    }
    free(w);
}

static void read_list(Reader *r, int type, PfbList *l)
{
    static const int comps[9] = { 0, 0, 0, 0, 1, 3, 4, 3, 2 };
    int i, n;

    memset(l, 0, sizeof(*l));
    n = rd_int(r);
    rd_int(r);  /* 1: pfCycleBuffer */
    rd_int(r);  /* user data */
    if (n < 0 || r->error || r->p + 4L * n * comps[type] > r->end) {
        r->error = 1;
        return;
    }
    l->count = n;
    l->comps = comps[type];
    if (type == 4) {
        l->i = (int *)malloc(sizeof(int) * (n + 1));
        for (i = 0; i < n; i++)
            l->i[i] = rd_int(r);
    } else {
        l->f = (float *)malloc(sizeof(float) * (n * l->comps + 1));
        rd_floats(r, l->f, n * l->comps);
    }
}

static void read_gset(Reader *r, PfbGSet *g)
{
    int i;
    g->prim = rd_int(r);
    g->nprims = rd_int(r);
    g->lengths = rd_int(r);
    for (i = 0; i < 4; i++) {
        g->attr[i].bind = rd_int(r);
        g->attr[i].list = rd_int(r);
        g->attr[i].ilist = rd_int(r);
    }
    g->flatshade = rd_int(r) == 2;
    g->wireframe = rd_int(r) == 2;
    rd_int(r);  /* compile GL */
    g->gstate = rd_int(r);
    rd_int(r);  /* gstate index */
    g->linewidth = rd_float(r);
    g->pointsize = rd_float(r);
    rd_int(r);  /* draw bin */
    rd_int(r);  /* isect mask */
    rd_int(r);  /* highlight */
    rd_int(r);
    rd_floats(r, g->bbox, 6);
    rd_int(r);  /* user data */
}

static void read_node(Reader *r, PfbNode *nd)
{
    int n, p = 1, i, k, *w = rd_words(r, &n);

    memset(nd, 0, sizeof(*nd));
    for (i = 0; i < 16; i++)
        nd->matrix[i] = (i % 5) == 0;
    nd->ref = -1;
    if (w && n > 0) {
        nd->type = w[0];
        switch (nd->type) {
        case PFB_NODE_TEXT:
        case PFB_NODE_GEODE:
            nd->ngsets = n > 1 && w[1] >= 0 && w[1] < n ? w[1] : 0;
            p = 2;
            if (nd->type == PFB_NODE_TEXT)
                p += nd->ngsets;  /* string indices; no gsets */
            break;
        case PFB_NODE_BBOARD:
            nd->ngsets = n > 1 && w[1] >= 0 && w[1] < n ? w[1] : 0;
            p = 2 + 3 * nd->ngsets + 4;
            break;
        case PFB_NODE_REF23:
        case PFB_NODE_MORPH:
            nd->ref = n > 1 ? w[1] : -1;
            p = 2;
            break;
        case PFB_NODE_SCS:
        case PFB_NODE_DCS:
            p = nd->type == PFB_NODE_DCS ? 2 : 1;
            for (i = 0; i < 16 && p + i < n; i++)
                nd->matrix[i] = word_float(w[p + i]);
            p += 16;
            break;
        case PFB_NODE_SCENE:
            p = 3;
            break;
        case PFB_NODE_SWITCH:
            nd->switch_val = n > 1 ? w[1] : -1;
            p = 2;
            break;
        case PFB_NODE_LOD:
            k = n > 1 && w[1] >= 0 && w[1] < n ? w[1] : 0;
            p = 2 + 2 * (k + 1) + 3 + 2;
            break;
        case PFB_NODE_SEQUENCE:
            nd->nframes = n > 1 && w[1] >= 0 && 2 + w[1] + 6 <= n ? w[1] : 0;
            if (nd->nframes == 0) {
                p = n;
                break;
            }
            nd->times = (float *)malloc(sizeof(float) * (nd->nframes + 1));
            for (i = 0; i < nd->nframes; i++)
                nd->times[i] = word_float(w[2 + i]);
            p = 2 + nd->nframes;
            nd->seq_mode = w[p];
            nd->seq_begin = w[p + 1];
            nd->seq_end = w[p + 2];
            nd->seq_speed = word_float(w[p + 3]);
            nd->seq_reps = w[p + 4];
            p += 6;
            break;
        case PFB_NODE_LAYER:
            p = 4;
            break;
        default:
            break;
        }
        if (nd->ngsets > n - p)
            nd->ngsets = p < n ? n - p : 0;
        if (p < n) {
            if (nd->type == PFB_NODE_GEODE || nd->type == PFB_NODE_BBOARD) {
                nd->gsets = (int *)malloc(sizeof(int) * (nd->ngsets + 1));
                for (i = 0; i < nd->ngsets && p < n; i++)
                    nd->gsets[i] = w[p++];
            } else if (nd->type == PFB_NODE_TEXT) {
                nd->ngsets = 0;
            } else {
                nd->nchildren = w[p++];
                if (nd->nchildren < 0 || p + nd->nchildren > n)
                    nd->nchildren = 0;
                nd->children = (int *)malloc(sizeof(int) * (nd->nchildren + 1));
                for (i = 0; i < nd->nchildren; i++)
                    nd->children[i] = w[p++];
            }
        }
    }
    free(w);
    nd->name = rd_name(r);
}

/*
 * pfMorph: number of attributes, then for each attribute
 *   attr, floats per element, elements, sources, dest list, index flag,
 *   source lists[sources]
 * followed by the weights (one per source).
 */
static void apply_morph(PfbFile *f, const int *w, int n, const float *weights)
{
    static const int list_type[6] = { 0, 6, 7, 8, 5, 0 };
    int nattr, a, p = 1, i, j;
    struct { int type, fpe, nelts, nsrc, dst, src[8]; } at[8];

    if (n < 1)
        return;
    nattr = w[0];
    if (nattr > 8)
        return;
    for (a = 0; a < nattr; a++) {
        if (p + 6 > n)
            return;
        at[a].type = w[p] >= 1 && w[p] <= 4 ? list_type[w[p]] : 0;
        at[a].fpe = w[p + 1];
        at[a].nelts = w[p + 2];
        at[a].nsrc = w[p + 3];
        at[a].dst = w[p + 4];
        if (w[p + 5] != 0 || at[a].nsrc > 8)
            return;  /* indexed morphs are not used by Mekton */
        p += 6;
        for (i = 0; i < at[a].nsrc && p < n; i++)
            at[a].src[i] = w[p++];
    }
    for (a = 0; a < nattr; a++) {
        PfbList *dl;
        int t = at[a].type, cnt;
        if (!t || at[a].dst < 0 || at[a].dst >= f->nlist[t] ||
            p + at[a].nsrc > n)
            continue;
        dl = &f->list[t][at[a].dst];
        cnt = at[a].nelts * at[a].fpe;
        if (cnt > dl->count * dl->comps)
            cnt = dl->count * dl->comps;
        for (j = 0; j < cnt; j++) {
            float v = 0.0f;
            for (i = 0; i < at[a].nsrc; i++) {
                int s = at[a].src[i];
                float wt = weights ? weights[i] : word_float(w[p + i]);
                if (s >= 0 && s < f->nlist[t] && j < f->list[t][s].count * f->list[t][s].comps)
                    v += wt * f->list[t][s].f[j];
            }
            dl->f[j] = v;
        }
    }
}

int pfb_morph_sources(const PfbFile *f, int morph)
{
    if (morph < 0 || morph >= f->nmorph || !f->morph[morph] ||
        f->morph_len[morph] < 5 || f->morph[morph][0] < 1)
        return 0;
    return f->morph[morph][4];
}

void pfb_morph(PfbFile *f, int morph, const float *weights)
{
    if (morph >= 0 && morph < f->nmorph && f->morph[morph])
        apply_morph(f, f->morph[morph], f->morph_len[morph], weights);
}

/* --- file --------------------------------------------------------------- */

static void *grow(void *p, int *count, int add, size_t size)
{
    p = realloc(p, size * (*count + add + 1));
    memset((char *)p + size * *count, 0, size * add);
    *count += add;
    return p;
}

PfbFile *pfb_load(const char *path)
{
    FILE *fp = fopen(path, "rb");
    unsigned char *buf;
    long size;
    Reader r;
    PfbFile *f;
    int i;

    if (!fp)
        return NULL;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = (unsigned char *)malloc(size > 0 ? size : 1);
    if (fread(buf, 1, size, fp) != (size_t)size) {
        fclose(fp);
        free(buf);
        return NULL;
    }
    fclose(fp);

    r.p = buf;
    r.end = buf + size;
    r.error = 0;
    if (rd_u32(&r) != PFB_MAGIC) {
        free(buf);
        return NULL;
    }
    rd_u32(&r);
    rd_u32(&r);
    r.p = buf + rd_u32(&r);

    f = (PfbFile *)calloc(1, sizeof(PfbFile));
    while (!r.error && r.p + 12 <= r.end) {
        int type = rd_int(&r), count = rd_int(&r), bytes = rd_int(&r), base;
        const unsigned char *next = r.p + bytes;
        if (bytes < 0 || next > r.end || count < 0 || count > bytes / 4)
            break;
        switch (type) {
        case 0:
            base = f->nmtl;
            f->mtl = (PfbMtl *)grow(f->mtl, &f->nmtl, count, sizeof(PfbMtl));
            for (i = 0; i < count && !r.error; i++)
                read_mtl(&r, &f->mtl[base + i]);
            break;
        case 1:
            base = f->ntex;
            f->tex = (PfbTex *)grow(f->tex, &f->ntex, count, sizeof(PfbTex));
            for (i = 0; i < count && !r.error; i++)
                read_tex(&r, &f->tex[base + i]);
            break;
        case 2:
            base = f->ntenv;
            f->tenv = (PfbTEnv *)grow(f->tenv, &f->ntenv, count, sizeof(PfbTEnv));
            for (i = 0; i < count && !r.error; i++)
                read_tenv(&r, &f->tenv[base + i]);
            break;
        case 3:
            base = f->ngstate;
            f->gstate = (PfbGState *)grow(f->gstate, &f->ngstate, count, sizeof(PfbGState));
            for (i = 0; i < count && !r.error; i++)
                read_gstate(&r, &f->gstate[base + i]);
            break;
        case 4: case 5: case 6: case 7: case 8:
            base = f->nlist[type];
            f->list[type] = (PfbList *)grow(f->list[type], &f->nlist[type], count, sizeof(PfbList));
            for (i = 0; i < count && !r.error; i++)
                read_list(&r, type, &f->list[type][base + i]);
            break;
        case 10:
            base = f->ngset;
            f->gset = (PfbGSet *)grow(f->gset, &f->ngset, count, sizeof(PfbGSet));
            for (i = 0; i < count && !r.error; i++)
                read_gset(&r, &f->gset[base + i]);
            break;
        case 12:
            base = f->nnode;
            f->node = (PfbNode *)grow(f->node, &f->nnode, count, sizeof(PfbNode));
            for (i = 0; i < count && !r.error; i++)
                read_node(&r, &f->node[base + i]);
            break;
        case 14:
            base = f->nmorph;
            f->morph = (int **)realloc(f->morph, sizeof(int *) * (base + count));
            f->morph_len = (int *)realloc(f->morph_len, sizeof(int) * (base + count));
            f->nmorph += count;
            for (i = 0; i < count && !r.error; i++)
                f->morph[base + i] = rd_words(&r, &f->morph_len[base + i]);
            break;
        default:
            /* user data, light points, ...: not needed for drawing */
            break;
        }
        r.p = next;
    }
    for (i = 0; i < f->nmorph; i++)
        pfb_morph(f, i, NULL);
    free(buf);
    if (f->nnode == 0) {
        pfb_free(f);
        return NULL;
    }
    return f;
}

void pfb_free(PfbFile *f)
{
    int i, t;
    if (!f)
        return;
    for (i = 0; i < f->ntex; i++)
        free(f->tex[i].name);
    for (i = 0; i < f->nnode; i++) {
        free(f->node[i].name);
        free(f->node[i].children);
        free(f->node[i].gsets);
        free(f->node[i].times);
    }
    for (t = 4; t <= 8; t++) {
        for (i = 0; i < f->nlist[t]; i++) {
            free(f->list[t][i].f);
            free(f->list[t][i].i);
        }
        free(f->list[t]);
    }
    for (i = 0; i < f->nmorph; i++)
        free(f->morph[i]);
    free(f->morph);
    free(f->morph_len);
    free(f->mtl);
    free(f->tex);
    free(f->tenv);
    free(f->gstate);
    free(f->gset);
    free(f->node);
    free(f);
}
