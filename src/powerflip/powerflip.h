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
#ifndef POWERFLIP_H
#define POWERFLIP_H

#include <stdio.h>
#include "porting/iris2ogl.h"
#include "light.h"
#include "libdemo/trackball.h"
#include "libdemo/ui.h"
#include "libdemo/event.h"
#include "yaodl.h"

#define MODELDIR "/usr/demos/data/models/"

/* drawing data types */

enum DrawType
{
	POLYGONS,
	LINES,
	SUBSMOOTHLINES, 
	NUM_DrawTypes
} ;

typedef struct {
    int npoints;
	int display ;	/* Draw me at all ? */
	enum DrawType type ;	/* Polygons? Lines? What? */
	int select ;	/* Am I selected? */
    int material;	/* Material index */
    int ablend;	/* Alpha-blend me? */
	char *fname ;	/* Filename for menus */
	struct YModel *model;	/* YAODL objects */
	float er[4];	/* Cumulative rotation */
	float espin[4];	/* Incremental rotation to spin */
	float trans[3];	/* Translation */
} flipobj;

/* YAODL models of powerflip (.ydl, .defs), see pfobj.c and yaodl.c */
flipobj *readflipobj(char *name);
void drawflipobj(flipobj *obj);
float objmaxpoint(flipobj *obj);

#endif
