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
 *	hash.h
 *	Various functions for figuring out if we've seen
 *	a vertex/edge before.  Uses very simple hashing schemes,
 *  not appropriate for heavy-duty industrial use.
 */

/*
 *	These two functions return number of unique vertices/edge seen
 */
int h_get_nv(void);
int h_get_ne(void);

/*
 *	These functions initialize the tables; they must be given the
 * maximum possible number of distinct vertices/edges
 */
void h_init_vertex(int);
void h_init_edge(int);

/*
 *	And these functions free up the space made in above
 */
void h_destroy_vertex(void);
void h_destroy_edge(void);

/*
 *	The useful functions.  Returns number of vertex/edge found.  These
 * numbers are guaranteed to be unique for unique edges/vertices, and
 * will increase by one every time a unique vertex/edge is found.
 */

/* Argument points to a float[3], which should be x, y, and z */
int h_find_vertex(float *);

/* Argument is vertex numbers found from h_find_vertex */
int h_find_edge(int, int);
