/*
 * pf_scene.c - libpf of IRIS Performer 2.0: the scene graph calls missing
 * from the layer of Mekton (bounds, static transforms, partitions)
 */
#include "pf_internal.h"

#include <math.h>

/* the box of the geometry of a graph under a matrix */
static void bound_node(pfNode *n, pfMatrix m, pfVec3 lo, pfVec3 hi, int *any, int depth)
{
    pfMatrix mm;
    int i, j, k;
    if (!n || depth > 64)
        return;
    if (n->type == PFNODE_DCS || n->type == PFNODE_SCS) {
        pfMultMat(mm, n->mat, m);
        m = mm;
    }
    for (i = 0; i < n->ngsets; i++) {
        pfGeoSet *g = n->gsets[i];
        const float *co = (const float *)g->attr[PFGS_COORD3];
        int nv = g->nverts;
        if (!co)
            continue;
        if (nv <= 0) {
            /* the vertices the primitives read */
            nv = 0;
            switch (g->prim) {
            case PFGS_POINTS: nv = g->nprims; break;
            case PFGS_LINES: nv = 2 * g->nprims; break;
            case PFGS_TRIS: nv = 3 * g->nprims; break;
            case PFGS_QUADS: nv = 4 * g->nprims; break;
            default:
                if (g->lengths)
                    for (j = 0; j < g->nprims; j++)
                        nv += g->lengths[j];
            }
            if (g->ilist[PFGS_COORD3]) {
                int mx = 0;
                for (j = 0; j < nv; j++)
                    if (g->ilist[PFGS_COORD3][j] > mx)
                        mx = g->ilist[PFGS_COORD3][j];
                nv = mx + 1;
            }
        }
        for (j = 0; j < nv; j++) {
            pfVec3 p, q;
            pfCopyVec3(q, co + 3 * j);
            /* the geosets of a billboard are around their position */
            if (n->type == PFNODE_BBOARD && n->bbpos)
                pfAddVec3(q, q, n->bbpos[i]);
            pfXformPt3(p, q, m);
            for (k = 0; k < 3; k++) {
                if (!*any || p[k] < lo[k]) lo[k] = p[k];
                if (!*any || p[k] > hi[k]) hi[k] = p[k];
            }
            *any = 1;
        }
    }
    for (i = 0; i < n->nchildren; i++) {
        pfNode *c = n->children[i];
        if (c && c->bsphmode) {
            /* a sphere given by the application */
            pfVec3 p;
            pfXformPt3(p, c->bsph, m);
            for (k = 0; k < 3; k++) {
                if (!*any || p[k] - c->bsph[3] < lo[k]) lo[k] = p[k] - c->bsph[3];
                if (!*any || p[k] + c->bsph[3] > hi[k]) hi[k] = p[k] + c->bsph[3];
            }
            *any = 1;
        } else {
            bound_node(c, m, lo, hi, any, depth + 1);
        }
    }
}

int pfiNodeBound(pfNode *n, pfSphere *s)
{
    pfMatrix m;
    pfVec3 lo, hi;
    int any = 0;
    pfMakeIdentMat(m);
    pfSetVec3(lo, 0, 0, 0);
    pfSetVec3(hi, 0, 0, 0);
    bound_node(n, m, lo, hi, &any, 0);
    pfCombineVec3(s->center, 0.5f, lo, 0.5f, hi);
    s->radius = any ? pfDistancePt3(lo, hi) / 2 : 0;
    return any;
}

int pfGetNodeBSphere(pfNode *n, pfSphere *s)
{
    if (!n) {
        pfSetVec3(s->center, 0, 0, 0);
        s->radius = 0;
        return 0;
    }
    if (n->bsphmode) {
        pfCopyVec3(s->center, n->bsph);
        s->radius = n->bsph[3];
        return PFBOUND_STATIC;
    }
    pfiNodeBound(n, s);
    if (s->radius <= 0 && n->nchildren == 0 && n->ngsets == 0)
        s->radius = 0;
    return PFBOUND_DYNAMIC;
}

int pfNodeBSphere(pfNode *n, pfSphere *s, int mode)
{
    (void)mode;
    if (!n)
        return 0;
    if (!s) {
        n->bsphmode = 0;
        return 0;
    }
    pfCopyVec3(n->bsph, s->center);
    n->bsph[3] = s->radius;
    n->bsphmode = 1;
    return 1;
}

pfSCS *pfNewSCS(pfMatrix m)
{
    pfNode *n = pfNewDCS();
    pfDCSMat(n, m);
    n->type = PFNODE_SCS;
    return n;
}

pfPartition *pfNewPart(void) { return pfNewGroup(); }
void pfPartAttr(pfPartition *p, int which, void *attr) { (void)p; (void)which; (void)attr; }
void pfPartVal(pfPartition *p, int which, float val) { (void)p; (void)which; (void)val; }
void pfBuildPart(pfPartition *p) { (void)p; }
void pfUpdatePart(pfPartition *p) { (void)p; }

void pfGetGSetAttrLists(pfGeoSet *g, int attr, void **alist, unsigned short **ilist)
{
    if (attr < 0 || attr > 3) {
        *alist = NULL;
        *ilist = NULL;
        return;
    }
    *alist = g->attr[attr];
    *ilist = g->ilist[attr];
}

/* the static transformations (pfSCS) are applied to copies of the
 * coordinates and the normals of the geosets under them */
static void flatten_node(pfNode *n, pfMatrix m, int depth)
{
    pfMatrix mm, inv, nm;
    int i, j, ident;
    if (!n || depth > 64)
        return;
    if (n->type == PFNODE_DCS)
        return;
    if (n->type == PFNODE_SCS) {
        pfMultMat(mm, n->mat, m);
        m = mm;
    }
    ident = 1;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            if (m[i][j] != (i == j ? 1.0f : 0.0f))
                ident = 0;
    if (!ident) {
        pfInvertAffMat(inv, m);
        for (i = 0; i < 4; i++)
            for (j = 0; j < 4; j++)
                nm[i][j] = inv[j][i];
        for (i = 0; i < n->ngsets; i++) {
            pfGeoSet *g = n->gsets[i];
            int nv = g->nverts, k;
            float *co;
            if (!g->attr[PFGS_COORD3])
                continue;
            if (nv <= 0)
                continue;   /* the count of the vertices is unknown */
            co = (float *)malloc(sizeof(float) * 3 * nv);
            for (k = 0; k < nv; k++)
                pfXformPt3(co + 3 * k, (const float *)g->attr[PFGS_COORD3] + 3 * k, m);
            g->attr[PFGS_COORD3] = co;
            if (g->attr[PFGS_NORMAL3] && g->bind[PFGS_NORMAL3] == PFGS_PER_VERTEX && !g->ilist[PFGS_NORMAL3]) {
                float *no = (float *)malloc(sizeof(float) * 3 * nv);
                for (k = 0; k < nv; k++) {
                    pfXformVec3(no + 3 * k, (const float *)g->attr[PFGS_NORMAL3] + 3 * k, nm);
                    pfNormalizeVec3(no + 3 * k);
                }
                g->attr[PFGS_NORMAL3] = no;
            }
            if (g->tris) {
                free(g->tris);
                g->tris = NULL;
            }
            g->hascenter = 0;
        }
    }
    for (i = 0; i < n->nchildren; i++)
        flatten_node(n->children[i], m, depth + 1);
    if (n->type == PFNODE_SCS)
        pfMakeIdentMat(n->mat);
}

int pfFlatten(pfNode *n, int mode)
{
    pfMatrix m;
    (void)mode;
    pfMakeIdentMat(m);
    flatten_node(n, m, 0);
    return 0;
}
