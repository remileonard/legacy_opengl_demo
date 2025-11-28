/*
 * Copyright 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to restrictions
 * as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software clause at DFARS 252.227-7013, and/or in similar or
 * successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
 * rights reserved under the Copyright Laws of the United States.
 */
/* lighting and material stuff */

/* Use defineshading() before trying to use other routines */
void defineshading(void) ;

/*
 * setmaterial expects a value in the range of 0...NUM_MATERIALS, and
 * will make that material the current material.  Materials
 * NUM_MATERIALS...NUM_MATERIALS*2 are defined to be transparent
 * versions of the materials on machines that can do alpha-blending.
 * Materials are defined at the end of light.c
 */
#define NUM_MATERIALS 10
void setmaterial(int);
extern char *matnames[NUM_MATERIALS];

/*
 *	switch_light will toggle the given light (rand 0..NUM_LIGHTS) on
 * and off.
 */
#define NUM_LIGHTS 8
void switch_light(int) ;
extern char *lightnames[NUM_LIGHTS];

/*
 *	rebind_lights will rebind all the lights that are turned on,
 * causing them to change position if the viewing matrix has changed.
 */
void rebind_lights(void);

/*
 *	draw_lights will draw a representation of each of the lights that
 * is turned on; local lights are represented by small spheres
 * (icosahedron, really) and infinite lights are arrows pointing in
 * the direction they are shining.  It returns the number of polygons
 * required to draw the lights.
 */
int draw_lights(void);

/*
 *	setmodel will set the lighting model (range 0..NUM_LMODELS)
 */
#define NUM_LMODELS 2
void setmodel(int);
extern char *lmodelnames[NUM_LMODELS];

/*
 *	And resetmodel() should be used to turn lighting back on after it
 * has been turned off.
 */
void resetmodel(void);
