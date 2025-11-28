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
/*
 *	Better lighting constants
 * By Gavin Bell for Silicon Graphics, Inc.
 * 12/2/88
*/
#include "porting/iris2ogl.h"
#include <stdio.h>

#include "light.h"

/*
 *	When changing materials/models, keep track of the last one so we
 * don't immediately lmbind it again
 */
static int curmaterial = -1;
static int curmodel = -1;

/* Pre-define a few things declared at the bottom of this file */
extern float ldef_material[][15];
extern float ldef_light[][14];
extern float ldef_lmodel[][10];

/* This keeps track of which lights are on/off */
static int onoff[MAX_LIGHTS];

static float ldef_alpha[] =
{
	ALPHA, 0.2, LMNULL
};

#define SIZE(x) sizeof(x)/sizeof(float)

void
defineshading(void)
{
	int i, alpha;
	char machinetype[32];
	
	/*
	 * Only define transparent materials if machine is capable of
	 * blending
	 */
	if (getgdesc(GD_BLEND))
		alpha = 1;
	else alpha = 0;

	/* define material properties */
	for (i = 0; i < NUM_MATERIALS; i++)
	{
		lmdef (DEFMATERIAL, i+1, SIZE(ldef_material[i]), ldef_material[i]);
		if (alpha)
		{
			lmdef(DEFMATERIAL, i+1+NUM_MATERIALS,
				SIZE(ldef_material[i]), ldef_material[i]);
			lmdef(DEFMATERIAL, i+1+NUM_MATERIALS,
				SIZE(ldef_alpha), ldef_alpha);
		}
	}

	/* define lighting model */
	for (i = 0 ; i < NUM_LMODELS; i++)
		lmdef (DEFLMODEL, i+1, SIZE(ldef_lmodel[i]), ldef_lmodel[i]);

	/* define light source properties */
	for (i = 0 ; (i < NUM_LIGHTS) && (i < MAX_LIGHTS); i++)
		lmdef (DEFLIGHT, i+1, SIZE(ldef_light[i]), ldef_light[i]);

	/* Turn all the lights off */
	for (i = 0; i < MAX_LIGHTS; i++) onoff[i] = 0;
}

void
setmaterial(int m)
{
	/* do not pick same material twice */
	if (m == curmaterial) 
		return;

	if (m < 0)
		fprintf(stderr, "setmaterial error: material %d undefined\n", m);
	else
	{	/* Use the material */
		curmaterial = m;
		lmbind(MATERIAL, curmaterial+1);
	}
}

/*
 *	Turn given light on or off:  Note that the light's position will
 * be affected by what's currently on the viewing stack.  Note also
 * that this routine assumes that LIGHT0,LIGHT1, etc. are #defined in
 * numerical order!
 */
/*
 *	Enhancement possibility:  store the light number in the onoff
 * array, in the position it is bound (i.e. if light[4] is bound to
 * LIGHT2, then onoff[2] == 4).  When turning on or off, look for the
 * light in onoff; if found, turn off by setting onoff[n] = 0 and
 * lmbinding it.  If not found, look for an empty space in onoff and
 * bind it there.  If no empty space if found, print a warning.
 */
void
switch_light(int m)
{
	if (m > NUM_LIGHTS || m < 0 || m > MAX_LIGHTS)
	{
		fprintf(stderr, "switch_light error: light %d undefined\n", m);
	}
	else
	{
		if (onoff[m])	/* Light is currently on */
			onoff[m] = 0;
		else
			onoff[m] = m+1;
		lmbind(LIGHT0 + m, onoff[m]);
	}
}

/*
 *	This routine re-binds lights.  Use it when you desire moving
 * lights (re-define the viewing transformation matrix, call this
 * routine to get transformed lights, then draw your objects).
 */
void
rebind_lights(void)
{
	int i;
	for (i = 0 ; (i < MAX_LIGHTS) && (i < NUM_LIGHTS) ; i++)
	{
		lmbind(LIGHT0 + i, onoff[i]);
	}
}

void
setmodel(int m)
{
	if (m == curmodel)
		return;
	if (m < 0)
	{
		fprintf(stderr, "setmodel error: model %d undefined\n", m);
	}
	else
	{
		curmodel = m;
		lmbind(LMODEL, curmodel+1);
	}
}

void
resetmodel()
{
	lmbind(LMODEL, curmodel+1);
}

static float lmat[] =
{
	EMISSION, 0.0, 0.0, 0.0,
	DIFFUSE, 0.0, 0.0, 0.0, 
	AMBIENT, 0.0, 0.0, 0.0,
	SHININESS, 0.0, 
	LMNULL
};

int
draw_lights(void)
{
	void draw_icosa(void);
	int i, numdrawn;

	numdrawn = 0;
	for (i = 0 ; i < NUM_LIGHTS; i++)
	{
		if (!onoff[i]) continue;

		lmat[1] = ldef_light[i][1] * 0.4;
		lmat[2] = ldef_light[i][2] * 0.4;
		lmat[3] = ldef_light[i][3] * 0.4;

		lmat[5] = ldef_light[i][1] ;
		lmat[6] = ldef_light[i][2] ;
		lmat[7] = ldef_light[i][3] ;

		lmdef(DEFMATERIAL, NUM_MATERIALS+1, SIZE(lmat), lmat);
		setmaterial(NUM_MATERIALS);
		pushmatrix();
		translate(ldef_light[i][9], ldef_light[i][10],
			ldef_light[i][11]);
		if (ldef_light[i][12] != 0.0)	/* Local Viewer? */
		{
			scale(0.02, 0.02, 0.02);
			draw_icosa();
			numdrawn += 20;
		}
		popmatrix();
	}
	return numdrawn;
}
/*
 * Code produced by tm2code
 *  Run on file icosa.TM
 */
/* VERTEX DATA */
static float vdata[] = {
0.607062,0,-0.794654,
-0.303531,0.525731,-0.794655,
-0.303531,-0.525731,-0.794655,
0.491123,0.850651,-0.187592,
-0.982247,0,-0.187592,
0.491123,-0.850651,-0.187592,
-0.491123,0.850651,0.187592,
0.982247,0,0.187592,
-0.491123,-0.850651,0.187592,
0.303531,0.525731,0.794655,
-0.607062,0,0.794655,
0.303531,-0.525731,0.794655,
};

void
draw_icosa(void)
{
	bgntmesh();
	n3f(vdata+15);
	v3f(vdata+15);
	n3f(vdata+33);
	v3f(vdata+33);
	n3f(vdata+21);
	v3f(vdata+21);
	n3f(vdata+27);
	v3f(vdata+27);
	swaptmesh();
	n3f(vdata+9);
	v3f(vdata+9);
	swaptmesh();
	n3f(vdata+0);
	v3f(vdata+0);
	n3f(vdata+15);
	v3f(vdata+15);
	n3f(vdata+6);
	v3f(vdata+6);
	swaptmesh();
	n3f(vdata+24);
	v3f(vdata+24);
	n3f(vdata+33);
	v3f(vdata+33);
	n3f(vdata+30);
	v3f(vdata+30);
	n3f(vdata+27);
	v3f(vdata+27);
	n3f(vdata+18);
	v3f(vdata+18);
	n3f(vdata+9);
	v3f(vdata+9);
	n3f(vdata+3);
	v3f(vdata+3);
	n3f(vdata+0);
	v3f(vdata+0);
	swaptmesh();
	n3f(vdata+6);
	v3f(vdata+6);
	swaptmesh();
	n3f(vdata+12);
	v3f(vdata+12);
	n3f(vdata+18);
	v3f(vdata+18);
	swaptmesh();
	n3f(vdata+30);
	v3f(vdata+30);
	swaptmesh();
	n3f(vdata+24);
	v3f(vdata+24);
	n3f(vdata+6);
	v3f(vdata+6);
	endtmesh();
}

/*
 *	Static definitions for lighting.
 */

char *matnames[NUM_MATERIALS] =
{
	"Brass",
	"Shiny Brass",
	"Pewter",
	"Silver",
	"Gold",
	"Shiny Gold",
	"Plaster",
	"Red Plastic", 
	"Green Plastic",
	"Blue Plastic", 
};

float ldef_material[][15] =
{
	{	/* BRASS */
		AMBIENT, .35, .25,  .1,
		DIFFUSE, .65, .5, .35,
		SPECULAR, .8, .6, 0.,
		SHININESS, 5.,
		LMNULL
	}, 
	{	/* SHINY BRASS */
		AMBIENT, .25, .15, 0.,
		DIFFUSE, .65, .5, .35,
		SPECULAR, .9, .6, 0.,
		SHININESS, 10.,
		LMNULL
	}, 
	{	/* PEWTER */
		AMBIENT, .1, .1,  .1,
		DIFFUSE, .6, .55 , .65,
		SPECULAR, .9, .9, .95,
		SHININESS, 10.,
		LMNULL
	}, 
	{	/* SILVER */
		AMBIENT, .4, .4,  .4,
		DIFFUSE, .3, .3, .3,
		SPECULAR, .9, .9, .95,
		SHININESS, 30.,
		LMNULL
	}, 
	{	/* GOLD */
		AMBIENT, .4, .2, 0.,
		DIFFUSE, .9, .5, 0.,
		SPECULAR, .7, .7, 0.,
		SHININESS, 10.,
		LMNULL
	}, 
	{	/* SHINY GOLD */
		AMBIENT, .4, .2, 0.,
		DIFFUSE, .9, .5, 0.,
		SPECULAR, .9, .9, 0.,
		SHININESS, 2.,
		LMNULL
	}, 
	{	/* PLASTER */
		AMBIENT, .2, .2,  .2,
		DIFFUSE, .95, .95, .95,
		SPECULAR, 0., 0., 0.,
		SHININESS, 1.,
		LMNULL
	}, 
	{	/* RED PLASTIC */
		AMBIENT, .3, .1, .1,
		DIFFUSE, .5, .1, .1,
		SPECULAR, .45, .45, .45,
		SHININESS, 30.,
		LMNULL
	}, 
	{	/* GREEN PLASTIC */
		AMBIENT, .1, .3, .1,
		DIFFUSE, .1, .5, .1,
		SPECULAR, .45, .45, .45,
		SHININESS, 30.,
		LMNULL
	},
	{	/* BLUE PLASTIC */
		AMBIENT, .1, .1, .3,
		DIFFUSE, .1, .1, .5,
		SPECULAR, .45, .45, .45,
		SHININESS, 30.,
		LMNULL
	}
} ;

char *lightnames[NUM_LIGHTS] =
{
	"White Infinite",
	"Red Local",
	"Blue Infinite",
	"Green Local",
	"Yellow Local",
	"Magenta Infinite",
	"White Local",
	"Pink Infinite"
} ;

float ldef_light[][14] =
{
	{	/* WHITE INFINITE */
		LCOLOR, .8, .8, .8,
		AMBIENT, 0., 0., 0.,
		POSITION, -0.2, 0.2, 0.2, 0., 
		LMNULL
	},
	{	/* RED LOCAL */
		LCOLOR, .9, 0., 0.,
		AMBIENT, .1, 0., 0.,
		POSITION, 0.2, 0.2, 0.2, 1., 
		LMNULL
	}, 
	{	/* BLUE INFINITE */
		LCOLOR, .1, .1, .9,
		AMBIENT, 0., 0., .05,
		POSITION, 0.2, -0.2, 0.2, 0., 
		LMNULL
	}, 
	{	/* GREEN LOCAL */
		LCOLOR, .04, .9, .04,
		AMBIENT, .0, 0., 0.,
		POSITION, -0.2, -0.2, 0.2, 1., 
		LMNULL
	}, 
	{	/* YELLOW LOCAL */
		LCOLOR, .8, .8, .3,
		AMBIENT, .05, .05, 0.,
		POSITION, -0.2, 0.2, -0.2, 1., 
		LMNULL
	}, 
	{	/* MAGENTA INFINITE */
		LCOLOR,	.8, 0., .7,
		AMBIENT, .0, .0, .0,
		POSITION, 0.2, 0.2, -0.2, 0., 
		LMNULL
	}, 
	{	/* WHITE LOCAL */
		LCOLOR, 1.0, 1.0, 1.0,
		AMBIENT, 0., 0., 0.,
		POSITION, 0.2, -0.2, -0.2, 1., 
		LMNULL
	}, 
	{	/* PINK INFINITE */
		LCOLOR, .9, .5, .65,
		AMBIENT, 0., 0., 0., 
		POSITION, -0.2, -0.2, -0.2, 0., 
		LMNULL
	}, 
};

char *lmodelnames[NUM_LMODELS] =
{
	"Infinite Viewer",
	"Local Viewer", 
};

float ldef_lmodel[][10] =
{
	{	/* Infinite viewer */
		LOCALVIEWER, 0., 
		AMBIENT, .3,  .3, .3, 
		ATTENUATION, 1., 0., 
		LMNULL
	}, 
	{	/* Local viewer */
		LOCALVIEWER, 1.0, 
		AMBIENT, .3,  .3,  .3, 
		ATTENUATION, 1.0, 0.02, 
		LMNULL
	}, 
};
