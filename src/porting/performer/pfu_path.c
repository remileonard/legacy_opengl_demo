/*
 * pfu_path.c - the paths and the vehicles of the later perfly
 * (perfly -A model.flt,file.path,Name): models moving along the paths of
 * .path files, which the eye can follow.
 *
 * A .path file holds these lines (# starts a comment):
 *
 *   speed s rate      the speed, in units per second, and its rate of
 *                     change (a negative speed runs the path backward)
 *   line x1 y1 z1 x2 y2 z2
 *                     a straight segment
 *   fillet r          rounds the corner between the line before and the
 *                     line after with an arc of radius r
 *   arc cx cy cz r a0 a1
 *                     an arc of center c and radius r, from angle a0 to
 *                     a1 (degrees, counterclockwise from +X)
 *   close             the path loops
 *
 * The models face +X: they are turned along the direction of their motion.
 *
 * Keys, taken here before perfly sees them: v follows the next vehicle
 * (then none), V switches between the chase view and the view on board.
 */
#include "pf_internal.h"

#include <ctype.h>
#include <math.h>

#define MAX_SEGS 256
#define MAX_VEHICLES 16

typedef struct {
    int arc;
    pfVec3 p0, p1;              /* line: from p0 to p1 */
    pfVec3 center;              /* arc: center (z of the start), radius */
    float radius, a0, sweep;    /* degrees */
    float z0, z1;
    float len;
} PathSeg;

typedef struct {
    PathSeg seg[MAX_SEGS];
    int nseg;
    float total;
    float speed, rate;
    int closed;
} Path;

typedef struct {
    char model[256], pathfile[256], name[64];
    Path path;
    pfDCS *dcs;
    pfSphere bs;
    float dist, speed;
    pfVec3 pos, dir;
    int ok;
} Vehicle;

static Vehicle vehicles[MAX_VEHICLES];
static int nvehicles, attached;
static int follow = -1, onboard;
/* the motion model under the vehicles, and the Motion menu of the GUI */
static int base_model;
static pfuWidget *motion;

#define PFUI_FOLLOW 100     /* + vehicle: the chase view */
#define PFUI_RIDE 200       /* + vehicle: the view on board */
static double last_update = -1;

/* --- paths ---------------------------------------------------------------- */

static void add_line(Path *p, const pfVec3 a, const pfVec3 b)
{
    PathSeg *s;
    if (p->nseg == MAX_SEGS)
        return;
    s = &p->seg[p->nseg++];
    memset(s, 0, sizeof(*s));
    pfCopyVec3(s->p0, a);
    pfCopyVec3(s->p1, b);
    s->len = pfDistancePt3(a, b);
}

static void add_arc(Path *p, const pfVec3 c, float r, float a0, float sweep, float z0, float z1)
{
    PathSeg *s;
    if (p->nseg == MAX_SEGS)
        return;
    s = &p->seg[p->nseg++];
    memset(s, 0, sizeof(*s));
    s->arc = 1;
    pfCopyVec3(s->center, c);
    s->radius = r;
    s->a0 = a0;
    s->sweep = sweep;
    s->z0 = z0;
    s->z1 = z1;
    s->len = (float)sqrt(PF_SQUARE(r * PF_DEG2RAD(fabs(sweep))) + PF_SQUARE(z1 - z0));
}

/* rounds the corner between the last line of the path and the line a-b:
 * the line is trimmed, the arc added, and a-b given back trimmed */
static void fillet(Path *p, float r, pfVec3 a, const pfVec3 b)
{
    PathSeg *l = p->nseg ? &p->seg[p->nseg - 1] : NULL;
    float d1[2], d2[2], l1, l2, cosang, phi, t, cross, a0, ang;
    pfVec3 c, s0, s1;
    if (!l || l->arc || r <= 0)
        return;
    d1[0] = l->p1[0] - l->p0[0]; d1[1] = l->p1[1] - l->p0[1];
    d2[0] = b[0] - a[0]; d2[1] = b[1] - a[1];
    l1 = (float)sqrt(d1[0] * d1[0] + d1[1] * d1[1]);
    l2 = (float)sqrt(d2[0] * d2[0] + d2[1] * d2[1]);
    if (l1 < 1e-6f || l2 < 1e-6f)
        return;
    d1[0] /= l1; d1[1] /= l1;
    d2[0] /= l2; d2[1] /= l2;
    cosang = d1[0] * d2[0] + d1[1] * d2[1];
    cross = d1[0] * d2[1] - d1[1] * d2[0];
    phi = (float)acos(PF_CLAMP(cosang, -1, 1));
    if (phi < 1e-4f || phi > 3.1415f)
        return;
    t = r * (float)tan(phi / 2);
    /* a corner too short for the radius takes what it can */
    if (t > 0.5f * l1 || t > 0.5f * l2) {
        t = PF_MIN2(0.5f * l1, 0.5f * l2);
        r = t / (float)tan(phi / 2);
    }
    /* the ends of the arc on the two lines */
    pfSetVec3(s0, l->p1[0] - d1[0] * t, l->p1[1] - d1[1] * t, l->p1[2]);
    pfSetVec3(s1, a[0] + d2[0] * t, a[1] + d2[1] * t, a[2]);
    l->p1[0] = s0[0];
    l->p1[1] = s0[1];
    l->len = pfDistancePt3(l->p0, l->p1);
    a[0] = s1[0];
    a[1] = s1[1];
    /* the center, on the side of the turn */
    if (cross > 0)
        pfSetVec3(c, s0[0] - d1[1] * r, s0[1] + d1[0] * r, s0[2]);
    else
        pfSetVec3(c, s0[0] + d1[1] * r, s0[1] - d1[0] * r, s0[2]);
    a0 = (float)PF_RAD2DEG(atan2(s0[1] - c[1], s0[0] - c[0]));
    ang = (float)PF_RAD2DEG(phi);
    add_arc(p, c, r, a0, cross > 0 ? ang : -ang, s0[2], s1[2]);
}

static int load_path(Path *p, const char *file)
{
    char path[4096], line[512];
    FILE *fp;
    float pending = 0;
    memset(p, 0, sizeof(*p));
    p->speed = 10;
    p->rate = 1000;
    if (!pfFindFile(file, path, R_OK) || (fp = fopen(path, "r")) == NULL) {
        pfNotify(PFNFY_WARN, PFNFY_RESOURCE, "path %s not found", file);
        return 0;
    }
    while (fgets(line, sizeof(line), fp)) {
        char *s = line, cmd[32];
        float v[8];
        while (isspace((unsigned char)*s))
            s++;
        if (*s == '#' || !*s || sscanf(s, "%31s", cmd) != 1)
            continue;
        s += strlen(cmd);
        if (strcmp(cmd, "speed") == 0) {
            if (sscanf(s, "%f %f", &v[0], &v[1]) >= 1) {
                p->speed = v[0];
                if (sscanf(s, "%f %f", &v[0], &v[1]) == 2)
                    p->rate = v[1];
            }
        } else if (strcmp(cmd, "line") == 0 &&
                   sscanf(s, "%f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) == 6) {
            pfVec3 a, b;
            pfSetVec3(a, v[0], v[1], v[2]);
            pfSetVec3(b, v[3], v[4], v[5]);
            if (pending > 0)
                fillet(p, pending, a, b);
            pending = 0;
            add_line(p, a, b);
        } else if (strcmp(cmd, "fillet") == 0 && sscanf(s, "%f", &v[0]) == 1) {
            pending = v[0];
        } else if (strcmp(cmd, "arc") == 0 &&
                   sscanf(s, "%f %f %f %f %f %f", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) == 6) {
            pfVec3 c;
            pfSetVec3(c, v[0], v[1], v[2]);
            add_arc(p, c, v[3], v[4], v[5] - v[4], v[2], v[2]);
            pending = 0;
        } else if (strcmp(cmd, "close") == 0) {
            p->closed = 1;
        }
    }
    fclose(fp);
    if (p->closed && p->nseg > 1) {
        /* back to the start */
        PathSeg *last = &p->seg[p->nseg - 1], *first = &p->seg[0];
        pfVec3 e, st;
        if (last->arc) {
            float a = (float)PF_DEG2RAD(last->a0 + last->sweep);
            pfSetVec3(e, last->center[0] + last->radius * (float)cos(a),
                      last->center[1] + last->radius * (float)sin(a), last->z1);
        } else {
            pfCopyVec3(e, last->p1);
        }
        if (first->arc) {
            float a = (float)PF_DEG2RAD(first->a0);
            pfSetVec3(st, first->center[0] + first->radius * (float)cos(a),
                      first->center[1] + first->radius * (float)sin(a), first->z0);
        } else {
            pfCopyVec3(st, first->p0);
        }
        if (pfDistancePt3(e, st) > 1e-3f)
            add_line(p, e, st);
    }
    {
        int i;
        for (i = 0; i < p->nseg; i++)
            p->total += p->seg[i].len;
    }
    return p->nseg > 0 && p->total > 0;
}

/* the point and the direction at a distance along the path */
static void path_point(const Path *p, float d, pfVec3 pos, pfVec3 dir)
{
    int i;
    if (p->closed) {
        d = (float)fmod(d, p->total);
        if (d < 0)
            d += p->total;
    } else {
        d = PF_CLAMP(d, 0, p->total);
    }
    for (i = 0; i < p->nseg - 1 && d > p->seg[i].len; i++)
        d -= p->seg[i].len;
    {
        const PathSeg *s = &p->seg[i];
        float t = s->len > 0 ? PF_CLAMP(d / s->len, 0, 1) : 0;
        if (s->arc) {
            float a = (float)PF_DEG2RAD(s->a0 + s->sweep * t), sg = s->sweep > 0 ? 1.0f : -1.0f;
            pfSetVec3(pos, s->center[0] + s->radius * (float)cos(a),
                      s->center[1] + s->radius * (float)sin(a), s->z0 + (s->z1 - s->z0) * t);
            pfSetVec3(dir, -(float)sin(a) * sg * s->radius * (float)PF_DEG2RAD(fabs(s->sweep)),
                      (float)cos(a) * sg * s->radius * (float)PF_DEG2RAD(fabs(s->sweep)), s->z1 - s->z0);
        } else {
            pfCombineVec3(pos, 1 - t, s->p0, t, s->p1);
            pfSubVec3(dir, s->p1, s->p0);
        }
        pfNormalizeVec3(dir);
    }
}

/* --- vehicles --------------------------------------------------------------- */

int pfuiAddVehicle(const char *model, const char *pathfile, const char *name)
{
    Vehicle *v;
    if (nvehicles == MAX_VEHICLES)
        return -1;
    v = &vehicles[nvehicles];
    memset(v, 0, sizeof(*v));
    strncpy(v->model, model, sizeof(v->model) - 1);
    strncpy(v->pathfile, pathfile, sizeof(v->pathfile) - 1);
    strncpy(v->name, name && name[0] ? name : model, sizeof(v->name) - 1);
    return nvehicles++;
}

int pfuiNumVehicles(void) { return nvehicles; }

/* the matrix of a model facing +X along a direction */
static void vehicle_mat(pfMatrix m, const pfVec3 pos, const pfVec3 dir)
{
    pfVec3 x, y, z, up;
    pfCopyVec3(x, dir);
    if (pfNormalizeVec3(x) < 1e-6f)
        pfSetVec3(x, 1, 0, 0);
    pfSetVec3(up, 0, 0, 1);
    pfCrossVec3(y, up, x);
    if (pfNormalizeVec3(y) < 1e-6f)
        pfSetVec3(y, 0, 1, 0);
    pfCrossVec3(z, x, y);
    pfMakeIdentMat(m);
    pfCopyVec3(m[0], x);
    pfCopyVec3(m[1], y);
    pfCopyVec3(m[2], z);
    pfCopyVec3(m[3], pos);
}

/* the models are loaded and put in the scene with the first channel scene */
void pfuiAttachVehicles(pfScene *scene)
{
    int i;
    if (attached || !scene)
        return;
    attached = 1;
    for (i = 0; i < nvehicles; i++) {
        Vehicle *v = &vehicles[i];
        pfNode *model;
        pfdInitConverter(v->model);
        if (!load_path(&v->path, v->pathfile))
            continue;
        if ((model = pfdLoadFile(v->model)) == NULL) {
            pfNotify(PFNFY_WARN, PFNFY_RESOURCE, "vehicle %s: cannot load %s", v->name, v->model);
            continue;
        }
        pfGetNodeBSphere(model, &v->bs);
        if (v->bs.radius <= 0)
            v->bs.radius = 1;
        v->dcs = pfNewDCS();
        pfAddChild(v->dcs, model);
        pfAddChild(scene, v->dcs);
        v->speed = v->path.rate >= 1000 ? v->path.speed : 0;
        v->ok = 1;
        pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "vehicle %s: %s on %s, %.0f long, %.1f per second",
                 v->name, v->model, v->pathfile, v->path.total, v->path.speed);
    }
    if (nvehicles)
        pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "v: follow the next vehicle, V: chase or on board view");
}

/* the vehicles at the time of the frame */
void pfuiUpdateVehicles(void)
{
    double now = pfGetTime();
    float dt = last_update >= 0 ? (float)(now - last_update) : 0;
    int i;
    if (now == last_update)
        return;
    last_update = now;
    if (dt > 0.25f)
        dt = 0.25f;
    for (i = 0; i < nvehicles; i++) {
        Vehicle *v = &vehicles[i];
        pfMatrix m;
        pfVec3 dir;
        float target = v->path.speed, step = v->path.rate * dt;
        if (!v->ok)
            continue;
        /* the speed goes to its target at its rate */
        if (v->speed < target) v->speed = PF_MIN2(target, v->speed + step);
        else if (v->speed > target) v->speed = PF_MAX2(target, v->speed - step);
        v->dist += v->speed * dt;
        path_point(&v->path, v->dist, v->pos, dir);
        /* the model faces where it goes */
        if (v->speed < 0)
            pfScaleVec3(dir, -1, dir);
        pfCopyVec3(v->dir, dir);
        vehicle_mat(m, v->pos, dir);
        pfDCSMat(v->dcs, m);
    }
}

/* --- following --------------------------------------------------------------- */

int pfuiFollowedVehicle(void)
{
    return follow >= 0 && follow < nvehicles && vehicles[follow].ok ? follow : -1;
}

/* the eye that follows a vehicle: behind and above it, or on board */
int pfuiFollowEye(pfCoord *eye)
{
    Vehicle *v;
    pfVec3 dir, look, horiz;
    float r, hd;
    if (pfuiFollowedVehicle() < 0)
        return 0;
    v = &vehicles[follow];
    pfuiUpdateVehicles();
    r = v->bs.radius;
    pfCopyVec3(dir, v->dir);
    pfSetVec3(horiz, dir[0], dir[1], 0);
    if (pfNormalizeVec3(horiz) < 1e-6f)
        pfSetVec3(horiz, 1, 0, 0);
    if (onboard) {
        /* above the center of the model, looking ahead */
        pfSetVec3(eye->xyz, v->pos[0] + horiz[0] * r * 0.2f, v->pos[1] + horiz[1] * r * 0.2f,
                  v->bs.center[2] + v->pos[2] + r * 0.35f);
        pfCopyVec3(look, dir);
    } else {
        /* behind, above, looking at the model */
        pfSetVec3(eye->xyz, v->pos[0] - horiz[0] * r * 2.5f, v->pos[1] - horiz[1] * r * 2.5f,
                  v->pos[2] + v->bs.center[2] + r * 0.9f);
        pfSetVec3(look, v->pos[0] - eye->xyz[0], v->pos[1] - eye->xyz[1],
                  v->pos[2] + v->bs.center[2] - eye->xyz[2]);
    }
    hd = (float)sqrt(look[0] * look[0] + look[1] * look[1]);
    /* the eye looks along +Y at heading 0 */
    eye->hpr[0] = (float)PF_RAD2DEG(atan2(-look[0], look[1]));
    eye->hpr[1] = (float)PF_RAD2DEG(atan2(look[2], hd));
    eye->hpr[2] = 0;
    return 1;
}

/* v and V; 1 when the key was taken */
/* the motion models numbered past the 3 of perfly: the vehicles */
void pfuiSelectMotion(int model)
{
    int f = -1, ob = onboard;
    if (model >= PFUI_RIDE && model - PFUI_RIDE < nvehicles) {
        f = model - PFUI_RIDE;
        ob = 1;
    } else if (model >= PFUI_FOLLOW && model - PFUI_FOLLOW < nvehicles) {
        f = model - PFUI_FOLLOW;
        ob = 0;
    } else {
        base_model = model;
    }
    if (f >= 0 && !vehicles[f].ok)
        f = -1;
    if (f != follow || (f >= 0 && ob != onboard)) {
        if (f >= 0)
            pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "following %s (%s view)", vehicles[f].name,
                     ob ? "on board" : "chase");
        else if (follow >= 0)
            pfNotify(PFNFY_NOTICE, PFNFY_PRINT, "following nothing");
    }
    follow = f;
    onboard = ob;
}

/* the index of the motion model with the vehicles, -1: none followed */
int pfuiFollowModel(void)
{
    if (pfuiFollowedVehicle() < 0)
        return -1;
    return (onboard ? PFUI_RIDE : PFUI_FOLLOW) + follow;
}

/* the Motion menu of perfly gets an entry to follow and one to ride each
 * vehicle; strs and vals are given back extended, n their count */
int pfuiVehicleMenu(pfuWidget *w, pfuGUIString **strs, int **vals, int n)
{
    static pfuGUIString s[64];
    static int v[64];
    static char names[2 * MAX_VEHICLES][80];
    int i, k = 0;
    if (!nvehicles || n != 3 || n + 2 * nvehicles > 64)
        return n;
    for (i = 0; i < n; i++) {
        s[i] = (*strs)[i];
        v[i] = *vals ? (*vals)[i] : i;
    }
    for (i = 0; i < nvehicles; i++) {
        if (!vehicles[i].ok)
            continue;
        sprintf(names[k], "Follow %.60s", vehicles[i].name);
        s[n] = names[k++];
        v[n++] = PFUI_FOLLOW + i;
        sprintf(names[k], "Ride %.60s", vehicles[i].name);
        s[n] = names[k++];
        v[n++] = PFUI_RIDE + i;
    }
    *strs = s;
    *vals = v;
    motion = w;
    return n;
}

/* v and V; 1 when the key was taken */
int pfuiVehicleKey(int key)
{
    int i, f = follow, ob = onboard, model;
    if (!nvehicles || (key != 'v' && key != 'V'))
        return 0;
    if (key == 'V') {
        ob = !ob;
    } else {
        /* the next vehicle that moves, then none */
        for (i = 0; i <= nvehicles; i++) {
            f = f + 1 >= nvehicles ? -1 : f + 1;
            if (f < 0 || vehicles[f].ok)
                break;
        }
    }
    model = f < 0 ? base_model : (ob ? PFUI_RIDE : PFUI_FOLLOW) + f;
    if (f < 0)
        onboard = ob;
    if (motion) {
        /* through the menu of perfly, which changes the motion model */
        pfuWidgetValue(motion, (float)model);
        pfuGetWidgetActionFunc(motion)(motion);
    } else {
        pfuiSelectMotion(model);
    }
    return 1;
}

/* the name of the followed vehicle at the bottom of the master channel */
void pfuiDrawVehicleName(pfChannel *chan)
{
    char buf[128];
    int k = pfuiFollowedVehicle();
    if (k < 0 || !chan)
        return;
    sprintf(buf, "%s - %s  (v: next, V: view)", vehicles[k].name, onboard ? "on board" : "chase");
    pfuDrawMessage(chan, buf, PFU_MSG_CHAN, PFU_CENTER_JUSTIFIED, 0.5f, 0.04f, PFU_FONT_SMALL, PFU_RGB);
}
