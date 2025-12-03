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

/*
 *  flight/fog.c $Revision: 1.1 $
 */

#include "math.h"
#include "fcntl.h"
#include "stdio.h"
#include "flight.h"


#define MIN_FD		1
#define START_FD	67
#define MAX_FD		100


int fogon = FALSE;
int fog_d = START_FD;
unsigned long fog_c = 0xff999999;


set_fog_density(d)
    int d;
{
    float p[4];

    fog_d += d;
    if (fog_d < MIN_FD)
	fog_d = MIN_FD;
    else if (fog_d > MAX_FD)
	fog_d = MAX_FD;

    p[0] = 1.0 / fexp(fog_d/6.0);
    p[1] = (fog_c & 0xff) / 255.0;
    p[2] = ((fog_c >> 8) & 0xff) / 255.0;
    p[3] = ((fog_c >> 16) & 0xff) / 255.0;

    fogvertex(FG_DEFINE, p);
}


set_fog_color(c)
    unsigned long c;
{
    float p[4];

    fog_c = c;
    p[0] = 1.0 / fexp(fog_d/6.0);
    p[1] = (fog_c & 0xff) / 255.0;
    p[2] = ((fog_c >> 8) & 0xff) / 255.0;
    p[3] = ((fog_c >> 16) & 0xff) / 255.0;

    fogvertex(FG_DEFINE, p);
}


fog(b)
{
    if (fogon = b)
	fogvertex(FG_ON, (float *)0);
    else
	fogvertex(FG_OFF, (float *)0);
}

