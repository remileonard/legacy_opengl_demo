/*
 * Copyright 1984, 1991, 1992, 1993, 1994, Silicon Graphics, Inc.
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
 *  light.c
 */

#include "gobj.h"
#include "porting/iris2ogl.h"

static int curmaterial = 0;

void setmaterial(int name) {
    if (name != curmaterial) {
        curmaterial = name;
        lmbind(MATERIAL, name);
    }
}

void lsuspend(int b) {
    if (b) {
        if (curmaterial)
            lmbind(MATERIAL, 0);
    } else {
        if (curmaterial)
            lmbind(MATERIAL, curmaterial);
    }
}
