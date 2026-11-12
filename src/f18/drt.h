/*
 * drt.h - the "runtime" files of Designer's Workbench (.drt): the cockpit
 * head up display of the F-18 demo
 *
 * A .drt file is a .dwb file with links: transformations, visibility,
 * colors and texts of its groups, faces and strings driven by functions of
 * variables.  The variables are floats at offsets of a structure of the
 * application (esSetDrtBaseAddr), for the HUD of the demo the hudVars of
 * updateHudValues.
 */
#ifndef DRT_H
#define DRT_H

typedef struct Drt Drt;

/* reads a .drt (or .dwb) file, NULL on error */
Drt *drtLoad(const char *file);
/* draws it in the current OpenGL frame (Y up, the faces toward +Z), with
 * the variables at vars */
void drtDraw(Drt *d, const float *vars, int nvars);

#endif
