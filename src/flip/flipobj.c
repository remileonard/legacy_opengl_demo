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
#include <math.h>
#include "stdio.h"
#include "porting/iris2ogl.h"

#include "flip.h"
#include "hash.h"

int32_t swap_int32(int32_t val)
{
    uint8_t *bytes = (uint8_t *)&val;
    return ((int32_t)bytes[0] << 24) |
           ((int32_t)bytes[1] << 16) |
           ((int32_t)bytes[2] << 8) |
           ((int32_t)bytes[3]);
}

flipobj
*readflipobj(name)
char *name;
{
	FILE	*inf;
	flipobj	*obj;
	int32_t		i, j;
	int32_t		nlongs;
	int32_t		magic;
	int32_t		*ip;

	inf = fopen(name,"r");
	if(!inf) {
		fprintf(stderr,"readfast: can't open input file %s\n",name);
		exit(1);
	}
	fread(&magic, sizeof(int32_t), 1, inf);
    magic = swap_int32(magic); // swap endian
	if(magic != FASTMAGIC) {
	fprintf(stderr,"readfast: bad magic in object file\n");
	fclose(inf);
		exit(1);
	}
	obj = (flipobj *)malloc(sizeof(flipobj));
	fread(&obj->npoints,sizeof(int32_t),1,inf);
	obj->npoints = swap_int32(obj->npoints);
/*** IGNORE COLORS FIELD ***/
	fread(&magic,sizeof(int32_t),1,inf);

	/*
	 * Insure that the data is quad-word aligned and begins on a page
	 * boundary.  This shields us from the performance loss which occurs 
	 * whenever we try to fetch data which straddles a page boundary  (the OS
	 * has to map in the next virtual page and re-start the DMA transfer).
	 */
	nlongs = 8 * obj->npoints;
	obj->data = (int32_t *) malloc(nlongs*sizeof(int32_t) + 4096);
	//obj->data = (int32_t *) (((int)(obj->data)) + 0xfff);
	//obj->data = (int32_t *) (((int)(obj->data)) & 0xfffff000);
	ip = (int32_t *)obj->data;
	for (i = 0;  i < nlongs/4;  i++, ip += 4) {
		fread(ip, 3 * sizeof(int32_t), 1, inf);
		ip[0] = swap_int32(ip[0]);
		ip[1] = swap_int32(ip[1]);
		ip[2] = swap_int32(ip[2]);
	}
		
	fclose(inf);

/*
 *	This has to be done first
 */
	swirl_randomize(obj);
	find_edges(obj);

	return obj;
}

void
drawflipobj(obj)
flipobj *obj;
{
	register float *p,*end;
	enum DrawType lflag;

	p = obj->data;
	end = p + 8 * obj->npoints;
	lflag = obj->type;

	if (obj->type == POLYGONS)
	{
		while ( p < end) {
			bgnpolygon();
			n3f(p);
			v3f(p+4);
			n3f(p+8);
			v3f(p+12);
			n3f(p+16);
			v3f(p+20);
			n3f(p+24);
			v3f(p+28);
			endpolygon();
			p += 32;
		}
	}
	else
	{
		int i;
		
		for (i = 0 ; i < obj->nedges; i++)
		{
			bgnline();
			n3f(obj->edge[i].v0 - 4);
			v3f(obj->edge[i].v0);
			n3f(obj->edge[i].v1 - 4);
			v3f(obj->edge[i].v1);
			endline();
		}
	}
}


/*
 * objmaxpoint
 *
 * find the vertex farthest from the origin,
 * so we can set the near and far clipping planes tightly.
 */

#define MAXVERT(v) if ( (len = sqrt(	(*(v))  *  (*(v))  +	  \
					(*(v+1)) * (*(v+1)) +		   \
					(*(v+2)) * (*(v+2)) )) > max)  \
			max = len;

float
objmaxpoint(obj)
flipobj *obj;
{
	register float *p, *end;
	register int npolys;
	register float len;
	register float max = 0.0;

	p = obj->data;

	end = p + 8 * obj->npoints;
	while ( p < end) {
		MAXVERT(p+4);
		MAXVERT(p+12);
		MAXVERT(p+20);
		MAXVERT(p+28);
		p += 32;
	}

	return max;
}

/*
 *	Use hash functions to find all unique edges
 */
find_edges(obj)
flipobj *obj;
{
	int i, j, v0, v1, n;
	float *p, *end;

	h_init_vertex(obj->npoints * 2);
	h_init_edge(obj->npoints * 4);

/* First run through, to figure out how many there are */
	p = obj->data;
	end = p + 8 * obj->npoints;
	while ( p < end) {
		v0 = h_find_vertex(p+4);
		v1 = h_find_vertex(p+12);
		h_find_edge(v0, v1);
		v0 = h_find_vertex(p+12);
		v1 = h_find_vertex(p+20);
		h_find_edge(v0, v1);
		v0 = h_find_vertex(p+20);
		v1 = h_find_vertex(p+28);
		h_find_edge(v0, v1);
		v0 = h_find_vertex(p+28);
		v1 = h_find_vertex(p+4);
		h_find_edge(v0, v1);
		p += 32;
	}
/* Now malloc enough space */
	obj->nedges = h_get_ne();
	obj->edge = (flipedge *)malloc(sizeof(flipedge)*obj->nedges);

/* And now run through, filling up structure */
	p = obj->data;
	end = p + 8 * obj->npoints;
	while ( p < end) {
		v0 = h_find_vertex(p+4);
		v1 = h_find_vertex(p+12);
		n = h_find_edge(v0, v1);
		obj->edge[n].v0 = p+4;
		obj->edge[n].v1 = p+12;
		
		v0 = h_find_vertex(p+12);
		v1 = h_find_vertex(p+20);
		n = h_find_edge(v0, v1);
		obj->edge[n].v0 = p+12;
		obj->edge[n].v1 = p+20;

		v0 = h_find_vertex(p+20);
		v1 = h_find_vertex(p+28);
		n = h_find_edge(v0, v1);
		obj->edge[n].v0 = p+20;
		obj->edge[n].v1 = p+28;

		v0 = h_find_vertex(p+28);
		v1 = h_find_vertex(p+4);
		n = h_find_edge(v0, v1);
		obj->edge[n].v0 = p+28;
		obj->edge[n].v1 = p+4;

		p += 32;
	}
	h_destroy_vertex();
	h_destroy_edge();
}
