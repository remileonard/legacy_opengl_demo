/*
 * Copyright 1993, 1994, Silicon Graphics, Inc.
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
 *  flight/sound.h $Revision: 1.1 $
 */

#define MAXAUDIOPORT	3

extern short	*cannonsample, *locksample,
		*misslesample, *diesample,
		sound_mode;
extern int	cannonlength, locklength,
		misslelength, dielength;

void	play_samps(short* buffer, int len);
void	play_explosion(float dist);
void	thrust_type(short enginetype);
void	pitch_thrust(float vol);
void	play_thrust(void);
void	init_audio(void);


