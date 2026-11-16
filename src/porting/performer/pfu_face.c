/*
 * pfu_face.c - the "face" perfly (perfly.face of the Earth to Matterhorn
 * demo), rebuilt from its IRIX executable: when a file named "paths" is in
 * the current folder, the databases of the command line are split into
 * worlds, shown one at a time, and the eye is held on a tether around the
 * center of the world. Zooming in past the inner radius of a world goes to
 * the next one, zooming out past its outer radius back to the previous one.
 * The transition worlds hold two pictures (trans_1, trans_2) faded into
 * each other with the radius.
 *
 * Without "paths" the program is the normal perfly, like perfly.face.
 *
 * paths (readPathFile): blocks starting with "world n", then
 *   files n               the next n files of the command line
 *   numTextures n         the next n textures of texture.config
 *   textureOrder i i ...  the texture of each geoset, -1: none (default:
 *                         geoset k takes texture k)
 *   lighting 0|1          lighting of the textured geosets
 *   center x y z
 *   minRadius, maxRadius, inRadius, outRadius
 *   inAround, outAround, inTilt, outTilt     the eye entering the world
 *                         from inside (in) or from outside (out)
 *   nearMinAround ... farMaxTilt, minRoll, maxRoll
 *                         the limits of the angles at the inner (near)
 *                         and the outer (far) radius
 *   nearClip, farClip, skyColor r g b
 *   scaleLinearVelocity, maxAngularVelocity, maxRollVelocity
 *   tileSize              the texture paging of the original, unused
 *
 * texture.config (pfuSetTextureLayout): the number of textures, the number
 * of layouts, the layouts (letter, width, height, levels), then one line
 * per texture: a file name with %d (the folder of a mipmap level, 1 the
 * largest) and its layout letter. The largest level found is loaded, the
 * smaller ones are made by OpenGL.
 *
 * The images are 16 bit RGBA 5551; the .5551E ones are scrambled
 * (getImage5551E): a header "SEn", the count of inserted bytes, the width
 * and the height (big-endian ints), then the bytes xor'ed with 13 plus
 * their position, with runs of junk bytes inserted after runs of image
 * bytes whose lengths come from numbers5551E; the image is written from
 * its last byte back to its first.
 *
 * The motion of the tether is my own (pfiInputXformTether is not in the
 * executable as source): the left button goes in, the right button goes
 * out, the mouse away from the center of the view turns around and tilts;
 * Page Up / Page Down and the arrows do the same from the keyboard.
 */
#include <GL/freeglut.h>

#include "pf_internal.h"

#include <ctype.h>
#include <math.h>

#define MAX_WORLDS 64
#define MAX_ORDER 64
#define MAX_TEXTURES 256

typedef struct {
    int files, ntex, norder, order[MAX_ORDER];
    int lighting;
    pfVec3 center;
    float minRadius, maxRadius, inRadius, outRadius;
    float inAround, outAround, inTilt, outTilt;
    float nearMinAround, nearMaxAround, farMinAround, farMaxAround;
    float nearMinTilt, nearMaxTilt, farMinTilt, farMaxTilt;
    float nearClip, farClip;
    pfVec3 sky;
    float linear, angular, roll;
    int texbase;
    pfGroup *group;
    int textured;
} World;

static World worlds[MAX_WORLDS];
static int nworlds, active, current;
static pfSwitch *wswitch;
static pfChannel *face_chan;
static char texname[MAX_TEXTURES][256];
static pfTexture *textures[MAX_TEXTURES];
static int ntexnames;
static float radius, around, tilt;
/* the held keys: in, out, left, right, up, down */
static int kin, kout, kleft, kright, kup, kdown;

int pfuiFaceActive(void) { return active; }

/* --- the 5551E images ------------------------------------------------------- */

static void numbers5551E(unsigned int *s, unsigned int *count, unsigned int *skip)
{
    const unsigned int X = 10733, Y = 951;
    *count = ((X >> *s) + (Y << *s)) % 0x7fffffffU;
    *skip = ((X << *s) + (Y >> *s)) % 37;
    *s = (*s + 5) % 7;
}

static unsigned int be32(const unsigned char *p)
{
    return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) | ((unsigned int)p[2] << 8) | p[3];
}

/* the RGBA bytes of a 5551E file, NULL: not one */
static unsigned char *read_5551E(const char *file, int *w, int *h)
{
    FILE *fp = fopen(file, "rb");
    unsigned char *in, *out, *rgba;
    long size, n, i;
    unsigned int s = 2, count, skip;
    long pos;
    if (!fp)
        return NULL;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    in = (unsigned char *)malloc(size > 0 ? size : 1);
    if (size < 15 || fread(in, 1, size, fp) != (size_t)size || memcmp(in, "SEn", 3) != 0) {
        fclose(fp);
        free(in);
        return NULL;
    }
    fclose(fp);
    *w = (int)be32(in + 7);
    *h = (int)be32(in + 11);
    if (*w <= 0 || *h <= 0 || *w > 16384 || *h > 16384) {
        free(in);
        return NULL;
    }
    out = (unsigned char *)calloc((size_t)*w * *h, 2);
    pos = (long)*w * *h * 2 - 1;
    n = size - 15;
    numbers5551E(&s, &count, &skip);
    for (i = 0; i < n && pos >= 0; i++) {
        unsigned char b = (unsigned char)(in[15 + i] ^ ((13 + i) & 0xff));
        if (count) {
            count--;
            out[pos--] = b;
        } else if (skip) {
            skip--;
        } else {
            numbers5551E(&s, &count, &skip);
            out[pos--] = b;
        }
    }
    free(in);
    rgba = (unsigned char *)malloc((size_t)*w * *h * 4);
    for (i = 0; i < (long)*w * *h; i++) {
        unsigned int v = ((unsigned int)out[2 * i] << 8) | out[2 * i + 1];
        rgba[4 * i] = (unsigned char)(((v >> 11) & 31) * 255 / 31);
        rgba[4 * i + 1] = (unsigned char)(((v >> 6) & 31) * 255 / 31);
        rgba[4 * i + 2] = (unsigned char)(((v >> 1) & 31) * 255 / 31);
        rgba[4 * i + 3] = (unsigned char)(v & 1 ? 255 : 0);
    }
    free(out);
    return rgba;
}

/* the largest level of a texture of texture.config: its file name with
 * %d from 1, or the file without its folder */
static pfTexture *load_texture(int k)
{
    char path[512];
    const char *fmt = texname[k], *base;
    unsigned char *img = NULL;
    int level, w = 0, h = 0;
    pfTexture *t;
    if (textures[k] || !fmt[0])
        return textures[k];
    for (level = 1; level <= 16 && !img; level++) {
        sprintf(path, fmt, level);
        img = read_5551E(path, &w, &h);
    }
    if (!img) {
        base = strrchr(fmt, '/');
        strcpy(path, base ? base + 1 : fmt);
        img = read_5551E(path, &w, &h);
    }
    if (!img) {
        /* an image converted from it, in the formats of the layer: its
         * name without .5551E (earthR.sgi) */
        char found[1024], *e = strstr(path, ".5551E");
        if (e)
            *e = 0;
        t = pfNewTex(NULL);
        if (pfFindFile(path, found, R_OK) && pfLoadTexFile(t, found))
            return textures[k] = t;
        free(t);
        pfNotify(PFNFY_NOTICE, PFNFY_RESOURCE, "face: texture %s not found", fmt);
        return NULL;
    }
    t = pfNewTex(NULL);
    t->image = (unsigned int *)img;
    t->xsize = w;
    t->ysize = h;
    t->comps = 4;
    t->name = (char *)malloc(strlen(path) + 1);
    strcpy(t->name, path);
    pfNotify(PFNFY_INFO, PFNFY_PRINT, "face: texture %d %s, %dx%d", k, path, w, h);
    return textures[k] = t;
}

/* --- the files ---------------------------------------------------------------- */

static void read_texture_config(void)
{
    FILE *fp = fopen("texture.config", "r");
    char line[512];
    int numbers = 0, ntex = 0, nlayouts = 0;
    if (!fp)
        return;
    while (fgets(line, sizeof(line), fp)) {
        char *s = line, name[256], letter;
        while (isspace((unsigned char)*s))
            s++;
        if (*s == '#' || !*s)
            continue;
        if (numbers == 0) {
            ntex = atoi(s);
            numbers++;
        } else if (numbers == 1) {
            nlayouts = atoi(s);
            numbers++;
        } else if (nlayouts > 0) {
            nlayouts--;     /* the layout: letter, size, levels */
        } else if (sscanf(s, "%255s %c", name, &letter) == 2 && ntexnames < MAX_TEXTURES) {
            strcpy(texname[ntexnames++], name);
        }
    }
    fclose(fp);
    if (ntex != ntexnames)
        pfNotify(PFNFY_WARN, PFNFY_PRINT, "face: texture.config lists %d textures, not %d",
                 ntexnames, ntex);
}

static void world_defaults(World *w)
{
    memset(w, 0, sizeof(*w));
    w->maxRadius = 5000;
    w->inRadius = 20;
    w->outRadius = 5000;
    w->nearMinAround = w->farMinAround = w->nearMinTilt = w->farMinTilt = -180;
    w->nearMaxAround = w->farMaxAround = w->nearMaxTilt = w->farMaxTilt = 180;
    w->nearClip = 1;
    w->farClip = 100;
    w->linear = 1;
    w->angular = 30;
    w->roll = 30;
}

static int read_paths(void)
{
    FILE *fp = fopen("paths", "r");
    char line[512];
    World *w = NULL;
    if (!fp)
        return 0;
    while (fgets(line, sizeof(line), fp)) {
        char tag[64];
        float v = 0;
        char *s = line;
        while (isspace((unsigned char)*s))
            s++;
        if (*s == '#' || !*s || sscanf(s, "%63s %f", tag, &v) < 1)
            continue;
        if (strcmp(tag, "world") == 0) {
            if (nworlds == MAX_WORLDS)
                break;
            w = &worlds[nworlds++];
            world_defaults(w);
            continue;
        }
        if (!w) {
            pfNotify(PFNFY_WARN, PFNFY_PRINT, "face: the first tag of paths must be <world>");
            fclose(fp);
            return 0;
        }
#define TAG(name, field) else if (strcmp(tag, name) == 0) w->field = v
        if (strcmp(tag, "files") == 0) w->files = (int)v;
        TAG("numTextures", ntex);
        TAG("lighting", lighting);
        TAG("minRadius", minRadius);
        TAG("maxRadius", maxRadius);
        TAG("inRadius", inRadius);
        TAG("outRadius", outRadius);
        TAG("inAround", inAround);
        TAG("outAround", outAround);
        TAG("inTilt", inTilt);
        TAG("outTilt", outTilt);
        TAG("nearMinAround", nearMinAround);
        TAG("nearMaxAround", nearMaxAround);
        TAG("farMinAround", farMinAround);
        TAG("farMaxAround", farMaxAround);
        TAG("nearMinTilt", nearMinTilt);
        TAG("nearMaxTilt", nearMaxTilt);
        TAG("farMinTilt", farMinTilt);
        TAG("farMaxTilt", farMaxTilt);
        TAG("nearClip", nearClip);
        TAG("farClip", farClip);
        TAG("scaleLinearVelocity", linear);
        TAG("maxAngularVelocity", angular);
        TAG("maxRollVelocity", roll);
        else if (strcmp(tag, "center") == 0)
            sscanf(s, "%*s %f %f %f", &w->center[0], &w->center[1], &w->center[2]);
        else if (strcmp(tag, "skyColor") == 0)
            sscanf(s, "%*s %f %f %f", &w->sky[0], &w->sky[1], &w->sky[2]);
        else if (strcmp(tag, "textureOrder") == 0) {
            char *p = s + strlen("textureOrder");
            int n;
            w->norder = 0;
            while (w->norder < MAX_ORDER && sscanf(p, "%d%n", &w->order[w->norder], &n) == 1) {
                w->norder++;
                p += n;
            }
        } else if (strcmp(tag, "tileSize") != 0 && strcmp(tag, "minRoll") != 0 &&
                   strcmp(tag, "maxRoll") != 0)
            pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "face: unknown tag <%s> in paths", tag);
#undef TAG
    }
    fclose(fp);
    return nworlds > 0;
}

/* --- the textures of the geosets (helpTouchGeoSets) ----------------------------- */

static void touch_geosets(World *w, pfNode *n, int *k, int depth)
{
    int i, limit = w->norder ? w->norder : w->ntex;
    if (!n || depth > 64)
        return;
    for (i = 0; i < n->ngsets; i++) {
        pfGeoSet *g = n->gsets[i];
        pfGeoState *gs = pfNewGState(NULL);
        int offset = *k < limit ? (w->norder ? w->order[*k] : *k) : -1;
        pfTexture *t = offset >= 0 ? load_texture(w->texbase + offset) : NULL;
        if (g->gstate)
            *gs = *g->gstate;
        gs->texturing = t != NULL;
        gs->texture = t;
        gs->texenv = NULL;
        gs->lighting = w->lighting;
        if (t) {
            gs->alphafunc = GL_GREATER;
            gs->alpharef = 0.5f;
        }
        g->gstate = gs;
        (*k)++;
    }
    for (i = 0; i < n->nchildren; i++)
        touch_geosets(w, n->children[i], k, depth + 1);
}

/* the first geoset under a node */
static pfGeoSet *first_gset(pfNode *n, int depth)
{
    int i;
    if (!n || depth > 64)
        return NULL;
    if (n->ngsets)
        return n->gsets[0];
    for (i = 0; i < n->nchildren; i++) {
        pfGeoSet *g = first_gset(n->children[i], depth + 1);
        if (g)
            return g;
    }
    return NULL;
}

static int is_transition(const World *w)
{
    return w->files == 2 && w->outRadius <= 3;
}

/* --- worlds ---------------------------------------------------------------------- */

/* the scene of perfly: the loaded files are the children of its group,
 * under its trackball DCS or not */
static pfGroup *scene_group(pfScene *scene)
{
    pfNode *c = pfGetChild(scene, 0);
    if (c && c->type == PFNODE_DCS)
        c = pfGetChild(c, 0);
    return c && c->type == PFNODE_GROUP ? c : NULL;
}

/* the clip planes and the color of the sky of the world, set at the first
 * frame in it: perfly sets its own after the scene */
static int view_pending;

static void apply_view(const World *w)
{
    pfEarthSky *es;
    view_pending = 0;
    if (!face_chan)
        return;
    pfChanNearFar(face_chan, w->nearClip, w->farClip);
    if ((es = pfiChanESky(face_chan)) != NULL)
        pfESkyColor(es, PFES_CLEAR, w->sky[0], w->sky[1], w->sky[2], 1);
}

static void enter_world(int k, int from_outside)
{
    World *w;
    if (k < 0 || k >= nworlds)
        return;
    w = &worlds[k];
    current = k;
    if (!w->textured && w->group) {
        int count = 0;
        touch_geosets(w, w->group, &count, 0);
        w->textured = 1;
    }
    pfSwitchVal(wswitch, k);
    radius = from_outside ? w->outRadius : w->inRadius;
    around = from_outside ? w->outAround : w->inAround;
    tilt = from_outside ? w->outTilt : w->inTilt;
    view_pending = 1;
    pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "face: world %d", k);
}

void pfuiFaceAttach(pfScene *scene, pfChannel *chan)
{
    pfGroup *group;
    int i, f = 0, nroots, base = 0;
    pfNode **roots;
    if (active || !scene || !read_paths())
        return;
    read_texture_config();
    if ((group = scene_group(scene)) == NULL) {
        pfNotify(PFNFY_WARN, PFNFY_PRINT, "face: the scene of perfly is not found");
        return;
    }
    nroots = pfGetNumChildren(group);
    roots = (pfNode **)malloc(sizeof(pfNode *) * (nroots + 1));
    for (i = 0; i < nroots; i++)
        roots[i] = pfGetChild(group, i);
    for (i = 0; i < nroots; i++)
        pfRemoveChild(group, roots[i]);
    wswitch = pfNewSwitch();
    for (i = 0; i < nworlds; i++) {
        World *w = &worlds[i];
        int j;
        w->group = pfNewGroup();
        w->texbase = base;
        base += w->ntex;
        for (j = 0; j < w->files && f < nroots; j++)
            pfAddChild(w->group, roots[f++]);
        pfAddChild(wswitch, w->group);
    }
    if (f != nroots)
        pfNotify(PFNFY_WARN, PFNFY_PRINT, "face: paths uses %d files, %d were loaded", f, nroots);
    for (; f < nroots; f++)
        pfAddChild(group, roots[f]);
    free(roots);
    pfAddChild(group, wswitch);
    face_chan = chan;
    active = 1;
    pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "face: %d worlds, %d textures", nworlds, ntexnames);
    enter_world(0, 1);
}

/* the keys of the tether, held */
int pfuiFaceSpecial(int key, int down)
{
    int *k = NULL;
    if (!active)
        return 0;
    switch (key) {
    case GLUT_KEY_PAGE_UP: k = &kin; break;
    case GLUT_KEY_PAGE_DOWN: k = &kout; break;
    case GLUT_KEY_LEFT: k = &kleft; break;
    case GLUT_KEY_RIGHT: k = &kright; break;
    case GLUT_KEY_UP: k = &kup; break;
    case GLUT_KEY_DOWN: k = &kdown; break;
    }
    if (!k)
        return 0;
    *k = down;
    return 1;
}

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

/* the eye of the frame: the tether moved by the mouse and the keys */
int pfuiFaceEye(pfuMouse *m, int inside, int cx, int cy, int cw, int ch, float dt, pfCoord *eye)
{
    World *w;
    float t, range, nx = 0, ny = 0, lo, hi, ca, sa, ct, st;
    int buttons = 0, in, out;
    if (!active)
        return 0;
    w = &worlds[current];
    if (m && inside) {
        buttons = m->flags & PFUDEV_MOUSE_DOWN_MASK;
        if ((buttons & PFUDEV_MOUSE_LEFT_DOWN) && (m->modifiers & PFUDEV_MOD_CTRL))
            buttons = PFUDEV_MOUSE_RIGHT_DOWN;
        else if ((buttons & PFUDEV_MOUSE_LEFT_DOWN) && (m->modifiers & PFUDEV_MOD_SHIFT))
            buttons = PFUDEV_MOUSE_MIDDLE_DOWN;
        nx = (m->xpos - (cx + cw / 2.0f)) / (cw / 2.0f);
        ny = (m->ypos - (cy + ch / 2.0f)) / (ch / 2.0f);
        nx = PF_CLAMP(nx, -1, 1);
        ny = PF_CLAMP(ny, -1, 1);
        if (fabs(nx) < 0.05f) nx = 0;
        if (fabs(ny) < 0.05f) ny = 0;
    }
    in = kin || (buttons & PFUDEV_MOUSE_LEFT_DOWN);
    out = kout || (buttons & PFUDEV_MOUSE_RIGHT_DOWN);
    if (!buttons) {
        nx = 0;
        ny = 0;
    }
    nx += (kright ? 0.5f : 0) - (kleft ? 0.5f : 0);
    ny += (kup ? 0.5f : 0) - (kdown ? 0.5f : 0);

    /* in and out: the whole range of the world in 5 seconds at the speed 1 */
    range = w->outRadius - w->inRadius;
    if (range <= 0)
        range = PF_MAX2(1.0f, w->maxRadius - w->minRadius);
    if (in) radius -= w->linear * range / 5 * dt;
    if (out) radius += w->linear * range / 5 * dt;

    /* the angles, at most the angular velocity per frame of 1/60 s */
    around -= nx * w->angular * 60 * 0.5f * dt;
    tilt += ny * w->angular * 60 * 0.5f * dt;

    if (in && radius <= w->inRadius && w->inRadius >= w->minRadius) {
        enter_world((current + 1) % nworlds, 1);
        w = &worlds[current];
    } else if (out && radius >= w->outRadius && current > 0 && w->outRadius <= w->maxRadius) {
        enter_world(current - 1, 0);
        w = &worlds[current];
    }
    radius = PF_CLAMP(radius, w->minRadius, w->maxRadius);
    if (view_pending)
        apply_view(w);

    /* the limits of the angles, from those at the inner radius to those at
     * the outer one */
    t = w->outRadius > w->inRadius ? (radius - w->inRadius) / (w->outRadius - w->inRadius) : 0;
    t = PF_CLAMP(t, 0, 1);
    lo = lerp(w->nearMinAround, w->farMinAround, t);
    hi = lerp(w->nearMaxAround, w->farMaxAround, t);
    if (hi - lo < 359) around = PF_CLAMP(around, lo, hi);
    lo = lerp(w->nearMinTilt, w->farMinTilt, t);
    hi = lerp(w->nearMaxTilt, w->farMaxTilt, t);
    tilt = PF_CLAMP(tilt, PF_MAX2(lo, -90), PF_MIN2(hi, 90));

    /* the transitions: the second picture comes in as the eye goes in */
    if (is_transition(w)) {
        pfGeoSet *g = first_gset(pfGetChild(w->group, 1), 0);
        if (g) {
            static float col[MAX_WORLDS][4];
            float *c = col[current];
            c[0] = c[1] = c[2] = 1;
            c[3] = 1 - t;
            g->attr[PFGS_COLOR4] = c;
            g->bind[PFGS_COLOR4] = PFGS_OVERALL;
            g->ilist[PFGS_COLOR4] = NULL;
            if (g->gstate)
                g->gstate->transparency = 1;
        }
    }

    /* the eye on the sphere of the radius, looking at the center */
    ca = (float)cos(PF_DEG2RAD(around));
    sa = (float)sin(PF_DEG2RAD(around));
    ct = (float)cos(PF_DEG2RAD(tilt));
    st = (float)sin(PF_DEG2RAD(tilt));
    pfSetVec3(eye->xyz, w->center[0] + radius * ct * ca, w->center[1] + radius * ct * sa,
              w->center[2] + radius * st);
    pfSetVec3(eye->hpr, around + 90, -tilt, 0);
    return 1;
}
