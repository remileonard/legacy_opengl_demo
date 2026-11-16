/*
 * pfi_xformer.c - the pfiTDFXformer of libpfui (IRIS Performer 2.0): the
 * trackball, drive and fly motion models of the samples, driven by the
 * pfuMouse of the master channel.
 *
 *  trackball  left: rotate the scene, middle (or shift + left): pan,
 *             right (or ctrl + left, or left + middle): zoom
 *  drive      left: faster, right: slower or backward, the mouse across
 *             the channel turns; middle (or shift + left): height
 *  fly        left: faster, right: slower or backward, the mouse across
 *             and up/down turns and climbs; middle (or shift + left):
 *             steer at the same speed
 *
 * The mouse acts only when its button was pressed in the master channel
 * (not in the GUI). The scene and the speeds are scaled on the bounding
 * sphere of the node of the xformer. The collisions stop the eye in front
 * of the geometry; the drive follows the ground.
 */
#include "pf_internal.h"

#include <math.h>

struct pfiInputXform {
    int type;
};

struct pfiXformer {
    int type;
    int model;
    struct pfiInputXform tb, drive, fly;
    pfChannel *chan;
    pfuMouse *mouse;
    pfuEventStream *events;
    pfNode *node;
    pfDCS *dcs;
    pfCoord reset, coord, prev;
    float speed;
    int collide;
    pfSphere bs;
    double last_t;
    int buttons, inside;
    int lastx, lasty;
    /* trackball: rotation around the center of the scene, translation */
    float tbh, tbp;
    pfVec3 tbtrans;
    float drive_height;
};

void pfiInit(void) {}

pfType pfiGetIXformTravelClassType(void) { return PFI_TYPE_TRAVEL; }
pfType pfiGetIXformTrackballClassType(void) { return PFI_TYPE_TRACKBALL; }

pfiTDFXformer *pfiNewTDFXformer(void *arena)
{
    pfiXformer *x = (pfiXformer *)pfMalloc(sizeof(pfiXformer), arena);
    x->type = PFI_TYPE_TRACKBALL;
    x->tb.type = PFI_TYPE_TRACKBALL;
    /* drive and fly are travel models */
    x->drive.type = PFI_TYPE_TRAVEL;
    x->fly.type = PFI_TYPE_TRAVEL;
    x->model = PFITDF_TRACKBALL;
    x->bs.radius = 100;
    return x;
}

void pfiXformerAutoInput(pfiXformer *x, pfChannel *chan, pfuMouse *mouse, pfuEventStream *events)
{
    x->chan = chan;
    x->mouse = mouse;
    x->events = events;
}

void pfiXformerNode(pfiXformer *x, pfNode *node)
{
    x->node = node;
    pfGetNodeBSphere(node, &x->bs);
    if (x->bs.radius <= 0)
        x->bs.radius = 100;
}

void pfiXformerAutoPosition(pfiXformer *x, pfChannel *chan, pfDCS *dcs)
{
    (void)chan;
    x->dcs = dcs;
}

void pfiXformerResetCoord(pfiXformer *x, pfCoord *coord)
{
    x->reset = *coord;
    x->coord = *coord;
    x->prev = *coord;
}

void pfiXformerCoord(pfiXformer *x, pfCoord *coord)
{
    x->coord = *coord;
    x->prev = *coord;
}

static void update_dcs(pfiXformer *x)
{
    pfMatrix m, r;
    const float *c = x->bs.center;
    if (!x->dcs)
        return;
    /* rotation around the center, then the translation */
    pfMakeTransMat(m, -c[0], -c[1], -c[2]);
    pfMakeEulerMat(r, x->tbh, x->tbp, 0);
    pfPostMultMat(m, r);
    pfPostTransMat(m, m, c[0] + x->tbtrans[0], c[1] + x->tbtrans[1], c[2] + x->tbtrans[2]);
    pfDCSMat(x->dcs, m);
}

void pfiSelectXformerModel(pfiXformer *x, int model)
{
    if (model < 0 || model >= PFITDF_NUM_MODELS)
        return;
    if (model != x->model)
        x->speed = 0;
    x->model = model;
    x->type = model == PFITDF_TRACKBALL ? PFI_TYPE_TRACKBALL : PFI_TYPE_TRAVEL;
    if (model == PFITDF_DRIVE) {
        x->coord.hpr[1] = 0;
        x->coord.hpr[2] = 0;
        x->drive_height = -1;
    }
}

int pfiGetXformerCurModelIndex(pfiXformer *x) { return x->model; }

pfiInputXform *pfiGetXformerCurModel(pfiXformer *x)
{
    return x->model == PFITDF_TRACKBALL ? &x->tb : x->model == PFITDF_DRIVE ? &x->drive : &x->fly;
}

void pfiStopXformer(pfiXformer *x)
{
    x->speed = 0;
}

void pfiResetXformerPosition(pfiXformer *x)
{
    x->coord = x->reset;
    x->prev = x->reset;
    x->speed = 0;
    x->tbh = x->tbp = 0;
    pfSetVec3(x->tbtrans, 0, 0, 0);
    x->drive_height = -1;
    if (x->model == PFITDF_DRIVE)
        x->coord.hpr[1] = x->coord.hpr[2] = 0;
    update_dcs(x);
}

void pfiCenterXformer(pfiXformer *x)
{
    if (x->node)
        pfGetNodeBSphere(x->node, &x->bs);
    x->speed = 0;
    if (x->model == PFITDF_TRACKBALL) {
        x->tbh = x->tbp = 0;
        pfSetVec3(x->tbtrans, 0, 0, 0);
        update_dcs(x);
    } else {
        pfCopyVec3(x->coord.xyz, x->bs.center);
        x->prev = x->coord;
        x->drive_height = -1;
    }
}

void pfiGetXformerCoord(pfiXformer *x, pfCoord *coord) { *coord = x->coord; }

void pfiGetXformerMat(pfiXformer *x, pfMatrix m)
{
    pfMakeCoordMat(m, x->model == PFITDF_TRACKBALL ? &x->reset : &x->coord);
}

void pfiEnableXformerCollision(pfiXformer *x) { x->collide = 1; }
void pfiDisableXformerCollision(pfiXformer *x) { x->collide = 0; }

/* the first hit of a segment, its distance and normal */
static int isect(pfiXformer *x, const pfVec3 from, const pfVec3 dir, float len, float *d, pfVec3 nrm)
{
    pfSegSet ss;
    pfHit **hits[PFIS_MAX_SEGS];
    pfVec3 p;
    if (!x->node || len <= 0)
        return 0;
    memset(&ss, 0, sizeof(ss));
    ss.mode = PFTRAV_IS_PRIM;
    ss.activeMask = 1;
    ss.isectMask = 0xffffffffU;
    pfCopyVec3(ss.segs[0].pos, from);
    pfCopyVec3(ss.segs[0].dir, dir);
    ss.segs[0].length = len;
    if (pfNodeIsectSegs(x->node, &ss, hits) <= 0)
        return 0;
    pfQueryHit(*hits[0], PFQHIT_POINT, p);
    pfQueryHit(*hits[0], PFQHIT_NORM, nrm);
    *d = pfDistancePt3(from, p);
    return 1;
}

/* the eye in front of the geometry, the drive at its height over the ground */
int pfiCollideXformer(pfiXformer *x)
{
    pfVec3 dir, nrm;
    float len, d, margin = PF_MAX2(0.05f, x->bs.radius * 0.0005f);
    int hit = 0;
    if (!x->collide || x->model == PFITDF_TRACKBALL || !x->node)
        return 0;
    pfSubVec3(dir, x->coord.xyz, x->prev.xyz);
    len = pfNormalizeVec3(dir);
    if (len > 0 && isect(x, x->prev.xyz, dir, len + margin, &d, nrm)) {
        float back = d - margin;
        if (back < 0)
            back = 0;
        pfCombineVec3(x->coord.xyz, 1, x->prev.xyz, back, dir);
        x->speed = 0;
        hit = 1;
    }
    if (x->model == PFITDF_DRIVE) {
        pfVec3 top, down;
        float up = x->bs.radius * 2;
        pfSetVec3(down, 0, 0, -1);
        pfCopyVec3(top, x->coord.xyz);
        top[2] += x->drive_height > 0 ? x->drive_height * 0.9f : 0;
        if (isect(x, top, down, up * 2, &d, nrm)) {
            float ground = top[2] - d;
            if (x->drive_height < 0)
                x->drive_height = PF_MAX2(x->coord.xyz[2] - ground, 0.5f);
            x->coord.xyz[2] = ground + x->drive_height;
        }
    }
    x->prev = x->coord;
    return hit;
}

void pfiUpdateXformer(pfiXformer *x)
{
    pfuMouse *m = x->mouse;
    double now = pfGetTime();
    float dt = x->last_t > 0 ? (float)(now - x->last_t) : 0;
    int cx = 0, cy = 0, cw = 1, ch = 1, buttons, left, middle, right, shift, ctrl;
    float nx = 0, ny = 0, size = x->bs.radius * 2;

    x->last_t = now;
    if (dt > 0.25f)
        dt = 0.25f;
    if (!x->collide)
        x->prev = x->coord;
    if (!m)
        return;
    if (x->chan)
        pfiChanRect(x->chan, &cx, &cy, &cw, &ch);
    buttons = m->flags & PFUDEV_MOUSE_DOWN_MASK;
    /* a press counts when it was made in the channel, out of the GUI */
    if (buttons && !x->buttons)
        x->inside = m->xclick >= cx && m->xclick < cx + cw && m->yclick >= cy &&
                    m->yclick < cy + ch && !pfuInGUI(m->xclick, m->yclick);
    if (!buttons)
        x->inside = 0;
    if (buttons && !x->buttons) {
        /* the drag starts where the button was pressed */
        x->lastx = m->xclick;
        x->lasty = m->yclick;
    } else if (!buttons) {
        x->lastx = m->xpos;
        x->lasty = m->ypos;
    }
    x->buttons = buttons;
    if (!x->inside)
        buttons = 0;
    left = buttons & PFUDEV_MOUSE_LEFT_DOWN;
    middle = buttons & PFUDEV_MOUSE_MIDDLE_DOWN;
    right = buttons & PFUDEV_MOUSE_RIGHT_DOWN;
    shift = m->modifiers & PFUDEV_MOD_SHIFT;
    ctrl = m->modifiers & PFUDEV_MOD_CTRL;
    if (left && shift) { left = 0; middle = 1; }
    if (left && ctrl) { left = 0; right = 1; }
    nx = (m->xpos - (cx + cw / 2.0f)) / (cw / 2.0f);
    ny = (m->ypos - (cy + ch / 2.0f)) / (ch / 2.0f);
    nx = PF_CLAMP(nx, -1, 1);
    ny = PF_CLAMP(ny, -1, 1);

    if (x->model == PFITDF_TRACKBALL) {
        float dx = (float)(m->xpos - x->lastx) / cw, dy = (float)(m->ypos - x->lasty) / ch;
        if (left && middle) { left = middle = 0; right = 1; }
        if (left) {
            x->tbh += dx * 180;
            x->tbp -= dy * 180;
        } else if (middle) {
            x->tbtrans[0] += dx * size;
            x->tbtrans[2] += dy * size;
        } else if (right) {
            x->tbtrans[1] += dy * size * 2;
        }
        if (left || middle || right)
            update_dcs(x);
        x->lastx = m->xpos;
        x->lasty = m->ypos;
        return;
    }

    {
        /* the speeds follow the size of the scene: 1/50 of its radius per
         * second in flight, 1/200 on the road, reached in 4 seconds */
        float maxspeed = PF_MAX2(1.0f, x->bs.radius * (x->model == PFITDF_FLY ? 0.02f : 0.005f));
        float accel = maxspeed / 4;
        float dead = 0.05f, sx = fabs(nx) < dead ? 0 : nx, sy = fabs(ny) < dead ? 0 : ny;
        pfMatrix rot;
        pfVec3 fwd, y = { 0, 1, 0 };
        if (left)
            x->speed += accel * dt;
        if (right)
            x->speed -= accel * dt;
        x->speed = PF_CLAMP(x->speed, -maxspeed, maxspeed);
        if (left || middle || right) {
            x->coord.hpr[0] -= sx * 90 * dt;
            if (x->model == PFITDF_FLY) {
                x->coord.hpr[1] += sy * 60 * dt;
                x->coord.hpr[1] = PF_CLAMP(x->coord.hpr[1], -89, 89);
            } else if (middle) {
                /* the height of the drive */
                float dz = sy * maxspeed * 0.25f * dt;
                x->coord.xyz[2] += dz;
                if (x->drive_height > 0)
                    x->drive_height = PF_MAX2(0.1f, x->drive_height + dz);
            }
        }
        if (x->coord.hpr[0] > 180) x->coord.hpr[0] -= 360;
        if (x->coord.hpr[0] < -180) x->coord.hpr[0] += 360;
        if (x->model == PFITDF_DRIVE)
            x->coord.hpr[1] = x->coord.hpr[2] = 0;
        pfMakeEulerMat(rot, x->coord.hpr[0], x->coord.hpr[1], x->coord.hpr[2]);
        pfXformVec3(fwd, y, rot);
        pfCombineVec3(x->coord.xyz, 1, x->coord.xyz, x->speed * dt, fwd);
    }
}
