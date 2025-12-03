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
 *  colors.h $Revision: 1.1 $
 */


#include "ci_colors.h"
#include "rgb_colors.h"

extern short ci_table[64];
extern unsigned long cpack_table[64];

#define COLOR(col) if (in_cmode)		\
			color(ci_table[col]);	\
		   else				\
			cpack(cpack_table[col])


/*
 *  packed RGBA colors
 */
#define CP_HORIZON	(110 + (190<<8) + (250<<16))
#define CP_SKY		( 60 + (140<<8) + (220<<16))

extern long cp_sky, cp_W_horizon, cp_E_horizon;


/*
 *  underlay colors
 */
#define U_BLACK	0
#define U_BROWN		1
#define U_INST		1
#define U_MARKINGS	2

/*
 *  PUP colors
 */
#define P_ORANGE	3
#define P_GREY1		1
#define P_GREY2		2
#define P_MARKINGS	3

