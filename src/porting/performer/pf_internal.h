/*
 * pf_internal.h - the objects of the Performer layer shared by its files
 */
#ifndef PF_INTERNAL_H
#define PF_INTERNAL_H

#include <Performer/pf.h>
#include <Performer/pfutil.h>
#include <Performer/pfui.h>
#include <Performer/pfdu.h>

#define PFI_MAX_PIPES 8
#define PFI_MAX_CHANS 64
#define PFI_STATS_FRAMES 120

/* the types of the objects for pfIsOfType/pfGetType (nodes are below 100) */
enum {
    PFI_TYPE_PIPE = 100, PFI_TYPE_PWIN, PFI_TYPE_WIN, PFI_TYPE_CHAN, PFI_TYPE_ESKY,
    PFI_TYPE_FOG, PFI_TYPE_FRUST, PFI_TYPE_FSTATS, PFI_TYPE_LIGHT, PFI_TYPE_LMODEL,
    PFI_TYPE_STRING, PFI_TYPE_LIST, PFI_TYPE_PVCHAN,
    PFI_TYPE_TRACKBALL = 200, PFI_TYPE_TRAVEL, PFI_TYPE_DRIVE, PFI_TYPE_FLY
};

struct pfWindow {
    int type;
    int glutwin;
};

struct pfPipe {
    int type;
    int id;
    int screen;
    pfPipeWindow *pwin;
};

struct pfPipeWindow {
    int type;
    pfPipe *pipe;
    char name[PF_MAXSTRING];
    int xo, yo, xs, ys;
    int glutwin;            /* 0: not opened */
    int index;
    pfPWinFuncType config;
    pfWindow win;
};

struct pfPipeVideoChannel {
    int type;
    int xs, ys, outxs, outys;
    int dvr;
};

struct pfFrameStats {
    int type;
    unsigned int classes;
    float update_secs;
    /* the times of the last frames, in seconds */
    float frame[PFI_STATS_FRAMES], app[PFI_STATS_FRAMES], cull[PFI_STATS_FRAMES],
        draw[PFI_STATS_FRAMES];
    int count, last;
    /* the averages shown, updated every update_secs */
    double avg_start;
    int avg_frames;
    double sum_frame, sum_app, sum_cull, sum_draw;
    float fps, ms_frame, ms_app, ms_cull, ms_draw;
    int gsets, prims, verts;
};

struct pfChannel {
    int type;
    int id;
    pfPipe *pipe;
    pfChannel *master;      /* of the group, itself when alone */
    unsigned int share;
    pfChanFuncType func[4];
    pfScene *scene;
    pfEarthSky *esky;
    float near_, far_;
    float fovh, fovv;
    int autoaspect;
    pfMatrix view;          /* the eye: position and orientation */
    pfVec3 offxyz, offhpr;
    float vl, vr, vb, vt;   /* the viewport in the window, fractions */
    int projmode;
    int travmode[4];
    float lodscale, lodfade, pixlimit;
    float stress[5];
    unsigned int statsmode;
    pfFrustum *cullptope;
    pfFrameStats fstats;
    pfPipeVideoChannel pvchan;
};

struct pfFog {
    int type;
    int fogtype;
    pfVec3 color;
    float onset, opaque;
};

struct pfEarthSky {
    int type;
    int mode;
    pfVec4 skytop, skybot, horiz, grndfar, grndnear, clear;
    float grndht, horizangle;
    pfFog *fog;
};

struct pfFrustum {
    int type;
    float near_, far_;
    float l, r, b, t;       /* at the near plane */
};

struct pfLightModel {
    int type;
    pfVec3 ambient;
    int local;
};

struct pfLight {
    int type;
    int index;              /* GL_LIGHT0 + index */
    pfVec4 pos;
    pfVec3 ambient, diffuse, specular;
};

struct pfString {
    int type;
    char *text;
    pfVec4 color;
    int justify;
    pfMatrix mat;
    pfBox bbox;
};

struct pfList {
    int type;
    int num, max;
    void **elts;
};

/* the runtime (pf_runtime.c) */
int pfiNumPipes(void);
int pfiNumChans(void);
pfChannel *pfiChan(int i);
/* the pixel rectangle of a channel in its window */
void pfiChanRect(pfChannel *c, int *x, int *y, int *w, int *h);
/* the OpenGL projection and modelview of a channel */
void pfiChanProjection(pfChannel *c);
void pfiChanModelview(pfChannel *c);
/* hooks of libpfutil: the drawing of the GUI at the end of each window, the
 * input after each frame */
void pfiSetWinHook(void (*draw)(pfPipeWindow *pw));
double pfiFrameStart(void);

/* libpfutil (pfu_input.c, pfu_gui.c) */
void pfuiWinCreated(pfPipeWindow *pw);
/* the mouse as it is, without taking its events */
void pfuiPeekMouse(pfuMouse *mouse);
void pfuiTextWidth(const char *s, int big, int *w);
void pfuiDrawText(float x, float y, const char *s, int big);
/* the bounding sphere of a graph under a matrix (pf_scene.c) */
int pfiNodeBound(pfNode *n, pfSphere *s);

#endif
