/*
 * Copyright 1989, 1990, 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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


#include "gobj.h"

#include "porting/iris2ogl.h"
#include <stdio.h>
#include <fcntl.h>


FILE *ofp;

static object_t *curobj;


writeobj(fname, obj)
    char *fname;
    object_t *obj;
{
    int i;

    ofp = fopen(fname, "w");

    fprintf(ofp, "R%f\n", obj->radius);
    fprintf(ofp, "#---------------------- Branch nodes\n", i);
    fprintf(ofp, "B%d	# number of branch nodes\n", obj->bcount);

    curobj = obj;

    for (i=0; i < obj->bcount; i++)
    {
	fprintf(ofp, "#---------------------- B%d\n", i);
	writenode(&obj->blist[i]);
    }

    fprintf(ofp, "#---------------------- Translation nodes\n", i);
    fprintf(ofp, "T%d	# number of transformation nodes\n", obj->tcount);

    for (i=0; i < obj->tcount; i++)
    {
	writetrans(&obj->tlist[i]);
    }

    if (obj->mcount)
    {
	fprintf(ofp, "#---------------------- Material nodes\n", i);
	fprintf(ofp, "M%d	# number of material nodes\n",
		obj->mcount);

	for (i=0; i < obj->mcount; i++)
	{
	    fprintf(ofp, "#---------------------- M%d\n", i);
	    writemat(obj->mlist[i]);
	}
    }

    fprintf(ofp, "#---------------------- Geometry nodes\n", i);
    fprintf(ofp, "G%d	# number of geometry nodes\n", obj->gcount);

    for (i=0; i < obj->gcount; i++)
    {
	fprintf(ofp, "#---------------------- G%d\n", i);
	fprintf(ofp, "%d\n", obj->glist[i].type);
	switch(obj->glist[i].type)
	{
	    case SSECTION:
	    case LTV_GEOM:
		writessect(&obj->glist[i]);
		break;
	    case FSECTION:
		writefsect(&obj->glist[i]);
		break;
	    case PSECTION:
		writepsect(&obj->glist[i]);
		break;
	    case CSECTION:
	    case CLS_GEOM:
	    case CDS_GEOM:
		writecsect(&obj->glist[i]);
		break;
	    case CPV_GEOM:
	    case CDV_GEOM:
		writecdvgeom(&obj->glist[i]);
		break;
	    case CPU_GEOM:
	    case CDU_GEOM:
		writecdugeom(&obj->glist[i]);
		break;
	    case IMU_GEOM:
		writeimugeom(&obj->glist[i]);
		break;
	    case IMV_GEOM:
		writeimvgeom(&obj->glist[i]);
		break;
	    default:
		fprintf(stderr, "Error in writing \"%s\"\n", fname);
	}
    }
    fclose(ofp);

    return(1);
}


writenode(np)
    node_t *np;
{
    int i;

    fprintf(ofp, "0x%X,0x%X\n", np->statebits, np->modebits);
    fprintf(ofp, "%d", np->tcount);
    for (i=0; i < np->tcount; i++)
	fprintf(ofp, ",%d",  np->tlist[i]);
    fprintf(ofp, "\n%d", np->scount);
    for (i=0; i < np->scount; i++)
	switch(np->stlist[i])
	{
	    case BRANCH:
		fprintf(ofp, ",B%d",  np->slist[i]);
		break;
	    case GEOMETRY:
		fprintf(ofp, ",G%d",  np->slist[i]);
		break;
	    default:
		break;
	}
    fprintf(ofp, "\n");
}


writetrans(t)
    trans_t *t;
{
    switch(t->type)
    {
	case ROTX:
	case ROTY:
	case ROTZ:
	    fprintf(ofp, "%d,%d\n", t->type, t->angle);
	    break;
	case TRANSLATE:
	case SCALE:
	    fprintf(ofp, "%d,%f,%f,%f\n", t->type,  t->x, t->y, t->z);
	    break;
	default:
	    break;
    }
}


writessect(sect)
    geometry_t *sect;
{
    int i, count;
    polygon_t *p;
    int vnum;

    write_mat_id(sect);
    fprintf(ofp, "%d\n", sect->vcount);
    for (i=0; i < sect->vcount; i++)
    {
	fprintf(ofp, "%f,%f,%f,%f,%f,%f\n",
		sect->vlist[i][X], sect->vlist[i][Y], sect->vlist[i][Z],
		sect->nlist[i][X], sect->nlist[i][Y], sect->nlist[i][Z]);
    }

    fprintf(ofp, "%d\n", sect->pcount);
    for (i=0; i < sect->pcount; i++)
    {
	p = &sect->plist[i];
	for(count = 0; count < p->vcount; count++)
	{
	    fprintf(ofp, "%d", p->vnlist[count]);
	    if (count < p->vcount-1)
	    {
		fprintf(ofp, ",");
		if (count && !(count % 10))
		    fprintf(ofp, "\\\n");
	    }
	    else
		fprintf(ofp, "\n");
	}
    }
}


writefsect(sect)
    geometry_t *sect;
{
    int i, count;
    polygon_t *p;
    int vnum;

    write_mat_id(sect);
    fprintf(ofp, "%d\n", sect->vcount);
    for (i=0; i < sect->vcount; i++)
    {
	fprintf(ofp, "%f,%f,%f\n",
		sect->vlist[i][X], sect->vlist[i][Y], sect->vlist[i][Z]);
    }

    fprintf(ofp, "%d\n", sect->pcount);
    for (i=0; i < sect->pcount; i++)
    {
	p = &sect->plist[i];
	fprintf(ofp, "%f,%f,%f,",
		p->normal[X], p->normal[Y], p->normal[Z]);

	for(count = 0; count < p->vcount; count++)
	{
	    fprintf(ofp, "%d", p->vnlist[count]);
	    if (count < p->vcount-1)
		fprintf(ofp, ",");
	    else
		fprintf(ofp, "\n");
	}
    }
}


writepsect(sect)
    geometry_t *sect;
{
    int i, count;
    polygon_t *p;
    int vnum;

    write_mat_id(sect);
    fprintf(ofp, "%f,%f,%f\n",
	    sect->normal[X], sect->normal[Y], sect->normal[Z]);
    fprintf(ofp, "%d\n", sect->vcount);
    for (i=0; i < sect->vcount; i++)
    {
	fprintf(ofp, "%f,%f,%f\n",
		sect->vlist[i][X], sect->vlist[i][Y], sect->vlist[i][Z]);
    }

    fprintf(ofp, "%d\n", sect->pcount);
    for (i=0; i < sect->pcount; i++)
    {
	p = &sect->plist[i];
	for(count = 0; count < p->vcount; count++)
	{
	    fprintf(ofp, "%d", p->vnlist[count]);
	    if (count < p->vcount-1)
		fprintf(ofp, ",");
	    else
		fprintf(ofp, "\n");
	}
    }
}


writecsect(sect)
    geometry_t *sect;
{
    int i, count;
    polygon_t *p;
    int vnum;

    fprintf(ofp, "0x%X\n", sect->color);
    fprintf(ofp, "%d\n", sect->vcount);
    for (i=0; i < sect->vcount; i++)
    {
	fprintf(ofp, "%f,%f,%f\n",
		sect->vlist[i][X], sect->vlist[i][Y], sect->vlist[i][Z]);
    }

    fprintf(ofp, "%d\n", sect->pcount);
    for (i=0; i < sect->pcount; i++)
    {
	p = &sect->plist[i];
	for(count = 0; count < p->vcount; count++)
	{
	    fprintf(ofp, "%d", p->vnlist[count]);
	    if (count < p->vcount-1)
		fprintf(ofp, ",");
	    else
		fprintf(ofp, "\n");
	}
    }
}


writecdvgeom(sect)
    geometry_t *sect;
{
    int i, count;
    polygon_t *p;
    int vnum;

    fprintf(ofp, "%d\n", sect->vcount);
    for (i=0; i < sect->vcount; i++)
    {
	fprintf(ofp, "0x%X,%f,%f,%f\n", sect->clist[i],
		sect->vlist[i][X], sect->vlist[i][Y], sect->vlist[i][Z]);
    }

    fprintf(ofp, "%d\n", sect->pcount);
    for (i=0; i < sect->pcount; i++)
    {
	p = &sect->plist[i];
	for(count = 0; count < p->vcount; count++)
	{
	    fprintf(ofp, "%d", p->vnlist[count]);
	    if (count < p->vcount-1)
		fprintf(ofp, ",");
	    else
		fprintf(ofp, "\n");
	}
    }
}


writecdugeom(sect)
    geometry_t *sect;
{
    int i, count;
    polygon_t *p;
    int vnum;

    fprintf(ofp, "%d\n", sect->vcount);
    for (i=0; i < sect->vcount; i++)
    {
	fprintf(ofp, "%f,%f,%f\n",
		sect->vlist[i][X], sect->vlist[i][Y], sect->vlist[i][Z]);
    }

    fprintf(ofp, "%d\n", sect->pcount);
    for (i=0; i < sect->pcount; i++)
    {
	p = &sect->plist[i];
	fprintf(ofp, "0x%x,", p->color);

	for(count = 0; count < p->vcount; count++)
	{
	    fprintf(ofp, "%d", p->vnlist[count]);
	    if (count < p->vcount-1)
		fprintf(ofp, ",");
	    else
		fprintf(ofp, "\n");
	}
    }
}


writeimugeom(sect)
    geometry_t *sect;
{
    int i, count;
    polygon_t *p;
    int vnum;

    fprintf(ofp, "%d\n", sect->vcount);
    for (i=0; i < sect->vcount; i++) 
    {
	fprintf(ofp, "%f,%f,%f\n",
		sect->vlist[i][X], sect->vlist[i][Y], sect->vlist[i][Z]);
    }

    fprintf(ofp, "%d\n", sect->pcount);
    for (i=0; i < sect->pcount; i++)
    {
	p = &sect->plist[i];
	fprintf(ofp, "%d,%d,", p->type, p->color);

	for (count=0; count < p->vcount; count++)
	{
	    fprintf(ofp, "%d", p->vnlist[count]);
	    if (count < p->vcount-1)
		fprintf(ofp, ",");
	    else
		fprintf(ofp, "\n");
	}
    }
}


writeimvgeom(sect)
    geometry_t *sect;
{
    int i, count;
    polygon_t *p;
    int vnum;

    fprintf(ofp, "%d\n", sect->vcount);
    for (i=0; i < sect->vcount; i++) 
    {
	fprintf(ofp, "%f,%f,%f,%d\n",
		sect->vlist[i][X], sect->vlist[i][Y], sect->vlist[i][Z],
		sect->clist[i]);
    }

    fprintf(ofp, "%d\n", sect->pcount);
    for (i=0; i < sect->pcount; i++)
    {
	p = &sect->plist[i];
	fprintf(ofp, "%d,", p->type);

	for (count=0; count < p->vcount; count++)
	{
	    fprintf(ofp, "%d", p->vnlist[count]);
	    if (count < p->vcount-1)
		fprintf(ofp, ",");
	    else
		fprintf(ofp, "\n");
	}
    }
}


bwriteobj(fname, obj)
    char *fname;
    object_t *obj;
{
    int ofd;
    node_t *b;
    geometry_t *g;
    polygon_t *p;
    int i, j;

    ofd = open(fname, O_WRONLY | O_CREAT, 0644);
    if (ofd == -1)
	fprintf(stderr, "can't open \"%s\"\n", fname);

    /*
     *  write null byte that says this is a binary file
     */
    write(ofd, "\0", 1);

    write(ofd, obj, sizeof(object_t));

    write(ofd, obj->blist, sizeof(node_t) * obj->bcount);
    for (i=0; i < obj->bcount; i++)
    {
	b = &obj->blist[i];
	if (b->tcount)
	    write(ofd, b->tlist, sizeof(int) * b->tcount);
	if (b->scount)
	{
	    write(ofd, b->slist, sizeof(int) * b->scount);
	    write(ofd, b->stlist, sizeof(int) * b->scount);
	}
    }

    if (obj->tcount)
	write(ofd, obj->tlist, sizeof(trans_t) * obj->tcount);

    write(ofd, obj->glist, sizeof(geometry_t) * obj->gcount);

    for (i=0; i < obj->gcount; i++)
    {
	g = &obj->glist[i];

	for (j=0; j < g->vcount; j++)
	    write(ofd, g->vlist[j], sizeof(float)*3);
	    
	if (g->nlist)
	{
	    for (j=0; j < g->vcount; j++)
		write(ofd, g->nlist[j], sizeof(float)*3);
	}

	if (g->clist)
	    write(ofd, g->clist, sizeof(long) * g->vcount);

	write(ofd, g->plist, sizeof(polygon_t) * g->pcount);
	for (j=0; j < g->pcount; j++)
	{
	    p = &g->plist[j];

	    write(ofd, p->vnlist, sizeof(int) * p->vcount);
	}
    }

    close(ofd);
}


writemat(id)
    int id;
{
    fprintf(ofp, "%f,%f,%f\n",
	    mlist[id].data[1], mlist[id].data[2], mlist[id].data[3]);
    fprintf(ofp, "%f,%f,%f,%f\n",
	    mlist[id].data[5], mlist[id].data[6], mlist[id].data[7],
	    mlist[id].data[9]);
    fprintf(ofp, "%f,%f,%f\n",
	    mlist[id].data[11], mlist[id].data[12], mlist[id].data[13]);
    fprintf(ofp, "%f\n", mlist[id].data[15]);
    fprintf(ofp, "%f,%f,%f\n",
	    mlist[id].data[17], mlist[id].data[18], mlist[id].data[19]);
}

write_mat_id(sect)
    geometry_t *sect;
{
    if (!curobj->mcount)
	fprintf(ofp, "%d\n", sect->material);
    else
    {
	int i;

	for (i=0; sect->material != mlist[curobj->mlist[i]].id; i++);

	fprintf(ofp, "%d\n", i);
    }
}
