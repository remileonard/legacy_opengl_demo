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
 *  objects.h $Revision: 1.1 $
 */


/*
 *  plane types
 */
#define C150 0
#define B747 1
#define F15  2
#define F16  3
#define F18  4
#define P38  5
#define F14  6
#define B727 7
#define STEALTH 8

/*
 *  plane names
 */
#define C150_NAME "C-150"
#define B747_NAME "B-747"
#define F15_NAME "F-15"
#define F16_NAME "F-16"
#define F18_NAME "F-18"
#define P38_NAME "P-38"
#define F14_NAME "F-14"
#define B727_NAME "B-727"
#define STEALTH_NAME "F-117A Stealth"

/*
 *  plane state bits
 */
#define PS_MAINBODY	0x0001
#define PS_LANDINGGEAR	0x0002
#define PS_FAR		0x0004
#define PS_SHADOW	0x0008
#define PS_FARSHADOW	0x0010
#define PS_THRUST	0x0020
#define PS_SW1		0x0100
#define PS_SW2		0x0200
#define PS_SW3		0x0400
#define PS_SW4		0x0800
#define PS_RK1		0x1000
#define PS_RK2		0x2000
#define PS_RK3		0x4000
#define PS_RK4		0x8000
#define PS_WEAPONS	0xff00
#define PS_W_SHIFT	8

/*
 *  building state bits
 */
#define BUILDING_NEAR	0x01
#define BUILDING_FAR	0x02
