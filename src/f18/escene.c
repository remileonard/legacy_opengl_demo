/*
 * escene.c - the script interpreter and the scene of EasyScene, as used by
 * the F-18 demo (see escene.h)
 *
 * A script is a list of statements ended by ';'.  '#' starts a comment
 * until the end of the line; the lines starting with '%' are directives
 * (%ifdef NAME, %ifndef NAME, %else, %endif, %define NAME).  A statement is
 * a keyword, its arguments (words, "strings" and numbers) and clauses
 * separated by ','.  The statements of the demo scripts:
 *
 *   path "dir:dir...";
 *   process app cull draw;  cpu N app isolate;  framerate F;  phase LOCK;
 *   pipe "name" add on screen N, viewport x0, x1, y0, y1;
 *   qdev "DEVICE" function "func" [, autorepeat];
 *   module "name" add, preinit "func", postapp "func"...;
 *   configure;
 *   object "name" load "file" [isectGeom "file"] [mask N];
 *   object "name" loadstatic "file";  object "name" type terrain;
 *   channel "name" add on pipe "pipe";
 *   channel "name" viewport l r b t, clip near, far, fov h v, fadeLOD;
 *   channel "name" attach "object";
 *   userCallback "func" argument "string";   (usercb is the same)
 *
 * A '.' after the phase of a module ("preinit. func") is accepted and
 * ignored.  The modules run in the order of the script.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "escene.h"
#include "dwb.h"

esScene es;

/* --- registry ------------------------------------------------------------- */

typedef struct Func {
    char *name;
    esFunc f;
} Func;

static Func *funcs;
static int nfuncs;

static char *dupstr(const char *s)
{
    char *d = (char *)malloc(strlen(s) + 1);
    strcpy(d, s);
    return d;
}

void esRegisterFunction(const char *name, esFunc f)
{
    int i;
    for (i = 0; i < nfuncs; i++)
        if (strcmp(funcs[i].name, name) == 0) {
            funcs[i].f = f;
            return;
        }
    funcs = (Func *)realloc(funcs, sizeof(Func) * (nfuncs + 1));
    funcs[nfuncs].name = dupstr(name);
    funcs[nfuncs++].f = f;
}

esFunc esFindFunction(const char *name)
{
    int i;
    for (i = 0; i < nfuncs; i++)
        if (strcmp(funcs[i].name, name) == 0)
            return funcs[i].f;
    return NULL;
}

/* a missing function is reported once */
static esFunc lookup(const char *name, const char *what)
{
    static char **missing;
    static int nmissing;
    esFunc f = esFindFunction(name);
    int i;
    if (f)
        return f;
    for (i = 0; i < nmissing; i++)
        if (strcmp(missing[i], name) == 0)
            return NULL;
    missing = (char **)realloc(missing, sizeof(char *) * (nmissing + 1));
    missing[nmissing++] = dupstr(name);
    fprintf(stderr, "easyScene: %s function \"%s\" not found\n", what, name);
    return NULL;
}

static char **defines;
static int ndefines;

void esDefine(const char *name)
{
    defines = (char **)realloc(defines, sizeof(char *) * (ndefines + 1));
    defines[ndefines++] = dupstr(name);
}

static int is_defined(const char *name)
{
    int i;
    for (i = 0; i < ndefines; i++)
        if (strcmp(defines[i], name) == 0)
            return 1;
    return 0;
}

/* --- modules -------------------------------------------------------------- */

typedef struct Module {
    char *name;
    char *fname[ES_NPHASES];
    esFunc f[ES_NPHASES];
} Module;

static Module *modules;
static int nmodules;

static const char *phase_names[ES_NPHASES] = {
    "preinit", "init", "postinit", "preapp", "postapp",
    "precull", "postcull", "predraw", "postdraw", "postrender", "exit"
};

static Module *find_module(const char *name)
{
    int i;
    for (i = 0; i < nmodules; i++)
        if (strcmp(modules[i].name, name) == 0)
            return &modules[i];
    return NULL;
}

void esRunPhase(int phase)
{
    int i;
    for (i = 0; i < nmodules; i++)
        if (modules[i].f[phase])
            modules[i].f[phase](NULL);
}

/* --- objects, channels ---------------------------------------------------- */

esObject *esFindObject(const char *name)
{
    int i;
    for (i = 0; i < es.nobjects; i++)
        if (strcmp(es.objects[i]->name, name) == 0)
            return es.objects[i];
    return NULL;
}

esChannel *esFindChannel(const char *name)
{
    int i;
    for (i = 0; i < es.nchannels; i++)
        if (strcmp(es.channels[i].name, name) == 0)
            return &es.channels[i];
    return NULL;
}

static esPipe *find_pipe(const char *name)
{
    int i;
    for (i = 0; i < es.npipes; i++)
        if (strcmp(es.pipes[i].name, name) == 0)
            return &es.pipes[i];
    return NULL;
}

esKeyBinding *esFindKey(const char *device)
{
    int i;
    for (i = 0; i < es.nkeys; i++)
        if (strcmp(es.keys[i].device, device) == 0)
            return &es.keys[i];
    return NULL;
}

void esObjectPos(esObject *o, float x, float y, float z, float h, float p, float r)
{
    pfSetVec3(o->xyz, x, y, z);
    pfSetVec3(o->hpr, h, p, r);
    if (o->node && o->node->type == PFNODE_DCS) {
        pfDCSTrans(o->node, x, y, z);
        pfDCSRot(o->node, h, p, r);
    }
}

void esChannelViewMatrix(const esChannel *c, float m[16])
{
    pfMatrix eye, off, world, inv;
    pfCoord co;
    int i, j;
    pfMakeIdentMat(eye);
    if (c->attach) {
        pfCopyVec3(co.xyz, c->attach->xyz);
        pfCopyVec3(co.hpr, c->attach->hpr);
        pfMakeCoordMat(eye, &co);
    }
    pfCopyVec3(co.xyz, c->viewpos);
    pfCopyVec3(co.hpr, c->viewrot);
    pfMakeCoordMat(off, &co);
    pfMultMat(world, off, eye);
    pfInvertOrthoMat(inv, world);
    /* Performer looks along +Y with Z up, OpenGL along -Z with Y up */
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            m[i * 4 + j] = inv[i][j];
    for (i = 0; i < 4; i++) {
        float y = m[i * 4 + 1], z = m[i * 4 + 2];
        m[i * 4 + 1] = z;
        m[i * 4 + 2] = -y;
    }
}

static void bounds(pfNode *n, pfMatrix m, pfVec3 lo, pfVec3 hi, int depth)
{
    int i, j, k;
    pfMatrix mm;
    if (!n || depth > 64)
        return;
    if (n->type == PFNODE_DCS || n->type == PFNODE_SCS)
        pfMultMat(mm, n->mat, m);
    else
        pfCopyMat(mm, m);
    for (i = 0; i < n->ngsets; i++) {
        const float *v = (const float *)n->gsets[i]->attr[PFGS_COORD3];
        if (!v)
            continue;
        for (j = 0; j < n->gsets[i]->nverts; j++) {
            pfVec3 p;
            pfXformPt3(p, v + 3 * j, mm);
            for (k = 0; k < 3; k++) {
                if (p[k] < lo[k]) lo[k] = p[k];
                if (p[k] > hi[k]) hi[k] = p[k];
            }
        }
    }
    for (i = 0; i < n->nchildren; i++)
        bounds(n->children[i], mm, lo, hi, depth + 1);
}

/* the models loaded once, cloned for the other objects */
typedef struct Loaded {
    char *file;
    pfNode *model;
    pfVec3 center;
    float radius;
} Loaded;

static Loaded *loaded;
static int nloaded;

static int load_object(esObject *o)
{
    char path[4096];
    pfNode *m = NULL;
    int i;
    for (i = 0; i < nloaded; i++)
        if (strcmp(loaded[i].file, o->file) == 0) {
            m = pfClone(loaded[i].model, 0);
            pfCopyVec3(o->center, loaded[i].center);
            o->radius = loaded[i].radius;
            break;
        }
    if (!m) {
        pfMatrix ident;
        pfVec3 lo = { 1e30f, 1e30f, 1e30f }, hi = { -1e30f, -1e30f, -1e30f };
        size_t n = strlen(o->file);
        if (!pfFindFile(o->file, path, 4)) {
            fprintf(stderr, "easyScene: cannot find %s for object %s\n", o->file, o->name);
            return 0;
        }
        if (n > 4 && strcmp(o->file + n - 4, ".dwb") == 0) {
            m = pfdLoadFile_dwb(path);
        } else {
            fprintf(stderr, "easyScene: %s: format not supported yet (object %s)\n", o->file, o->name);
            return 0;
        }
        if (!m)
            return 0;
        pfMakeIdentMat(ident);
        bounds(m, ident, lo, hi, 0);
        if (lo[0] <= hi[0]) {
            pfAddVec3(o->center, lo, hi);
            pfScaleVec3(o->center, 0.5f, o->center);
            o->radius = 0.5f * pfDistancePt3(lo, hi);
        }
        loaded = (Loaded *)realloc(loaded, sizeof(Loaded) * (nloaded + 1));
        loaded[nloaded].file = dupstr(o->file);
        loaded[nloaded].model = m;
        pfCopyVec3(loaded[nloaded].center, o->center);
        loaded[nloaded++].radius = o->radius;
    }
    o->model = m;
    o->node = o->is_static ? pfNewGroup() : pfNewDCS();
    pfAddChild(o->node, m);
    pfAddChild(es.root, o->node);
    return 1;
}

/* --- tokens --------------------------------------------------------------- */

enum { T_END, T_WORD, T_STRING, T_NUMBER, T_PUNCT };

typedef struct Token {
    int type;
    char text[1024];
    double num;
    int line;
} Token;

typedef struct Lexer {
    const char *s;
    int line;
    int skip[32];          /* %ifdef nesting: the lines are skipped */
    int depth;
    const char *file;
} Lexer;

static int skipping(const Lexer *L)
{
    int i;
    for (i = 0; i < L->depth; i++)
        if (L->skip[i])
            return 1;
    return 0;
}

static void directive(Lexer *L)
{
    char word[64], arg[256];
    int n = 0, k = 0;
    L->s++;
    while (isalpha((unsigned char)*L->s) && n < 63)
        word[n++] = *L->s++;
    word[n] = 0;
    while (*L->s == ' ' || *L->s == '\t')
        L->s++;
    while (*L->s && !isspace((unsigned char)*L->s) && k < 255)
        arg[k++] = *L->s++;
    arg[k] = 0;
    if (strcmp(word, "ifdef") == 0 || strcmp(word, "ifndef") == 0) {
        if (L->depth < 32)
            L->skip[L->depth++] = is_defined(arg) == (word[2] == 'n');
    } else if (strcmp(word, "else") == 0) {
        if (L->depth > 0)
            L->skip[L->depth - 1] = !L->skip[L->depth - 1];
    } else if (strcmp(word, "endif") == 0) {
        if (L->depth > 0)
            L->depth--;
    } else if (strcmp(word, "define") == 0) {
        if (!skipping(L))
            esDefine(arg);
    } else {
        fprintf(stderr, "%s:%d: unknown directive %%%s\n", L->file, L->line, word);
    }
    while (*L->s && *L->s != '\n')
        L->s++;
}

static void next(Lexer *L, Token *t)
{
    int at_line_start;
    for (;;) {
        while (*L->s && isspace((unsigned char)*L->s)) {
            if (*L->s == '\n')
                L->line++;
            L->s++;
        }
        /* a directive starts its line (the text starts with a newline) */
        {
            const char *p = L->s;
            while (p[-1] == ' ' || p[-1] == '\t')
                p--;
            at_line_start = p[-1] == '\n';
        }
        if (*L->s == '#') {
            while (*L->s && *L->s != '\n')
                L->s++;
            continue;
        }
        if (*L->s == '%' && at_line_start) {
            directive(L);
            continue;
        }
        if (skipping(L) && *L->s) {
            /* the lines of a false %ifdef */
            while (*L->s && *L->s != '\n')
                L->s++;
            continue;
        }
        break;
    }
    t->line = L->line;
    t->text[0] = 0;
    if (!*L->s) {
        t->type = T_END;
        return;
    }
    if (*L->s == '"') {
        int n = 0;
        L->s++;
        while (*L->s && *L->s != '"' && n < 1023) {
            if (*L->s == '\n')
                L->line++;
            t->text[n++] = *L->s++;
        }
        t->text[n] = 0;
        if (*L->s == '"')
            L->s++;
        t->type = T_STRING;
        return;
    }
    if (isdigit((unsigned char)*L->s) || ((*L->s == '-' || *L->s == '+' || *L->s == '.') &&
                                         isdigit((unsigned char)L->s[1]))) {
        char *e;
        int n;
        if (L->s[0] == '0' && (L->s[1] == 'x' || L->s[1] == 'X'))
            t->num = (double)strtoul(L->s, &e, 16);
        else
            t->num = strtod(L->s, &e);
        n = (int)(e - L->s);
        if (n > 1023)
            n = 1023;
        memcpy(t->text, L->s, n);
        t->text[n] = 0;
        L->s = e;
        t->type = T_NUMBER;
        return;
    }
    if (isalpha((unsigned char)*L->s) || *L->s == '_') {
        int n = 0;
        while ((isalnum((unsigned char)*L->s) || *L->s == '_') && n < 1023)
            t->text[n++] = *L->s++;
        t->text[n] = 0;
        t->type = T_WORD;
        return;
    }
    t->text[0] = *L->s++;
    t->text[1] = 0;
    t->type = T_PUNCT;
}

/* --- statements ----------------------------------------------------------- */

#define MAX_TOKENS 256

typedef struct Stmt {
    Token tok[MAX_TOKENS];
    int n, pos;
    const char *file;
} Stmt;

static Token *peek(Stmt *s)
{
    static Token end;
    /* the commas and the dots of the clauses carry no meaning */
    while (s->pos < s->n && s->tok[s->pos].type == T_PUNCT &&
           (s->tok[s->pos].text[0] == ',' || s->tok[s->pos].text[0] == '.'))
        s->pos++;
    if (s->pos >= s->n) {
        end.type = T_END;
        end.text[0] = 0;
        return &end;
    }
    return &s->tok[s->pos];
}

static Token *take(Stmt *s)
{
    Token *t = peek(s);
    if (t->type != T_END)
        s->pos++;
    return t;
}

static int accept(Stmt *s, const char *word)
{
    Token *t = peek(s);
    if (t->type == T_WORD && strcmp(t->text, word) == 0) {
        s->pos++;
        return 1;
    }
    return 0;
}

static const char *str_arg(Stmt *s)
{
    Token *t = take(s);
    return t->type == T_STRING || t->type == T_WORD || t->type == T_NUMBER ? t->text : "";
}

static double num_arg(Stmt *s)
{
    Token *t = peek(s);
    if (t->type != T_NUMBER)
        return 0;
    s->pos++;
    return t->num;
}

static void warn(Stmt *s, const char *msg, const char *arg)
{
    fprintf(stderr, "%s:%d: %s%s%s\n", s->file, s->n ? s->tok[0].line : 0, msg,
            arg ? " " : "", arg ? arg : "");
}

/* the directories of a path, relative to the current directory or to the
 * directory of the demo (the parent of the scripts directory), where the
 * original "./data/models" is the "models" of our data/f18 */
static char script_root[4096];

static int is_dir(const char *p)
{
    struct stat st;
    return stat(p, &st) == 0 && (st.st_mode & S_IFDIR) != 0;
}

static void set_path(const char *spec)
{
    char out[16384] = "";
    const char *p = spec;
    while (*p) {
        const char *e = strchr(p, ':');
        size_t n = e ? (size_t)(e - p) : strlen(p);
        char dir[1024], cand[3][8192];
        int k;
        if (n >= sizeof(dir))
            n = sizeof(dir) - 1;
        memcpy(dir, p, n);
        dir[n] = 0;
        p += n;
        if (*p == ':')
            p++;
        if (!dir[0])
            continue;
        sprintf(cand[0], "%s", dir);
        sprintf(cand[1], "%s/%s", script_root, dir);
        {
            const char *d = dir;
            if (strncmp(d, "./", 2) == 0)
                d += 2;
            if (strncmp(d, "data/", 5) == 0)
                d += 5;
            sprintf(cand[2], "%s/%s", script_root, d);
        }
        for (k = 0; k < 3; k++)
            if (is_dir(cand[k]) && strlen(out) + strlen(cand[k]) + 2 < sizeof(out)) {
                if (out[0])
                    strcat(out, ":");
                strcat(out, cand[k]);
            }
    }
    free(es.path);
    es.path = dupstr(out[0] ? out : ".");
    pfFilePath(es.path);
}

static void stmt_pipe(Stmt *s)
{
    const char *name = str_arg(s);
    esPipe *p = find_pipe(name);
    if (accept(s, "add")) {
        if (p) {
            warn(s, "pipe already exists:", name);
        } else {
            es.pipes = (esPipe *)realloc(es.pipes, sizeof(esPipe) * (es.npipes + 1));
            p = &es.pipes[es.npipes++];
            memset(p, 0, sizeof(*p));
            p->name = dupstr(name);
            p->viewport[1] = 1279;
            p->viewport[3] = 1023;
        }
    }
    if (!p) {
        warn(s, "unknown pipe", name);
        return;
    }
    while (peek(s)->type != T_END) {
        if (accept(s, "on")) {
            if (accept(s, "screen"))
                p->screen = (int)num_arg(s);
        } else if (accept(s, "viewport")) {
            int k;
            for (k = 0; k < 4; k++)
                p->viewport[k] = (int)num_arg(s);
        } else {
            warn(s, "pipe: ignored", take(s)->text);
        }
    }
}

static void stmt_qdev(Stmt *s)
{
    const char *dev = str_arg(s);
    esKeyBinding *k = esFindKey(dev);
    if (!k) {
        es.keys = (esKeyBinding *)realloc(es.keys, sizeof(esKeyBinding) * (es.nkeys + 1));
        k = &es.keys[es.nkeys++];
        memset(k, 0, sizeof(*k));
        k->device = dupstr(dev);
    }
    while (peek(s)->type != T_END) {
        if (accept(s, "function")) {
            free(k->function);
            k->function = dupstr(str_arg(s));
            k->func = lookup(k->function, "qdev");
        } else if (accept(s, "autorepeat")) {
            k->autorepeat = 1;
        } else {
            warn(s, "qdev: ignored", take(s)->text);
        }
    }
}

static void stmt_module(Stmt *s)
{
    const char *name = str_arg(s);
    Module *m = find_module(name);
    if (accept(s, "add")) {
        if (m) {
            warn(s, "module already exists:", name);
        } else {
            modules = (Module *)realloc(modules, sizeof(Module) * (nmodules + 1));
            m = &modules[nmodules++];
            memset(m, 0, sizeof(*m));
            m->name = dupstr(name);
        }
    }
    if (!m) {
        warn(s, "module does not exist:", name);
        return;
    }
    while (peek(s)->type != T_END) {
        Token *t = take(s);
        int ph;
        for (ph = 0; ph < ES_NPHASES; ph++)
            if (t->type == T_WORD && strcmp(t->text, phase_names[ph]) == 0)
                break;
        if (ph == ES_NPHASES) {
            warn(s, "module: ignored", t->text);
            continue;
        }
        free(m->fname[ph]);
        m->fname[ph] = dupstr(str_arg(s));
        m->f[ph] = lookup(m->fname[ph], phase_names[ph]);
    }
}

static void stmt_object(Stmt *s)
{
    const char *name = str_arg(s);
    esObject *o = esFindObject(name);
    while (peek(s)->type != T_END) {
        int st = 0;
        if (accept(s, "load") || (st = accept(s, "loadstatic")) != 0) {
            const char *file = str_arg(s);
            if (o) {
                warn(s, "object already exists:", name);
                return;
            }
            o = (esObject *)calloc(1, sizeof(esObject));
            o->name = dupstr(name);
            o->file = dupstr(file);
            o->is_static = st;
            o->isect_mask = 0xffffffffU;
            es.objects = (esObject **)realloc(es.objects, sizeof(esObject *) * (es.nobjects + 1));
            es.objects[es.nobjects++] = o;
            load_object(o);
        } else if (!o) {
            warn(s, "object not loaded yet:", name);
            return;
        } else if (accept(s, "isectGeom")) {
            free(o->isect_file);
            o->isect_file = dupstr(str_arg(s));
        } else if (accept(s, "mask")) {
            o->isect_mask = (unsigned int)num_arg(s);
        } else if (accept(s, "type")) {
            if (accept(s, "terrain"))
                o->is_terrain = 1;
            else
                warn(s, "object type ignored:", take(s)->text);
        } else {
            warn(s, "object: ignored", take(s)->text);
        }
    }
}

static void stmt_channel(Stmt *s)
{
    const char *name = str_arg(s);
    esChannel *c = esFindChannel(name);
    if (accept(s, "add")) {
        if (c) {
            warn(s, "channel already exists:", name);
            return;
        }
        es.channels = (esChannel *)realloc(es.channels, sizeof(esChannel) * (es.nchannels + 1));
        c = &es.channels[es.nchannels++];
        memset(c, 0, sizeof(*c));
        c->name = dupstr(name);
        c->viewport[1] = c->viewport[3] = 1;
        c->near_clip = 1;
        c->far_clip = 10000;
        c->fov_h = 45;
        c->fov_v = -1;
        if (accept(s, "on") && accept(s, "pipe")) {
            const char *pn = str_arg(s);
            if ((c->pipe = find_pipe(pn)) == NULL)
                warn(s, "couldn't find pipe", pn);
        }
    }
    if (!c) {
        warn(s, "channel does not exist:", name);
        return;
    }
    while (peek(s)->type != T_END) {
        int k;
        if (accept(s, "viewport")) {
            for (k = 0; k < 4; k++)
                c->viewport[k] = (float)num_arg(s);
        } else if (accept(s, "clip")) {
            c->near_clip = (float)num_arg(s);
            c->far_clip = (float)num_arg(s);
        } else if (accept(s, "fov")) {
            c->fov_h = (float)num_arg(s);
            c->fov_v = (float)num_arg(s);
        } else if (accept(s, "fadeLOD")) {
            c->fade_lod = 1;
        } else if (accept(s, "viewpos")) {
            for (k = 0; k < 3; k++)
                c->viewpos[k] = (float)num_arg(s);
        } else if (accept(s, "viewrot")) {
            for (k = 0; k < 3; k++)
                c->viewrot[k] = (float)num_arg(s);
        } else if (accept(s, "attach")) {
            const char *on = str_arg(s);
            if ((c->attach = esFindObject(on)) == NULL)
                warn(s, "cannot find object to attach channel:", on);
        } else {
            warn(s, "channel: ignored", take(s)->text);
        }
    }
}

static void stmt_usercb(Stmt *s)
{
    const char *fname = str_arg(s);
    char func[1024], arg[1024] = "";
    esFunc f;
    strcpy(func, fname);
    if (accept(s, "argument"))
        strcpy(arg, str_arg(s));
    if ((f = lookup(func, "userCallback")) != NULL)
        f(arg);
}

static void run_stmt(Stmt *s, void (*configure_cb)(void))
{
    Token *t = take(s);
    const char *kw = t->text;
    if (t->type != T_WORD) {
        warn(s, "syntax error near", kw);
        return;
    }
    if (strcmp(kw, "path") == 0) {
        set_path(str_arg(s));
    } else if (strcmp(kw, "framerate") == 0) {
        es.framerate = (float)num_arg(s);
    } else if (strcmp(kw, "process") == 0 || strcmp(kw, "cpu") == 0 || strcmp(kw, "phase") == 0) {
        /* one process, the frames are not locked */
    } else if (strcmp(kw, "pipe") == 0) {
        stmt_pipe(s);
    } else if (strcmp(kw, "qdev") == 0) {
        stmt_qdev(s);
    } else if (strcmp(kw, "module") == 0) {
        stmt_module(s);
    } else if (strcmp(kw, "configure") == 0) {
        if (es.npipes == 0) {
            warn(s, "No pipes defined. Cannot start easyScene", NULL);
            return;
        }
        es.configured = 1;
        if (configure_cb)
            configure_cb();
        esRunPhase(ES_PREINIT);
        esRunPhase(ES_INIT);
    } else if (strcmp(kw, "object") == 0) {
        stmt_object(s);
    } else if (strcmp(kw, "channel") == 0) {
        stmt_channel(s);
    } else if (strcmp(kw, "userCallback") == 0 || strcmp(kw, "usercb") == 0) {
        stmt_usercb(s);
    } else {
        warn(s, "unknown statement", kw);
    }
}

int esLoadScript(const char *file, void (*configure_cb)(void))
{
    FILE *fp = fopen(file, "rb");
    char *text, *sep;
    long size;
    Lexer L;
    Stmt *s;
    Token t;
    int i;

    if (!fp) {
        fprintf(stderr, "easyScene: cannot open %s\n", file);
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    text = (char *)malloc(size + 2);
    text[0] = '\n';                 /* so that the first line starts a line */
    size = (long)fread(text + 1, 1, size, fp);
    text[size + 1] = 0;
    fclose(fp);

    /* the root of the demo: the parent of the directory of the script */
    strncpy(script_root, file, sizeof(script_root) - 1);
    for (i = 0; script_root[i]; i++)
        if (script_root[i] == '\\')
            script_root[i] = '/';
    sep = strrchr(script_root, '/');
    if (sep) {
        *sep = 0;
        sep = strrchr(script_root, '/');
        if (sep)
            *sep = 0;
        else
            strcpy(script_root, ".");
    } else {
        strcpy(script_root, "..");
    }
    free(es.script);
    es.script = dupstr(file);
    if (!es.root)
        es.root = pfNewScene();
    if (!es.framerate)
        es.framerate = 60;

    memset(&L, 0, sizeof(L));
    L.s = text + 1;
    L.line = 1;
    L.file = file;
    s = (Stmt *)calloc(1, sizeof(Stmt));
    s->file = file;
    for (;;) {
        next(&L, &t);
        if (t.type == T_END)
            break;
        if (t.type == T_PUNCT && t.text[0] == ';') {
            if (s->n)
                run_stmt(s, configure_cb);
            s->n = s->pos = 0;
            continue;
        }
        if (s->n < MAX_TOKENS)
            s->tok[s->n++] = t;
    }
    if (s->n)
        run_stmt(s, configure_cb);
    free(s);
    free(text);
    esRunPhase(ES_POSTINIT);
    return 1;
}
