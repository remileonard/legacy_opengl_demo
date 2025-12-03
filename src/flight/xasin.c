/*
 * Copyright 1984-1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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
 *  flight/xasin.c $Revision: 1.1 $
 */
#include "porting/iris2ogl.h"

extern int xatable1[], xatable2[], xatable3[];

Angle xasin(r)
float r;
{
    short neg;
    int i;

    i = r * 1073741824.0;
    if (i < 0) {
        neg = 1;
        i = -i;
    } else
        neg = 0;

    if (i <= 929887697) /* 0 - 60 degrees	*/
        i = xatable1[(i + 1048580) >> 21];
    else {
        i -= 1069655912;
        if (i < -20000)
            i = xatable2[((-i) - 20000) >> 18]; /* 60 - 85 degrees	*/
        else {
            if (i > 0)
                i = (i + 1895) >> 13;
            else
                i = 0;
            if (i < 499)
                i = xatable3[i]; /* 85 - 90  degrees	*/
            else
                i = 900;
        }
    }

    if (neg)
        return (-i);
    else
        return (i);
}
