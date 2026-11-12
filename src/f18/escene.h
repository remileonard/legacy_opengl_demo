/*
 * escene.h - the subset of Coryphaeus EasyScene used by the F-18 demo
 *
 * EasyScene runs an application from a script (.set): the search path of
 * the files, the pipes (windows) and channels (views), the objects loaded
 * from model files, the keys bound to functions (qdev), and the modules:
 * functions of the application called at the phases of the frame (preinit,
 * postinit, preapp, postapp, predraw, postdraw...).
 *
 * The functions named by a script are found by name, as the original did
 * with the symbols of the executable: the application registers them with
 * esRegisterFunction before loading the script.  A missing function is
 * reported once, and the script goes on.
 */
#ifndef ESCENE_H
#define ESCENE_H

#include "pf.h"

/* the phases of the modules, in the order of a frame */
enum {
    ES_PREINIT, ES_INIT, ES_POSTINIT,
    ES_PREAPP, ES_POSTAPP,
    ES_PRECULL, ES_POSTCULL,
    ES_PREDRAW, ES_POSTDRAW, ES_POSTRENDER,
    ES_EXIT,
    ES_NPHASES
};

/* a function of the application: the argument is the one of userCallback,
 * NULL for the modules and the keys */
typedef void (*esFunc)(const char *arg);

typedef struct esObject {
    char *name;
    char *file;             /* model file */
    char *isect_file;       /* isectGeom: the geometry of the intersections */
    unsigned int isect_mask;
    int is_static;          /* loadstatic: no DCS */
    int is_terrain;         /* type terrain */
    pfNode *model;          /* the loaded model */
    pfNode *node;           /* its DCS (or group for static objects) in the scene */
    pfVec3 xyz, hpr;        /* position and orientation of the DCS */
    float radius;           /* bounding sphere of the model */
    pfVec3 center;
} esObject;

typedef struct esPipe {
    char *name;
    int screen;
    int viewport[4];        /* xmin xmax ymin ymax */
} esPipe;

typedef struct esChannel {
    char *name;
    esPipe *pipe;
    float viewport[4];      /* left right bottom top, fractions of the pipe */
    float near_clip, far_clip;
    float fov_h, fov_v;     /* degrees, -1 computed from the aspect */
    int fade_lod;
    esObject *attach;       /* the eye follows this object */
    pfVec3 viewpos, viewrot; /* offset of the eye on the object */
} esChannel;

typedef struct esKeyBinding {
    char *device;           /* IRIS GL device name: "WKEY", "UPARROWKEY"... */
    char *function;
    esFunc func;
    int autorepeat;
} esKeyBinding;

typedef struct esScene {
    char *script;
    char *path;             /* resolved search path, ':' separated */
    float framerate;
    pfScene *root;
    esPipe *pipes;
    int npipes;
    esChannel *channels;
    int nchannels;
    esObject **objects;
    int nobjects;
    esKeyBinding *keys;
    int nkeys;
    int configured;
} esScene;

extern esScene es;

/* the functions the scripts can name */
void esRegisterFunction(const char *name, esFunc f);
esFunc esFindFunction(const char *name);

/* symbols for %ifdef */
void esDefine(const char *name);

/* reads and runs a script, 0 on error; configure_cb is called at the
 * "configure" statement (the pipes are known: the window can be opened) */
int esLoadScript(const char *file, void (*configure_cb)(void));

/* calls the modules of a phase */
void esRunPhase(int phase);

esObject *esFindObject(const char *name);
esChannel *esFindChannel(const char *name);

/* places an object */
void esObjectPos(esObject *o, float x, float y, float z, float h, float p, float r);

/* the viewing matrix of a channel, for glLoadMatrixf: Performer looks along
 * +Y with Z up */
void esChannelViewMatrix(const esChannel *c, float m[16]);

/* the binding of an IRIS GL device name, NULL if none */
esKeyBinding *esFindKey(const char *device);

#endif
