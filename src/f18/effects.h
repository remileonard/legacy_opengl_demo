/*
 * effects.h - the visual effects of the F-18 demo: the esAddEffect effects
 * (explosions, debris, white flash), the fumes (esAddFume: the exhaust of
 * the missiles, the black smoke of the airplanes hit) and the smoke
 * columns (esAddSmoke: the wrecks)
 */
#ifndef EFFECTS_H
#define EFFECTS_H

#include "escene.h"

enum { FX_EXPLOSION = 1, FX_DEBRIS = 2, FX_FLAK = 3, FX_FLASH = 4 };

/* initEffects: the texture of the effects and the modules that update and
 * draw them */
void fxInit(void);
/* esAddEffect: an effect at a point (Performer coordinates), of a size */
void fxAddEffect(int type, const float pos[3], float scale);

/* esAddFume: puffs left behind an object, growing from size0 to size1 and
 * fading in life frames; returns a handle, -1 on error */
int fxAddFume(esObject *obj, float r, float g, float b, float size0, float size1, float life);
void fxFumeOn(int fume, int on);
void fxFumeColor(int fume, float r, float g, float b);

/* esAddSmoke: a column of smoke rising from an object */
int fxAddSmoke(esObject *obj, float size, float density);
void fxRemoveSmoke(int smoke);

#endif
