/*
 * yaodl.c - reader of the YAODL object files of powerflip (.ydl, .defs)
 *
 * YAODL ("Yet Another Object Description Language") is the SGI text format
 * of powerflip, the successor of flip.  A file is a list of objects:
 *
 *     (group (lines (vertices x y z ...), (vertices ...),), : colors 1 1 1),
 *
 *     #YAODL
 *     v = (vertices @float<NUL><size><data>, : (normals @float...,),),
 *     i = @indices<NUL><size><data>,
 *     (indexpolygons v, i,),
 *
 * - "(class item item ...)" is an object, items are separated by spaces or
 *   commas: numbers, other objects, names of definitions, binary blocks;
 * - "name = object," defines a name usable later in place of the object;
 * - ": object" associates an object (normals, colors) to the previous one,
 *   the parentheses may be omitted ("": colors 1 1 1");
 * - "@type<NUL>" followed by a big endian 32 bit size in bytes and the data
 *   is a binary block of big endian floats (float) or ints (indices, int);
 * - comments are C comments and lines starting with '#'.
 *
 * The geometry is turned into the quads of flip (4 normals and vertices per
 * polygon, 8 floats per point): triangles repeat their last vertex, larger
 * polygons are cut into a fan, and the segments of lines become degenerate
 * quads (a, b, b, a) drawn with the edges of flip.
 */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flip.h"

typedef struct ynode {
    char cls[32];
    float *f;               /* numbers, as floats */
    int nf, maxf;
    struct ynode **kids;    /* sub-objects and references */
    int nkids, maxkids;
    struct ynode **assoc;   /* associated objects (": normals") */
    int nassoc, maxassoc;
} ynode;

typedef struct {
    char name[64];
    ynode *node;
} ydef;

typedef struct {
    const unsigned char *s;
    long len, pos;
    ydef *defs;
    int ndefs, maxdefs;
    const char *file;
    int error;
} yparser;

/* output: flip points, 8 floats each (normal + pad, vertex + pad) */
typedef struct {
    float *data;
    int npoints, maxpoints;
    int polygons;           /* some polygons, not only lines */
} yout;

static void *grow(void *p, int *max, int n, size_t size)
{
    if (n < *max)
        return p;
    *max = *max ? *max * 2 : 16;
    if (*max <= n)
        *max = n + 1;
    p = realloc(p, *max * size);
    if (!p) {
        fprintf(stderr, "yaodl: out of memory\n");
        exit(1);
    }
    return p;
}

static ynode *new_node(const char *cls)
{
    ynode *n = (ynode *)calloc(1, sizeof(ynode));
    strncpy(n->cls, cls, sizeof(n->cls) - 1);
    return n;
}

static void add_float(ynode *n, float f)
{
    n->f = (float *)grow(n->f, &n->maxf, n->nf, sizeof(float));
    n->f[n->nf++] = f;
}

static void add_kid(ynode *n, ynode *k)
{
    n->kids = (ynode **)grow(n->kids, &n->maxkids, n->nkids, sizeof(ynode *));
    n->kids[n->nkids++] = k;
}

static void add_assoc(ynode *n, ynode *a)
{
    n->assoc = (ynode **)grow(n->assoc, &n->maxassoc, n->nassoc, sizeof(ynode *));
    n->assoc[n->nassoc++] = a;
}

/* --- lexer ------------------------------------------------------------------ */

static void skip_space(yparser *p)
{
    while (p->pos < p->len) {
        int c = p->s[p->pos];
        if (c == '#') {
            while (p->pos < p->len && p->s[p->pos] != '\n')
                p->pos++;
        } else if (c == '/' && p->pos + 1 < p->len && p->s[p->pos + 1] == '*') {
            p->pos += 2;
            while (p->pos + 1 < p->len && !(p->s[p->pos] == '*' && p->s[p->pos + 1] == '/'))
                p->pos++;
            p->pos += 2;
        } else if (isspace(c) || c == ',') {
            p->pos++;
        } else {
            break;
        }
    }
}

static int peek(yparser *p)
{
    skip_space(p);
    return p->pos < p->len ? p->s[p->pos] : -1;
}

static int is_ident_char(int c)
{
    return isalnum(c) || c == '_';
}

static void read_ident(yparser *p, char *buf, int size)
{
    int n = 0;
    while (p->pos < p->len && is_ident_char(p->s[p->pos])) {
        if (n < size - 1)
            buf[n++] = (char)p->s[p->pos];
        p->pos++;
    }
    buf[n] = 0;
}

static int is_number_start(yparser *p)
{
    int c = p->s[p->pos];
    if (isdigit(c) || c == '.')
        return 1;
    if ((c == '-' || c == '+') && p->pos + 1 < p->len)
        return isdigit(p->s[p->pos + 1]) || p->s[p->pos + 1] == '.';
    return 0;
}

static float read_number(yparser *p)
{
    char buf[64];
    int n = 0;
    while (p->pos < p->len && n < 63) {
        int c = p->s[p->pos];
        if (!(isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E'))
            break;
        buf[n++] = (char)c;
        p->pos++;
    }
    buf[n] = 0;
    return (float)atof(buf);
}

static unsigned int be32(const unsigned char *b)
{
    return ((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16) | ((unsigned int)b[2] << 8) | b[3];
}

/* "@type<NUL><size><data>": the values are added to n */
static void read_binary(yparser *p, ynode *n)
{
    char type[32];
    int k = 0, i, count;
    unsigned int size;
    p->pos++; /* '@' */
    while (p->pos < p->len && p->s[p->pos] != 0) {
        if (!isspace(p->s[p->pos]) && k < 31)
            type[k++] = (char)p->s[p->pos];
        p->pos++;
    }
    type[k] = 0;
    p->pos++; /* NUL */
    if (p->pos + 4 > p->len) {
        p->error = 1;
        return;
    }
    size = be32(p->s + p->pos);
    p->pos += 4;
    if (size > (unsigned int)(p->len - p->pos)) {
        fprintf(stderr, "yaodl: %s: truncated binary block @%s\n", p->file, type);
        p->error = 1;
        return;
    }
    count = (int)(size / 4);
    for (i = 0; i < count; i++) {
        unsigned int u = be32(p->s + p->pos + 4 * i);
        if (strcmp(type, "float") == 0) {
            float f;
            memcpy(&f, &u, 4);
            add_float(n, f);
        } else {
            add_float(n, (float)(int)u);
        }
    }
    p->pos += size;
}

/* --- parser ------------------------------------------------------------------ */

static ynode *find_def(yparser *p, const char *name)
{
    int i;
    for (i = p->ndefs - 1; i >= 0; i--)
        if (strcmp(p->defs[i].name, name) == 0)
            return p->defs[i].node;
    return NULL;
}

static ynode *parse_object(yparser *p);

/* the items of an object until ')' (or until ',' / ')' for a bare object) */
static void parse_items(yparser *p, ynode *n, int bare)
{
    ynode *last = NULL;
    for (;;) {
        int c;
        if (bare) {
            /* a bare object ends at the next comma or parenthesis */
            while (p->pos < p->len && (p->s[p->pos] == ' ' || p->s[p->pos] == '\t'))
                p->pos++;
            if (p->pos >= p->len || p->s[p->pos] == ',' || p->s[p->pos] == ')' || p->s[p->pos] == '\n'
                || p->s[p->pos] == '/')
                return;
            c = p->s[p->pos];
        } else {
            c = peek(p);
            if (c < 0) {
                p->error = 1;
                return;
            }
            if (c == ')') {
                p->pos++;
                return;
            }
        }
        if (c == '(') {
            last = parse_object(p);
            if (last)
                add_kid(n, last);
        } else if (c == '@') {
            read_binary(p, n);
        } else if (c == ':') {
            ynode *a;
            p->pos++;
            a = parse_object(p);
            if (a)
                add_assoc(last ? last : n, a);
        } else if (is_number_start(p)) {
            add_float(n, read_number(p));
        } else if (is_ident_char(c)) {
            char name[64];
            ynode *d;
            read_ident(p, name, sizeof(name));
            if ((d = find_def(p, name)) != NULL) {
                add_kid(n, d);
                last = d;
            } else {
                fprintf(stderr, "yaodl: %s: unknown name %s\n", p->file, name);
            }
        } else {
            p->pos++; /* unexpected character */
        }
        if (p->error)
            return;
    }
}

/* "(class items)", "class items" (bare, after ':'), a definition name or a
   lone binary block */
static ynode *parse_object(yparser *p)
{
    char cls[32];
    ynode *n;
    int c = peek(p);
    if (c == '(') {
        p->pos++;
        peek(p);
        read_ident(p, cls, sizeof(cls));
        n = new_node(cls);
        parse_items(p, n, 0);
        return n;
    }
    if (c == '@') {
        n = new_node("");
        read_binary(p, n);
        return n;
    }
    if (c >= 0 && is_ident_char(c)) {
        ynode *d;
        read_ident(p, cls, sizeof(cls));
        if ((d = find_def(p, cls)) != NULL)
            return d;
        n = new_node(cls);
        parse_items(p, n, 1);
        return n;
    }
    if (c >= 0)
        p->pos++;
    return NULL;
}

/* the top level: definitions and objects */
static void parse_file(yparser *p, ynode *root)
{
    for (;;) {
        int c = peek(p);
        if (c < 0 || p->error)
            return;
        if (is_ident_char(c) && !is_number_start(p)) {
            /* "name = object" */
            long save = p->pos;
            char name[64];
            read_ident(p, name, sizeof(name));
            if (peek(p) == '=') {
                ynode *n;
                p->pos++;
                n = parse_object(p);
                if (n) {
                    p->defs = (ydef *)grow(p->defs, &p->maxdefs, p->ndefs, sizeof(ydef));
                    strncpy(p->defs[p->ndefs].name, name, sizeof(p->defs[0].name) - 1);
                    p->defs[p->ndefs].name[sizeof(p->defs[0].name) - 1] = 0;
                    p->defs[p->ndefs++].node = n;
                }
                continue;
            }
            p->pos = save;
        }
        if (c == ':') {
            /* association to the last top level object */
            ynode *a;
            p->pos++;
            a = parse_object(p);
            if (a && root->nkids)
                add_assoc(root->kids[root->nkids - 1], a);
            continue;
        }
        {
            ynode *n = parse_object(p);
            if (n)
                add_kid(root, n);
        }
    }
}

/* --- geometry -------------------------------------------------------------------- */

static ynode *find_assoc(ynode *n, const char *cls)
{
    int i;
    for (i = 0; i < n->nassoc; i++)
        if (strcmp(n->assoc[i]->cls, cls) == 0)
            return n->assoc[i];
    for (i = 0; i < n->nkids; i++)
        if (strcmp(n->kids[i]->cls, cls) == 0)
            return n->kids[i];
    return NULL;
}

static void put_point(yout *o, const float *n, const float *v)
{
    float *d;
    o->data = (float *)grow(o->data, &o->maxpoints, o->npoints, 8 * sizeof(float));
    d = o->data + 8 * o->npoints++;
    d[0] = n[0], d[1] = n[1], d[2] = n[2], d[3] = 0;
    d[4] = v[0], d[5] = v[1], d[6] = v[2], d[7] = 0;
}

static void normalize(float *n)
{
    float l = (float)sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (l > 0)
        n[0] /= l, n[1] /= l, n[2] /= l;
}

/* a polygon of nv vertices: v[i] points to xyz, nrm[i] to its normal or NULL */
static void put_polygon(yout *o, const float **v, const float **nrm, int nv)
{
    float fn[3] = { 0, 0, 0 };
    int i, k;
    if (nv < 3)
        return;
    /* the face normal (Newell), for the vertices without normal */
    for (i = 0; i < nv; i++) {
        const float *a = v[i], *b = v[(i + 1) % nv];
        fn[0] += (a[1] - b[1]) * (a[2] + b[2]);
        fn[1] += (a[2] - b[2]) * (a[0] + b[0]);
        fn[2] += (a[0] - b[0]) * (a[1] + b[1]);
    }
    normalize(fn);
    /* a fan of quads (0, k, k+1, k+2), the last one may be a triangle */
    for (k = 1; k < nv - 1; k += 2) {
        int idx[4];
        idx[0] = 0;
        idx[1] = k;
        idx[2] = k + 1;
        idx[3] = k + 2 < nv ? k + 2 : k + 1;
        for (i = 0; i < 4; i++)
            put_point(o, nrm && nrm[idx[i]] ? nrm[idx[i]] : fn, v[idx[i]]);
    }
    o->polygons = 1;
}

/* a segment of a line: a degenerate quad (a, b, b, a) */
static void put_segment(yout *o, const float *a, const float *b)
{
    float n[3];
    /* lines have no normal: point them away from the center for the lighting */
    n[0] = a[0] + b[0], n[1] = a[1] + b[1], n[2] = a[2] + b[2];
    if (n[0] == 0 && n[1] == 0 && n[2] == 0)
        n[2] = 1;
    normalize(n);
    put_point(o, n, a);
    put_point(o, n, b);
    put_point(o, n, b);
    put_point(o, n, a);
}

static int warned(const char *cls)
{
    static char seen[32][32];
    static int nseen;
    int i;
    for (i = 0; i < nseen; i++)
        if (strcmp(seen[i], cls) == 0)
            return 1;
    if (nseen < 32)
        strcpy(seen[nseen++], cls);
    return 0;
}

static void emit(yout *o, ynode *n, int depth)
{
    int i, j;
    if (depth > 64)
        return;
    if (strcmp(n->cls, "indexpolygons") == 0) {
        ynode *verts = NULL, *ind = NULL, *nrm;
        int npolys, nv, at;
        const float **pv, **pn;
        for (i = 0; i < n->nkids; i++) {
            if (strcmp(n->kids[i]->cls, "vertices") == 0)
                verts = n->kids[i];
            else if (strcmp(n->kids[i]->cls, "indices") == 0 || n->kids[i]->cls[0] == 0)
                ind = n->kids[i];
        }
        if (!verts || !ind || ind->nf < 1)
            return;
        nrm = find_assoc(verts, "normals");
        if (nrm && nrm->nf != verts->nf)
            nrm = NULL;
        nv = verts->nf / 3;
        /* number of polygons, then the size of each polygon, then the indices */
        npolys = (int)ind->f[0];
        if (npolys < 0 || 1 + npolys > ind->nf)
            return;
        at = 1 + npolys;
        for (i = 0; i < npolys; i++) {
            int cnt = (int)ind->f[1 + i];
            if (cnt <= 0 || at + cnt > ind->nf)
                break;
            pv = (const float **)malloc(sizeof(float *) * cnt);
            pn = (const float **)malloc(sizeof(float *) * cnt);
            for (j = 0; j < cnt; j++) {
                int k = (int)ind->f[at + j];
                if (k < 0 || k >= nv)
                    k = 0;
                pv[j] = verts->f + 3 * k;
                pn[j] = nrm ? nrm->f + 3 * k : NULL;
            }
            put_polygon(o, pv, pn, cnt);
            free(pv);
            free(pn);
            at += cnt;
        }
    } else if (strcmp(n->cls, "polygons") == 0) {
        /* each list of vertices is a polygon */
        for (i = 0; i < n->nkids; i++) {
            ynode *v = n->kids[i], *nrm;
            const float **pv, **pn;
            int cnt = v->nf / 3;
            if (strcmp(v->cls, "vertices") != 0 || cnt < 3)
                continue;
            nrm = find_assoc(v, "normals");
            if (nrm && nrm->nf != v->nf)
                nrm = NULL;
            pv = (const float **)malloc(sizeof(float *) * cnt);
            pn = (const float **)malloc(sizeof(float *) * cnt);
            for (j = 0; j < cnt; j++) {
                pv[j] = v->f + 3 * j;
                pn[j] = nrm ? nrm->f + 3 * j : NULL;
            }
            put_polygon(o, pv, pn, cnt);
            free(pv);
            free(pn);
        }
    } else if (strcmp(n->cls, "lines") == 0) {
        /* each list of vertices is a polyline */
        for (i = 0; i < n->nkids; i++) {
            ynode *v = n->kids[i];
            if (strcmp(v->cls, "vertices") != 0)
                continue;
            for (j = 0; j + 1 < v->nf / 3; j++)
                put_segment(o, v->f + 3 * j, v->f + 3 * j + 3);
        }
    } else {
        if (strcmp(n->cls, "group") != 0 && !warned(n->cls))
            fprintf(stderr, "yaodl: ignoring the attributes of %s\n", n->cls);
        for (i = 0; i < n->nkids; i++)
            emit(o, n->kids[i], depth + 1);
    }
}

/* the first colors of the file, white by default */
static int find_color_in(ynode *n, float *c, int depth)
{
    int i;
    if (depth > 64)
        return 0;
    if (strcmp(n->cls, "colors") == 0 && n->nf >= 3) {
        c[0] = n->f[0], c[1] = n->f[1], c[2] = n->f[2];
        return 1;
    }
    for (i = 0; i < n->nassoc; i++)
        if (find_color_in(n->assoc[i], c, depth + 1))
            return 1;
    for (i = 0; i < n->nkids; i++)
        if (find_color_in(n->kids[i], c, depth + 1))
            return 1;
    return 0;
}

static void find_color(ynode *root, float *c)
{
    c[0] = c[1] = c[2] = 1;
    find_color_in(root, c, 0);
}

/* --- entry point ------------------------------------------------------------------- */

int is_yaodl_file(const char *name)
{
    const char *e = strrchr(name, '.');
    FILE *f;
    char buf[8] = { 0 };
    if (e && (strcmp(e, ".ydl") == 0 || strcmp(e, ".defs") == 0 || strcmp(e, ".yaodl") == 0))
        return 1;
    if ((f = fopen(name, "rb")) == NULL)
        return 0;
    fread(buf, 1, 6, f);
    fclose(f);
    return strncmp(buf, "#YAODL", 6) == 0 || buf[0] == '(';
}

flipobj *readyaodl(const char *name)
{
    FILE *f;
    yparser p;
    ynode *root;
    yout o;
    flipobj *obj;
    unsigned char *buf;
    long len;

    if ((f = fopen(name, "rb")) == NULL) {
        fprintf(stderr, "readyaodl: can't open input file %s\n", name);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (unsigned char *)malloc(len + 1);
    if (!buf || fread(buf, 1, len, f) != (size_t)len) {
        fprintf(stderr, "readyaodl: can't read %s\n", name);
        exit(1);
    }
    fclose(f);

    memset(&p, 0, sizeof(p));
    p.s = buf;
    p.len = len;
    p.file = name;
    root = new_node("group");
    parse_file(&p, root);
    if (p.error)
        fprintf(stderr, "readyaodl: %s: syntax error near byte %ld\n", name, p.pos);

    memset(&o, 0, sizeof(o));
    emit(&o, root, 0);
    free(buf);
    if (o.npoints == 0) {
        fprintf(stderr, "readyaodl: no polygons or lines in %s\n", name);
        exit(1);
    }

    obj = (flipobj *)calloc(1, sizeof(flipobj));
    obj->npoints = o.npoints;
    obj->data = o.data;
    obj->colors = 0;
    obj->wire = !o.polygons;
    find_color(root, obj->wirecolor);
    printf("%s: %d polygons%s\n", name, o.npoints / 4, obj->wire ? " (lines)" : "");
    return obj;
}
