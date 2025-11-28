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
 *	hash.c
 *	Various functions for figuring out if we've seen
 *	a vertex/edge before.  Uses very simple hashing schemes,
 *  not appropriate for heavy-duty industrial use.
 *  See hash.h for programmer's interface.
 */
#include <stdio.h>
#include "hash.h"

typedef struct {
	int v0, v1;	/* Two vertices... */
	int num;	/* And a unique number */
} h_edge;
typedef struct {
	float *v;	/* Pointer to float[3] data */
	int num;	/* And unique number */
} h_vertex;

/*
 * The actual hash tables...
 */
static h_vertex *vh = NULL;
static int vsize = 0;
static h_edge *eh = NULL;
static int esize = 0;

/*
 * And for consecutive vertex/edge/face numbering
 *	(so each gets his/her/its own number, and we
 *	actually know how many there are at the end)
 */
static int lastv = 0;
static int laste = 0;

int
h_get_nv()
{
	return lastv;
}
int
h_get_ne()
{
	return laste;
}

/*
 *	Create a hash table for n vertices
 */
void
h_init_vertex(n)
int n;
{
	int i;

	vsize = n * 2;
	vh = (h_vertex *) malloc(sizeof(h_vertex) * vsize);
	if (!vh) {
        fprintf(stderr, "h_init_vertex: malloc FAILED\n");
        exit(1);
    }
	for (i = 0; i < vsize; i++) {
		vh[i].num = (-1);
	}
}
void h_destroy_vertex()
{
	free(vh);
	lastv = vsize = 0;
}

/*
 * Search the hash table for vertex with given xyz.
 *	Return vertex number.
 */
int
h_find_vertex(xyz)
float *xyz;
{
	float d;
	int hpos;

/* Starting at the previous entry, go looking for
 *	a vertex close enough to be the same...
 */
	d = (xyz[0] + xyz[1] + xyz[2])*19997.0;	/* Spread out... */
	if (d < 0.0) d = -d;
	hpos = (int)(d) % vsize;

	while (vh[hpos].num >= 0) /* Hash position FULL */
	{
		/*
		 * So start looking for this vertex in hash list
		 */
		if (veq(vh[hpos].v, xyz))
		{
			return vh[hpos].num;
		}
		else hpos = (hpos + 1) % vsize;	/* Check next spot */
	}
	/* If it wasn't found, insert it */
	vh[hpos].num = lastv;
	vh[hpos].v = xyz;
	++lastv;
	return vh[hpos].num;
}

#define FLUFF 1.0e-5
#define ABS(a) ((a) < 0.0 ? -(a) : (a))
#define EQ(a, b) ( ABS((a)-(b)) < FLUFF )

int
veq(v0, v1)
float *v0, *v1;
{
	if (EQ(v0[0], v1[0]) && EQ(v0[1], v1[1]) && EQ(v0[2], v1[2]))
			return 1;
	return 0;
}

void
h_init_edge(n)
int n;
{
	int i;
	esize = n * 2;
	eh = (h_edge *) malloc(sizeof(h_edge) * esize);
	for (i = 0; i < esize; i++) eh[i].num = (-1);
}
void
h_destroy_edge()
{
	free(eh);
	laste = esize = 0;
}

/*
 *	Search hash table for edge defined by v1, v2
 *	Return edge number.
 */
int
h_find_edge(v0, v1)
int v0, v1;
{
	int hpos;

	if (v0 > v1)
	{
		int t;
		t = v0; v0 = v1; v1 = t;
	}
	hpos = (v0 * 9997 + v1) % esize;
	while (eh[hpos].num >= 0)
	{
		if ((eh[hpos].v0 == v0) && (eh[hpos].v1 == v1))
		{
			return eh[hpos].num;
		}
		else hpos = (hpos + 1) % esize;
	}
	eh[hpos].num = laste;
	eh[hpos].v0 = v0;
	eh[hpos].v1 = v1;
	++laste;
	return eh[hpos].num;
}
