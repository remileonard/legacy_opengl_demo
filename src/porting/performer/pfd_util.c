/*
 * pfd_util.c - libpfdu of IRIS Performer 2.0: the loaders chosen on the
 * extension of the files (.pfb from Mekton, .dwb from the F-18 demo, .obj
 * here), the builder modes and the optimizations of the scene graph,
 * which keep the graph as it is.
 */
#include "pf_internal.h"

#include <ctype.h>
#include <math.h>

/* the Designer's Workbench loader of the F-18 demo (src/f18/dwb.c) */
pfNode *pfdLoadFile_dwb(const char *file);
pfNode *pfdLoadFile_obj(const char *file);

static int bldr_modes[64];
static pfGeoState default_gs;

static const char *file_ext(const char *file)
{
    const char *dot = strrchr(file, '.'), *sl = strrchr(file, '/');
#ifdef _WIN32
    const char *bs = strrchr(file, '\\');
    if (bs && (!sl || bs > sl))
        sl = bs;
#endif
    return dot && (!sl || dot > sl) ? dot + 1 : "";
}

static void register_loaders(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    pfdRegisterLoader("dwb", pfdLoadFile_dwb);
    pfdRegisterLoader("obj", pfdLoadFile_obj);
}

/* appends a directory to the file search path */
static void add_path(const char *dir)
{
    const char *old = pfGetFilePath();
    char *p;
    size_t n = strlen(old) + strlen(dir) + 2;
    if (strstr(old, dir)) {
        /* already there: look for it as a whole entry */
        const char *q = old;
        size_t l = strlen(dir);
        while ((q = strstr(q, dir)) != NULL) {
            if ((q == old || q[-1] == ':') && (q[l] == ':' || q[l] == '\0'))
                return;
            q += l;
        }
    }
    p = (char *)malloc(n);
    sprintf(p, "%s%s%s", old, old[0] ? ":" : "", dir);
    pfFilePath(p);
    free(p);
}

/*
 * The loader of the extension of a file: 1 when there is one. The
 * directory of the file and the texture directories next to it are added
 * to the file path, where the loaders look for the textures.
 */
int pfdInitConverter(const char *file)
{
    const char *ext = file_ext(file);
    const char *sl = strrchr(file, '/');
    char dir[1024];
    register_loaders();
#ifdef _WIN32
    {
        const char *bs = strrchr(file, '\\');
        if (bs && (!sl || bs > sl))
            sl = bs;
    }
#endif
    if (sl && (size_t)(sl - file) < sizeof(dir) - 20) {
        sprintf(dir, "%.*s", (int)(sl - file), file);
        add_path(dir);
        strcat(dir, "/../textures");
        add_path(dir);
    }
    return pfdFindLoader(file) != NULL || (ext[0] && strcmp(ext, "pfb") == 0);
}

int pfdExitConverter(const char *file) { (void)file; return 1; }
void pfdConverterMode(const char *ext, int mode, float value) { (void)ext; (void)mode; (void)value; }

void pfdAddExtAlias(const char *ext, const char *alias)
{
    char name[64];
    pfNode *(*load)(const char *);
    register_loaders();
    sprintf(name, "x.%.50s", alias);
    load = pfdFindLoader(name);
    if (load)
        pfdRegisterLoader(ext, load);
}

int pfdStoreFile(pfNode *root, const char *file) { (void)root; (void)file; return 0; }
void pfdFreeFileLoaders(void) {}

void pfdBldrMode(int mode, int val) { if (mode >= 0 && mode < 64) bldr_modes[mode] = val; }
int pfdGetBldrMode(int mode) { return mode >= 0 && mode < 64 ? bldr_modes[mode] : 0; }

void *pfdGetDefaultGState(void)
{
    default_gs.lighting = 1;
    default_gs.texturing = 1;
    default_gs.lpoint = 0;
    default_gs.transparency = 0;
    default_gs.cullface = GL_BACK;
    return &default_gs;
}

pfNode *pfdCleanTree(pfNode *node, void *keep) { (void)keep; return node; }
pfNode *pfdFreezeTransforms(pfNode *node, void *keep) { (void)keep; return node; }
void pfdMakeShared(pfNode *node) { (void)node; }
void pfdCombineLayers(pfNode *node) { (void)node; }
void pfdCombineBillboards(pfNode *node, int size) { (void)node; (void)size; }

/* --- the scene geostate: the most common modes of the geostates ---------- */

typedef struct {
    int lit, unlit, tex, untex, cull, nocull;
    int nodes[16];
    int geosets, prims, verts, gstates;
} GraphCount;

static void count_node(pfNode *n, GraphCount *gc, int depth)
{
    int i;
    if (!n || depth > 64)
        return;
    if (n->type >= 0 && n->type < 16)
        gc->nodes[n->type]++;
    for (i = 0; i < n->ngsets; i++) {
        pfGeoSet *g = n->gsets[i];
        pfGeoState *gs = g->gstate;
        int k, nv = 0;
        gc->geosets++;
        gc->prims += g->nprims;
        switch (g->prim) {
        case PFGS_POINTS: nv = g->nprims; break;
        case PFGS_LINES: nv = 2 * g->nprims; break;
        case PFGS_TRIS: nv = 3 * g->nprims; break;
        case PFGS_QUADS: nv = 4 * g->nprims; break;
        default:
            if (g->lengths)
                for (k = 0; k < g->nprims; k++)
                    nv += g->lengths[k];
        }
        gc->verts += nv;
        if (gs) {
            gc->gstates++;
            if (gs->lighting > 0) gc->lit++;
            else if (gs->lighting == 0) gc->unlit++;
            if (gs->texturing > 0) gc->tex++;
            else if (gs->texturing == 0) gc->untex++;
            if (gs->cullface && gs->cullface != ~0U) gc->cull++;
            else if (gs->cullface == 0) gc->nocull++;
        }
    }
    for (i = 0; i < n->nchildren; i++)
        count_node(n->children[i], gc, depth + 1);
}

void pfdMakeSharedScene(pfScene *scene)
{
    GraphCount gc;
    pfGeoState *gs = pfNewGState(NULL);
    memset(&gc, 0, sizeof(gc));
    count_node(scene, &gc, 0);
    gs->lighting = gc.lit >= gc.unlit;
    gs->texturing = gc.tex >= gc.untex;
    gs->cullface = gc.cull > gc.nocull ? GL_BACK : 0;
    pfSceneGState(scene, gs);
}

void pfdPrintSceneGraphStats(pfNode *node, double elapsed)
{
    static const char *names[] = {
        "", "pfGroup", "pfScene", "pfDCS", "pfSCS", "pfSwitch", "pfSequence",
        "pfLOD", "pfLayer", "pfGeode", "pfMorph", "pfBillboard", "pfLightSource"
    };
    GraphCount gc;
    int i;
    memset(&gc, 0, sizeof(gc));
    count_node(node, &gc, 0);
    pfNotify(PFNFY_INFO, PFNFY_MORE, "  Scene graph built in %.3f seconds", elapsed);
    for (i = 1; i <= PFNODE_LSOURCE; i++)
        if (gc.nodes[i])
            pfNotify(PFNFY_INFO, PFNFY_MORE, "    %-14s %8d", names[i], gc.nodes[i]);
    pfNotify(PFNFY_INFO, PFNFY_MORE, "    %-14s %8d", "pfGeoSet", gc.geosets);
    pfNotify(PFNFY_INFO, PFNFY_MORE, "    %-14s %8d", "pfGeoState", gc.gstates);
    pfNotify(PFNFY_INFO, PFNFY_MORE, "    %-14s %8d", "primitives", gc.prims);
    pfNotify(PFNFY_INFO, PFNFY_MORE, "    %-14s %8d", "vertices", gc.verts);
}

pfFont *pfdLoadFont_type1(const char *name, int type)
{
    (void)name;
    (void)type;
    return NULL;
}

/* --- Wavefront .obj ------------------------------------------------------------- */

typedef struct {
    char name[64];
    pfMaterial *mtl;
    pfTexture *tex;
    float alpha;
} ObjMtl;

typedef struct {
    int mtl;
    int n, max;
    int *v;                 /* triples: position, texcoord, normal (-1: none) */
} ObjGroup;

static char *skip_ws(char *p)
{
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

static void chomp(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = '\0';
}

static int load_mtl(const char *dir, const char *file, ObjMtl **mtls, int *nmtl)
{
    char path[1200], line[1024];
    FILE *fp;
    ObjMtl *m = NULL;
    sprintf(path, "%s%s", dir, file);
    if ((fp = fopen(path, "r")) == NULL && pfFindFile(file, path, R_OK))
        fp = fopen(path, "r");
    if (!fp)
        return 0;
    while (fgets(line, sizeof(line), fp)) {
        char *p = skip_ws(line);
        chomp(p);
        if (strncmp(p, "newmtl", 6) == 0) {
            *mtls = (ObjMtl *)realloc(*mtls, sizeof(ObjMtl) * (*nmtl + 1));
            m = &(*mtls)[(*nmtl)++];
            memset(m, 0, sizeof(*m));
            strncpy(m->name, skip_ws(p + 6), sizeof(m->name) - 1);
            m->mtl = pfNewMtl(NULL);
            m->alpha = 1;
        } else if (!m) {
            continue;
        } else if (p[0] == 'K' && (p[1] == 'a' || p[1] == 'd' || p[1] == 's' || p[1] == 'e')) {
            float r = 0, g = 0, b = 0;
            float *c = p[1] == 'a' ? m->mtl->ambient : p[1] == 'd' ? m->mtl->diffuse :
                       p[1] == 's' ? m->mtl->specular : m->mtl->emission;
            if (sscanf(p + 2, "%f %f %f", &r, &g, &b) == 3)
                pfSetVec3(c, r, g, b);
        } else if (p[0] == 'N' && p[1] == 's') {
            float ns = 0;
            sscanf(p + 2, "%f", &ns);
            /* the .mtl of the samples use 0..1000 */
            m->mtl->shininess = ns > 128 ? 128 : ns;
        } else if (p[0] == 'd' && (p[1] == ' ' || p[1] == '\t')) {
            sscanf(p + 1, "%f", &m->alpha);
        } else if (strncmp(p, "map_Kd", 6) == 0) {
            pfTexture *t = pfNewTex(NULL);
            if (pfLoadTexFile(t, skip_ws(p + 6)))
                m->tex = t;
        }
    }
    fclose(fp);
    return 1;
}

static int obj_index(int i, int n)
{
    return i > 0 ? i - 1 : i < 0 ? n + i : -1;
}

/*
 * A Wavefront .obj file: one geode, a geoset of triangles per material.
 * The files are made with Y up: they are turned to Performer's Z up.
 */
pfNode *pfdLoadFile_obj(const char *file)
{
    char path[1024], dir[1024], line[4096];
    FILE *fp;
    float *pos = NULL, *nrm = NULL, *tex = NULL;
    int npos = 0, nnrm = 0, ntex = 0, maxpos = 0, maxnrm = 0, maxtex = 0;
    ObjMtl *mtls = NULL;
    int nmtl = 0, cur = 0, i, k;
    ObjGroup *groups = NULL;
    int ngroups = 0;
    pfGeode *geode;
    const char *sl;

    if (!pfFindFile(file, path, R_OK) || (fp = fopen(path, "r")) == NULL) {
        pfNotify(PFNFY_WARN, PFNFY_RESOURCE, "pfdLoadFile_obj: cannot open %s", file);
        return NULL;
    }
    sl = strrchr(path, '/');
    if (sl)
        sprintf(dir, "%.*s/", (int)(sl - path), path);
    else
        dir[0] = '\0';

    while (fgets(line, sizeof(line), fp)) {
        char *p = skip_ws(line);
        chomp(p);
        if (p[0] == 'v' && (p[1] == ' ' || p[1] == '\t')) {
            float x = 0, y = 0, z = 0;
            sscanf(p + 1, "%f %f %f", &x, &y, &z);
            if (npos == maxpos) {
                maxpos = maxpos ? 2 * maxpos : 1024;
                pos = (float *)realloc(pos, sizeof(float) * 3 * maxpos);
            }
            /* Y up to Z up */
            pos[3 * npos] = x; pos[3 * npos + 1] = -z; pos[3 * npos + 2] = y;
            npos++;
        } else if (p[0] == 'v' && p[1] == 'n') {
            float x = 0, y = 0, z = 0;
            sscanf(p + 2, "%f %f %f", &x, &y, &z);
            if (nnrm == maxnrm) {
                maxnrm = maxnrm ? 2 * maxnrm : 1024;
                nrm = (float *)realloc(nrm, sizeof(float) * 3 * maxnrm);
            }
            nrm[3 * nnrm] = x; nrm[3 * nnrm + 1] = -z; nrm[3 * nnrm + 2] = y;
            nnrm++;
        } else if (p[0] == 'v' && p[1] == 't') {
            float u = 0, v = 0;
            sscanf(p + 2, "%f %f", &u, &v);
            if (ntex == maxtex) {
                maxtex = maxtex ? 2 * maxtex : 1024;
                tex = (float *)realloc(tex, sizeof(float) * 2 * maxtex);
            }
            tex[2 * ntex] = u; tex[2 * ntex + 1] = v;
            ntex++;
        } else if (strncmp(p, "mtllib", 6) == 0) {
            char *q = skip_ws(p + 6), *e;
            /* several names can follow */
            while (*q) {
                e = q;
                while (*e && *e != ' ' && *e != '\t')
                    e++;
                if (*e)
                    *e++ = '\0';
                load_mtl(dir, q, &mtls, &nmtl);
                q = skip_ws(e);
            }
        } else if (strncmp(p, "usemtl", 6) == 0) {
            const char *name = skip_ws(p + 6);
            cur = 0;
            for (i = 0; i < nmtl; i++)
                if (strcmp(mtls[i].name, name) == 0)
                    cur = i + 1;
        } else if (p[0] == 'f' && (p[1] == ' ' || p[1] == '\t')) {
            int fv[64][3], nfv = 0;
            ObjGroup *g = NULL;
            char *q = p + 1;
            while (*(q = skip_ws(q)) && nfv < 64) {
                int a = 0, b = 0, c = 0;
                char *e = q;
                a = (int)strtol(q, &e, 10);
                if (*e == '/') {
                    e++;
                    if (*e != '/')
                        b = (int)strtol(e, &e, 10);
                    if (*e == '/') {
                        e++;
                        c = (int)strtol(e, &e, 10);
                    }
                }
                fv[nfv][0] = obj_index(a, npos);
                fv[nfv][1] = obj_index(b, ntex);
                fv[nfv][2] = obj_index(c, nnrm);
                if (fv[nfv][0] >= 0 && fv[nfv][0] < npos)
                    nfv++;
                while (*e && *e != ' ' && *e != '\t')
                    e++;
                q = e;
            }
            if (nfv < 3)
                continue;
            for (i = 0; i < ngroups; i++)
                if (groups[i].mtl == cur)
                    g = &groups[i];
            if (!g) {
                groups = (ObjGroup *)realloc(groups, sizeof(ObjGroup) * (ngroups + 1));
                g = &groups[ngroups++];
                memset(g, 0, sizeof(*g));
                g->mtl = cur;
            }
            /* a fan of triangles */
            for (i = 2; i < nfv; i++) {
                int tri[3];
                tri[0] = 0; tri[1] = i - 1; tri[2] = i;
                if (g->n + 3 > g->max) {
                    g->max = g->max ? 2 * g->max : 768;
                    g->v = (int *)realloc(g->v, sizeof(int) * 3 * g->max);
                }
                for (k = 0; k < 3; k++) {
                    g->v[3 * g->n] = fv[tri[k]][0];
                    g->v[3 * g->n + 1] = fv[tri[k]][1];
                    g->v[3 * g->n + 2] = fv[tri[k]][2];
                    g->n++;
                }
            }
        }
    }
    fclose(fp);

    geode = pfNewGeode();
    geode->name = (char *)malloc(strlen(file) + 1);
    strcpy(geode->name, file);
    for (i = 0; i < ngroups; i++) {
        ObjGroup *g = &groups[i];
        pfGeoSet *gset = pfNewGSet(NULL);
        pfGeoState *gs = pfNewGState(NULL);
        float *co = (float *)malloc(sizeof(float) * 3 * g->n);
        float *no = (float *)malloc(sizeof(float) * 3 * g->n);
        float *tc = NULL;
        float *acc = NULL;
        int v, havetex = 0, havenrm = 1;
        ObjMtl *m = g->mtl ? &mtls[g->mtl - 1] : NULL;

        for (v = 0; v < g->n; v++) {
            if (g->v[3 * v + 1] >= 0 && g->v[3 * v + 1] < ntex)
                havetex = 1;
            if (g->v[3 * v + 2] < 0 || g->v[3 * v + 2] >= nnrm)
                havenrm = 0;
        }
        if (havetex)
            tc = (float *)calloc(2 * g->n, sizeof(float));
        if (!havenrm) {
            /* smooth normals: the face normals summed at each position */
            acc = (float *)calloc(3 * npos, sizeof(float));
            for (v = 0; v + 2 < g->n; v += 3) {
                pfVec3 e1, e2, fn;
                const float *a = pos + 3 * g->v[3 * v], *b = pos + 3 * g->v[3 * (v + 1)],
                            *c = pos + 3 * g->v[3 * (v + 2)];
                pfSubVec3(e1, b, a);
                pfSubVec3(e2, c, a);
                pfCrossVec3(fn, e1, e2);
                for (k = 0; k < 3; k++) {
                    float *s = acc + 3 * g->v[3 * (v + k)];
                    pfAddVec3(s, s, fn);
                }
            }
        }
        for (v = 0; v < g->n; v++) {
            memcpy(co + 3 * v, pos + 3 * g->v[3 * v], sizeof(pfVec3));
            if (havenrm)
                memcpy(no + 3 * v, nrm + 3 * g->v[3 * v + 2], sizeof(pfVec3));
            else
                memcpy(no + 3 * v, acc + 3 * g->v[3 * v], sizeof(pfVec3));
            pfNormalizeVec3(no + 3 * v);
            if (tc && g->v[3 * v + 1] >= 0 && g->v[3 * v + 1] < ntex)
                memcpy(tc + 2 * v, tex + 2 * g->v[3 * v + 1], sizeof(pfVec2));
        }
        free(acc);
        pfGSetPrimType(gset, PFGS_TRIS);
        pfGSetNumPrims(gset, g->n / 3);
        pfGSetAttr(gset, PFGS_COORD3, PFGS_PER_VERTEX, co, NULL);
        pfGSetAttr(gset, PFGS_NORMAL3, PFGS_PER_VERTEX, no, NULL);
        if (tc)
            pfGSetAttr(gset, PFGS_TEXCOORD2, PFGS_PER_VERTEX, tc, NULL);
        gset->nverts = g->n;
        gs->lighting = 1;
        gs->cullface = 0;
        gs->transparency = m && m->alpha < 1;
        gs->texturing = m && m->tex && tc;
        gs->texture = m && tc ? m->tex : NULL;
        if (m) {
            m->mtl->alpha = m->alpha;
            gs->frontmtl = m->mtl;
        } else {
            gs->frontmtl = pfNewMtl(NULL);
        }
        pfGSetGState(gset, gs);
        pfAddGSet(geode, gset);
        free(g->v);
    }
    free(groups);
    free(pos);
    free(nrm);
    free(tex);
    pfNotify(PFNFY_INFO, PFNFY_PRINT, "pfdLoadFile_obj: %s, %d vertices, %d materials",
             file, npos, nmtl);
    return geode;
}
