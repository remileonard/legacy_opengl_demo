/*
 * Performer/pfui.h - the libpfui of IRIS Performer 2.0 used by the sample
 * programs: the pfiTDFXformer, the motion models trackball, drive and fly
 * driven by the mouse, with collisions against the scene.
 */
#ifndef PERFORMER_PFUI_H
#define PERFORMER_PFUI_H

#include <Performer/pf.h>
#include <Performer/pfutil.h>

/* the motion models of the TDF xformer, the index of the current model */
#define PFITDF_TRACKBALL 0
#define PFITDF_DRIVE 1
#define PFITDF_FLY 2
#define PFITDF_NUM_MODELS 3

typedef struct pfiXformer pfiXformer;
typedef pfiXformer pfiTDFXformer;
typedef struct pfiInputXform pfiInputXform;

void pfiInit(void);

pfiTDFXformer *pfiNewTDFXformer(void *arena);
void pfiXformerAutoInput(pfiXformer *x, pfChannel *chan, pfuMouse *mouse, pfuEventStream *events);
void pfiXformerNode(pfiXformer *x, pfNode *node);
void pfiXformerAutoPosition(pfiXformer *x, pfChannel *chan, pfDCS *dcs);
void pfiXformerResetCoord(pfiXformer *x, pfCoord *coord);
void pfiXformerCoord(pfiXformer *x, pfCoord *coord);
void pfiSelectXformerModel(pfiXformer *x, int model);
int pfiGetXformerCurModelIndex(pfiXformer *x);
pfiInputXform *pfiGetXformerCurModel(pfiXformer *x);
pfType pfiGetIXformTravelClassType(void);
pfType pfiGetIXformTrackballClassType(void);
void pfiUpdateXformer(pfiXformer *x);
void pfiStopXformer(pfiXformer *x);
void pfiResetXformerPosition(pfiXformer *x);
void pfiCenterXformer(pfiXformer *x);
void pfiGetXformerCoord(pfiXformer *x, pfCoord *coord);
void pfiGetXformerMat(pfiXformer *x, pfMatrix m);
void pfiEnableXformerCollision(pfiXformer *x);
void pfiDisableXformerCollision(pfiXformer *x);
/* the collision of the eye with the scene, in the intersection callback */
int pfiCollideXformer(pfiXformer *x);

#endif
