/*
 * dwb.c - loader of the Designer's Workbench files (.dwb) of the F-18 demo
 *
 * Reconstructed from pfdLoad_dwb, linked in the IRIX binary of the demo,
 * and from the files of the demo.  A file is a sequence of big-endian
 * records: a 16-bit opcode, a 16-bit length, then the data.
 *
 *  0x1771 header      +0x04 float version, +0x110 short units
 *  0x1772 group       +0x34 flags, bit 26: the children are decals on the
 *                     first one (layer)
 *  0x1773 LOD         +0x08 switch out, +0x0c switch in, +0x10 center
 *  0x1775 polygon     +0x00 color index, +0x04 number of vertices,
 *                     +0x10 texture (-1 none), +0x2c color 0xAABBGGRR,
 *                     +0x36 material (-1 none)
 *  0x177a, 0x177c     vertices of the polygon, 44 bytes each: color
 *                     index, x y z, normal, s t, 2 floats
 *  0x1798 name        of the record before it
 *  0x179c file name   of the texture before it
 *  0x17ca, 0x17cb     push and pop: the records between them are the
 *                     children of the record before the push
 *  0x17ea material    +0x04 diffuse, +0x10 ambient, +0x1c shininess,
 *                     +0x20 specular, +0x2c emission, +0x38 alpha
 *  0x17eb texture     +0x00 width, +0x02 height, +0x04 components
 *
 * The other records (color table, eyepoints, lights, texture mappings,
 * texts of the cockpit) do not change the geometry and are skipped.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dwb.h"

#define OP_HEADER 0x1771
#define OP_GROUP 0x1772
#define OP_LOD 0x1773
#define OP_POLYGON 0x1775
#define OP_VERTICES 0x177a
#define OP_VERTICES2 0x177c
#define OP_NAME 0x1798
#define OP_TEXNAME 0x179c
#define OP_PUSH 0x17ca
#define OP_POP 0x17cb
#define OP_MATERIAL 0x17ea
#define OP_TEXTURE 0x17eb

#define GROUP_LAYER 0x04000000U
#define VERTEX_SIZE 44
#define MAX_DEPTH 256

static DwbStats stats;

const DwbStats *dwbGetStats(void) { return &stats; }

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

/* --- geometry being built -------------------------------------------------- */

/* the polygons of a geode that share a state go into one geoset */
typedef struct Batch {
    pfGeoSet *gset;
    int texture, material, transp;
    float *co, *no, *tc, *col;
    int *lengths;
    int nv, maxv, np, maxp;
    struct Batch *next;
} Batch;

typedef struct Level {
    pfNode *node;      /* where the children go */
    pfNode *geode;     /* the polygons of the node */
    Batch *batches;
    int layer;         /* each polygon in its own geode */
    int polygon;       /* inside a polygon: the vertices */
} Level;

typedef struct Loader {
    float scale;
    char dir[4096];
    /* textures and materials, in the order of the file */
    pfTexture **tex;
    int ntex;
    pfMaterial *mtl;
    int nmtl, maxmtl;
    pfGeoState **gstates;  /* [ (ntex + 1) * (nmtl + 1) * 2 ] */
    int gstate_tex, gstate_mtl;
    /* the polygon waiting for its vertices */
    Batch *poly_batch;
    unsigned int poly_color;
    int poly_nverts;
    Batch *all_batches;
} Loader;

static void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) {
        fprintf(stderr, "dwb: out of memory\n");
        exit(1);
    }
    return q;
}

static char *xstrdup(const char *s, size_t n)
{
    char *d = (char *)malloc(n + 1);
    memcpy(d, s, n);
    d[n] = 0;
    return d;
}

static void name_node(pfNode *n, const unsigned char *b, int len)
{
    int k = 0;
    while (k < len && b[k])
        k++;
    if (!n || k == 0)
        return;
    free(n->name);
    n->name = xstrdup((const char *)b, k);
}

/* --- textures -------------------------------------------------------------- */

static int file_exists(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return 0;
    fclose(fp);
    return 1;
}

/*
 * The images of the demo were encrypted for its distribution, and the demo
 * loads its textures with mypfLoadTexFile, which decrypts them
 * (ScrambleImageToImage).  An encrypted image is a verbatim SGI image whose
 * header has 4 in the word at 0x6c, after the colormap word; the run-length
 * encoded images and the other values are plain images.
 *
 * The pixels are XORed with a table of 1633 seeds: seed 0 is 0x67 and seed
 * i is 103 times seed i - 1, modulo 256.  The rows are read from the bottom
 * up, and the pixel i of a row uses the next seed of the table (going back
 * to the first one after the last) for the pixel i of the channels 0 and 2
 * and for the pixel width - 1 - i of the channels 1 and 3.  XOR is its own
 * inverse: the same function encrypts and decrypts.
 */
#define SCRAMBLE_SEEDS 0x661

static int is_encrypted(const char *path)
{
    unsigned char h[112];
    FILE *fp = fopen(path, "rb");
    int n;
    if (!fp)
        return 0;
    n = (int)fread(h, 1, sizeof(h), fp);
    fclose(fp);
    return n == (int)sizeof(h) && be16(h) == 474 && h[2] == 0 && be32(h + 0x6c) == 4;
}

/* decrypts the RGBA image of a texture loaded by pfLoadTexFile, where the
 * channels 0 and 1 of the images of one or two channels are luminance
 * (copied to red, green and blue) and alpha */
static void unscramble_texture(pfTexture *t)
{
    static unsigned char seed[SCRAMBLE_SEEDS];
    unsigned char *img = (unsigned char *)t->image;
    int x, y, i, n = 0, w = t->xsize, zs = t->comps;
    seed[0] = 0x67;
    for (i = 1; i < SCRAMBLE_SEEDS; i++)
        seed[i] = (unsigned char)(seed[i - 1] * 103);
    for (y = 0; y < t->ysize; y++) {
        unsigned char *row = img + (size_t)y * w * 4;
        for (x = 0; x < w; x++) {
            unsigned char s = seed[n];
            unsigned char *a = row + 4 * x, *b = row + 4 * (w - 1 - x);
            if (zs < 3) {
                a[0] ^= s;
                a[1] = a[2] = a[0];
                if (zs == 2)
                    b[3] ^= s;
            } else {
                a[0] ^= s;
                b[1] ^= s;
                a[2] ^= s;
                if (zs == 4)
                    b[3] ^= s;
            }
            n = (n + 1) % SCRAMBLE_SEEDS;
        }
    }
}

static const char *base_name(const char *path)
{
    const char *b = path, *p;
    for (p = path; *p; p++)
        if (*p == '/' || *p == '\\')
            b = p + 1;
    return b;
}

/* the file names are the ones of the modeler ("./encModels/enc/x.rgb",
 * or no extension): the image is searched by its name next to the model */
static pfTexture *load_texture(Loader *L, const char *name)
{
    static const char *sub[] = { "", "/../textures", "/textures" };
    static const char *ext[] = { "", ".rgb", ".rgba", ".int", ".inta", ".bw" };
    static pfTexture **cache;
    static char **cache_name;
    static int ncache;
    const char *base = base_name(name);
    char path[8192];
    pfTexture *t;
    int i, j, k;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 6; j++) {
            sprintf(path, "%s%s/%.1000s%s", L->dir, sub[i], base, ext[j]);
            if (!file_exists(path))
                continue;
            for (k = 0; k < ncache; k++)
                if (strcmp(cache_name[k], path) == 0) {
                    if (cache[k] && is_encrypted(path))
                        stats.textures_encrypted++;
                    return cache[k];
                }
            t = pfNewTex(NULL);
            if (!pfLoadTexFile(t, path)) {
                free(t);
                t = NULL;
            } else if (is_encrypted(path)) {
                unscramble_texture(t);
                stats.textures_encrypted++;
            }
            cache = (pfTexture **)xrealloc(cache, sizeof(*cache) * (ncache + 1));
            cache_name = (char **)xrealloc(cache_name, sizeof(*cache_name) * (ncache + 1));
            cache[ncache] = t;
            cache_name[ncache++] = xstrdup(path, strlen(path));
            return t;
        }
    return NULL;
}

/* --- states ---------------------------------------------------------------- */

static pfGeoState *get_gstate(Loader *L, int tex, int mtl, int transp)
{
    int nt = L->ntex + 1, nm = L->nmtl + 1, idx;
    pfGeoState *gs;
    if (tex < 0 || tex >= L->ntex || !L->tex[tex])
        tex = -1;
    if (mtl < 0 || mtl >= L->nmtl)
        mtl = -1;
    /* the tables grow with the textures and materials read so far */
    if (L->gstate_tex != nt || L->gstate_mtl != nm) {
        pfGeoState **old = L->gstates;
        int ot = L->gstate_tex, om = L->gstate_mtl, a, b, c;
        L->gstates = (pfGeoState **)calloc((size_t)nt * nm * 2, sizeof(pfGeoState *));
        for (a = 0; a < ot; a++)
            for (b = 0; b < om; b++)
                for (c = 0; c < 2; c++)
                    L->gstates[(a * nm + b) * 2 + c] = old[(a * om + b) * 2 + c];
        free(old);
        L->gstate_tex = nt;
        L->gstate_mtl = nm;
    }
    idx = ((tex + 1) * nm + (mtl + 1)) * 2 + (transp ? 1 : 0);
    if ((gs = L->gstates[idx]) != NULL)
        return gs;
    gs = (pfGeoState *)calloc(1, sizeof(pfGeoState));
    gs->lighting = 1;
    gs->texturing = tex >= 0;
    gs->texture = tex >= 0 ? L->tex[tex] : NULL;
    gs->lpoint = 0;
    gs->transparency = transp;
    gs->cullface = 0;
    if (tex >= 0 && (gs->texture->comps == 2 || gs->texture->comps == 4)) {
        gs->alphafunc = 0x0204; /* GL_GREATER */
        gs->alpharef = 0.0f;
    }
    if (mtl >= 0) {
        gs->frontmtl = &L->mtl[mtl];
    } else {
        /* the default material of Performer, colored by the polygons */
        static pfMaterial def;
        if (def.colormode == 0) {
            def.alpha = 1;
            pfSetVec3(def.ambient, 0.2f, 0.2f, 0.2f);
            pfSetVec3(def.diffuse, 0.8f, 0.8f, 0.8f);
            def.colormode = 1;
        }
        gs->frontmtl = &def;
    }
    L->gstates[idx] = gs;
    return gs;
}

static int batch_transparent(Loader *L, unsigned int color, int tex, int mtl)
{
    if ((color >> 24) < 255)
        return 1;
    if (mtl >= 0 && mtl < L->nmtl && L->mtl[mtl].alpha < 0.95f)
        return 1;
    if (tex >= 0 && tex < L->ntex && L->tex[tex] &&
        (L->tex[tex]->comps == 2 || L->tex[tex]->comps == 4))
        return 1;
    return 0;
}

static Batch *get_batch(Loader *L, Level *lv, int tex, int mtl, int transp)
{
    Batch *b;
    if (!lv->layer)
        for (b = lv->batches; b; b = b->next)
            if (b->texture == tex && b->material == mtl && b->transp == transp)
                return b;
    if (!lv->geode || lv->layer) {
        lv->geode = pfNewGeode();
        pfAddChild(lv->node, lv->geode);
    }
    b = (Batch *)calloc(1, sizeof(Batch));
    b->texture = tex;
    b->material = mtl;
    b->transp = transp;
    b->gset = pfNewGSet(NULL);
    pfGSetPrimType(b->gset, PFGS_POLYS);
    pfGSetGState(b->gset, get_gstate(L, tex, mtl, transp));
    pfAddGSet(lv->geode, b->gset);
    b->next = lv->batches;
    lv->batches = b;
    /* the list of all the batches, to finish the geosets at the end */
    {
        Batch *link = (Batch *)calloc(1, sizeof(Batch));
        link->gset = (pfGeoSet *)(void *)b; /* reused as a pointer to the batch */
        link->next = L->all_batches;
        L->all_batches = link;
    }
    return b;
}

static void add_vertices(Loader *L, const unsigned char *b, int len)
{
    Batch *bt = L->poly_batch;
    int n = len / VERTEX_SIZE, i, k;
    float rgba[4];
    if (!bt || n <= 0)
        return;
    L->poly_batch = NULL;
    if (L->poly_nverts > 0 && L->poly_nverts < n)
        n = L->poly_nverts;
    if (n < 3)
        return;
    rgba[0] = (L->poly_color & 0xff) / 255.0f;
    rgba[1] = ((L->poly_color >> 8) & 0xff) / 255.0f;
    rgba[2] = ((L->poly_color >> 16) & 0xff) / 255.0f;
    rgba[3] = ((L->poly_color >> 24) & 0xff) / 255.0f;
    if (bt->nv + n > bt->maxv) {
        bt->maxv = (bt->nv + n) * 2;
        bt->co = (float *)xrealloc(bt->co, sizeof(float) * 3 * bt->maxv);
        bt->no = (float *)xrealloc(bt->no, sizeof(float) * 3 * bt->maxv);
        bt->tc = (float *)xrealloc(bt->tc, sizeof(float) * 2 * bt->maxv);
        bt->col = (float *)xrealloc(bt->col, sizeof(float) * 4 * bt->maxv);
    }
    if (bt->np + 1 > bt->maxp) {
        bt->maxp = (bt->np + 1) * 2;
        bt->lengths = (int *)xrealloc(bt->lengths, sizeof(int) * bt->maxp);
    }
    for (i = 0; i < n; i++) {
        const unsigned char *v = b + i * VERTEX_SIZE;
        int o = bt->nv + i;
        for (k = 0; k < 3; k++) {
            bt->co[3 * o + k] = bef(v + 4 + 4 * k) * L->scale;
            bt->no[3 * o + k] = bef(v + 16 + 4 * k);
        }
        bt->tc[2 * o] = bef(v + 28);
        bt->tc[2 * o + 1] = bef(v + 32);
        memcpy(bt->col + 4 * o, rgba, sizeof(rgba));
    }
    bt->nv += n;
    bt->lengths[bt->np++] = n;
    stats.polygons++;
    stats.vertices += n;
}

static void finish_batches(Loader *L)
{
    Batch *link, *next;
    for (link = L->all_batches; link; link = next) {
        Batch *b = (Batch *)(void *)link->gset;
        pfGeoSet *g = b->gset;
        next = link->next;
        free(link);
        pfGSetNumPrims(g, b->np);
        pfGSetPrimLengths(g, b->lengths);
        pfGSetAttr(g, PFGS_COORD3, PFGS_PER_VERTEX, b->co, NULL);
        pfGSetAttr(g, PFGS_NORMAL3, PFGS_PER_VERTEX, b->no, NULL);
        pfGSetAttr(g, PFGS_COLOR4, PFGS_PER_VERTEX, b->col, NULL);
        if (b->texture >= 0)
            pfGSetAttr(g, PFGS_TEXCOORD2, PFGS_PER_VERTEX, b->tc, NULL);
        g->nverts = b->nv;
        /* the arrays stay alive with the geoset */
    }
    L->all_batches = NULL;
}

/* --- records --------------------------------------------------------------- */

static float units_scale(int units)
{
    switch (units) {
    case 1: return 0.3048f;     /* feet */
    case 2: return 0.0254f;     /* inches */
    case 3: return 0.01f;       /* centimeters */
    case 4: return 1852.0f;     /* nautical miles */
    case 5: return 1000.0f;     /* kilometers */
    case 6: return 1609.344f;   /* miles */
    default: return 1.0f;       /* meters */
    }
}

static void read_material(Loader *L, const unsigned char *b, int len)
{
    pfMaterial *m;
    if (len < 0x3c)
        return;
    if (L->nmtl == L->maxmtl) {
        /* the gstates point to the materials: the table must not move */
        if (L->nmtl)
            return;
        L->maxmtl = 1024;
        L->mtl = (pfMaterial *)calloc(L->maxmtl, sizeof(pfMaterial));
    }
    m = &L->mtl[L->nmtl++];
    pfSetVec3(m->diffuse, bef(b + 0x04), bef(b + 0x08), bef(b + 0x0c));
    pfSetVec3(m->ambient, bef(b + 0x10), bef(b + 0x14), bef(b + 0x18));
    m->shininess = bef(b + 0x1c);
    pfSetVec3(m->specular, bef(b + 0x20), bef(b + 0x24), bef(b + 0x28));
    pfSetVec3(m->emission, bef(b + 0x2c), bef(b + 0x30), bef(b + 0x34));
    m->alpha = bef(b + 0x38);
    if (m->alpha <= 0 || m->alpha > 1)
        m->alpha = 1;
    /* the colors of the polygons and of the vertices are kept */
    m->colormode = 1;
    stats.materials++;
}

static void read_dir(Loader *L, const char *file)
{
    const char *b = base_name(file);
    size_t n = (size_t)(b - file);
    if (n == 0) {
        strcpy(L->dir, ".");
    } else {
        if (n > sizeof(L->dir) - 1)
            n = sizeof(L->dir) - 1;
        memcpy(L->dir, file, n - 1);
        L->dir[n - 1] = 0;
        if (n == 1)
            strcpy(L->dir, "/");
    }
}

pfNode *pfdLoadFile_dwb(const char *file)
{
    Loader L;
    Level stack[MAX_DEPTH];
    unsigned char *data;
    pfNode *root, *last = NULL;
    Level last_level;
    long size, p;
    int depth = 0, last_is_poly = 0, last_tex = -1;
    FILE *fp = fopen(file, "rb");

    memset(&stats, 0, sizeof(stats));
    if (!fp) {
        fprintf(stderr, "dwb: cannot open %s\n", file);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    data = (unsigned char *)malloc(size > 0 ? size : 1);
    if (size < 8 || fread(data, 1, size, fp) != (size_t)size || be16(data) != OP_HEADER) {
        fprintf(stderr, "dwb: %s is not a Designer's Workbench file\n", file);
        fclose(fp);
        free(data);
        return NULL;
    }
    fclose(fp);

    memset(&L, 0, sizeof(L));
    L.scale = 1;
    read_dir(&L, file);
    root = pfNewGroup();
    name_node(root, (const unsigned char *)base_name(file), (int)strlen(base_name(file)));
    memset(&stack[0], 0, sizeof(Level));
    stack[0].node = root;
    memset(&last_level, 0, sizeof(last_level));

    for (p = 0; p + 4 <= size;) {
        int op = be16(data + p), len = be16(data + p + 2);
        const unsigned char *b = data + p + 4;
        Level *lv = &stack[depth];
        if (p + 4 + len > size)
            break;
        p += 4 + len;
        switch (op) {
        case OP_HEADER:
            if (len >= 8)
                stats.version = bef(b + 4);
            if (len >= 0x112)
                L.scale = units_scale(sbe16(b + 0x110));
            break;
        case OP_GROUP: {
            unsigned int flags = len >= 0x38 ? be32(b + 0x34) : 0;
            pfNode *n;
            if (flags & GROUP_LAYER) {
                n = pfNewLayer();
                stats.layers++;
            } else {
                n = pfNewGroup();
            }
            stats.groups++;
            pfAddChild(lv->node, n);
            last = n;
            memset(&last_level, 0, sizeof(last_level));
            last_level.node = n;
            last_level.layer = (flags & GROUP_LAYER) != 0;
            last_is_poly = 0;
            break;
        }
        case OP_LOD: {
            pfNode *lod = pfNewLOD(), *g = pfNewGroup();
            pfVec3 c;
            if (len >= 0x1c) {
                pfLODRange(lod, 0, bef(b + 0x0c) * L.scale);
                pfLODRange(lod, 1, bef(b + 0x08) * L.scale);
                pfSetVec3(c, bef(b + 0x10) * L.scale, bef(b + 0x14) * L.scale, bef(b + 0x18) * L.scale);
                pfLODCenter(lod, c);
            }
            pfAddChild(lod, g);
            pfAddChild(lv->node, lod);
            stats.lods++;
            last = lod;
            memset(&last_level, 0, sizeof(last_level));
            last_level.node = g;
            last_is_poly = 0;
            break;
        }
        case OP_POLYGON: {
            int tex, mtl, transp;
            unsigned int color;
            if (len < 0x38)
                break;
            tex = sbe16(b + 0x10);
            mtl = sbe16(b + 0x36);
            color = be32(b + 0x2c);
            transp = batch_transparent(&L, color, tex, mtl);
            {
                /* a polygon inside a polygon (a subface) goes with its parent */
                Level *target = lv;
                int d = depth;
                while (d > 0 && stack[d].polygon)
                    target = &stack[--d];
                L.poly_batch = get_batch(&L, target, tex >= 0 && tex < L.ntex && L.tex[tex] ? tex : -1,
                                         mtl >= 0 && mtl < L.nmtl ? mtl : -1, transp);
            }
            L.poly_color = color;
            L.poly_nverts = sbe16(b + 4);
            last = NULL;
            last_is_poly = 1;
            break;
        }
        case OP_VERTICES:
        case OP_VERTICES2:
            add_vertices(&L, b, len);
            break;
        case OP_NAME:
            if (last)
                name_node(last, b, len);
            break;
        case OP_MATERIAL:
            read_material(&L, b, len);
            break;
        case OP_TEXTURE:
            L.tex = (pfTexture **)xrealloc(L.tex, sizeof(pfTexture *) * (L.ntex + 1));
            L.tex[L.ntex] = NULL;
            last_tex = L.ntex++;
            stats.textures++;
            break;
        case OP_TEXNAME:
            if (last_tex >= 0 && !L.tex[last_tex]) {
                char *name = xstrdup((const char *)b, len);
                L.tex[last_tex] = load_texture(&L, name);
                if (L.tex[last_tex])
                    stats.textures_loaded++;
                free(name);
            }
            last_tex = -1;
            break;
        case OP_PUSH:
            if (depth + 1 >= MAX_DEPTH)
                break;
            depth++;
            if (last_is_poly) {
                /* the vertices and the subfaces of the polygon */
                stack[depth] = stack[depth - 1];
                stack[depth].polygon = 1;
            } else if (last_level.node) {
                stack[depth] = last_level;
            } else {
                /* the first push of the file: the root */
                stack[depth] = stack[depth - 1];
                stack[depth].polygon = 0;
            }
            memset(&last_level, 0, sizeof(last_level));
            last = NULL;
            last_is_poly = 0;
            break;
        case OP_POP:
            if (depth > 0) {
                /* the batches of a level stay with its geode */
                if (!stack[depth].polygon && depth > 0 && stack[depth].node == stack[depth - 1].node)
                    stack[depth - 1].batches = stack[depth].batches, stack[depth - 1].geode = stack[depth].geode;
                depth--;
            }
            memset(&last_level, 0, sizeof(last_level));
            last = NULL;
            last_is_poly = 0;
            break;
        default:
            break;
        }
    }
    finish_batches(&L);
    free(data);
    free(L.tex);
    return root;
}
