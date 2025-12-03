/*
 * Copyright 1992, 1993, 1994, Silicon Graphics, Inc.
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
#ifndef __AIFFLIB_H
#define	__AIFFLIB_H

#define	AIFF_NOMEM		-1	/* too many files open */
#define	AIFF_OPENFAILURE	-2	/* can't open a file (check errno) */
#define	AIFF_BADFD		-3	/* bad file descriptor */
#define	AIFF_BADDATASIZE	-4	/* AIFF file has invalid data size */
#define	AIFF_BADHEADER		-5	/* AIFF file has a bad header */
#define	AIFF_NOTAIFF		-6	/* doesn't appear to be AIFF file */
#define	AIFF_NOFORMCHUNK	-7	/* can't find the FORM header */
#define	AIFF_BADCHUNK		-8	/* file is corrupt */
#define	AIFF_WRITEFAILURE	-9	/* can't write to file */
#define	AIFF_PARAMSFIXED	-10	/* can't change audio parameters */
#define	AIFF_BADCHANNELS	-11	/* unsupported number of channels */
#define	AIFF_BADWIDTH		-12	/* unsupported sample width (bits) */
#define	AIFF_BADRATE		-13	/* unsupported sample rate */

#ifdef __cplusplus
extern "C" {
#endif

typedef	int	AIFFfile;

/* all the functions returning int return -1 on error and set AIFFerrno */

AIFFfile	AIFFopen(const char* filename, const char* dir);
int	AIFFclose(AIFFfile fd);

/* accepts and returns number of samples not bytes */
int	AIFFwrite(AIFFfile fd, const void* buf, unsigned nsamp);
int	AIFFread(AIFFfile fd, void* buf, unsigned nsamp);
int	AIFFgetlength(AIFFfile fd);

int	AIFFgetchannels(AIFFfile fd);	/* returns 1 or 2 */
int	AIFFgetwidth(AIFFfile fd);	/* returns 8, 16, or 24 */
int	AIFFgetrate(AIFFfile fd);	/* returns 48000, 44100, 32000, etc. */
int	AIFFsetchannels(AIFFfile fd, int channels);	/* takes 1 or 2 */
int	AIFFsetwidth(AIFFfile fd, int width);	/* takes 8, 16, or 24 */
int	AIFFsetrate(AIFFfile fd, int rate);	/* takes 48000, 44100, etc. */

/*
 * functions to convert AL defines (AL_RATE_48000, AL_STEREO, etc.) to
 * real numbers (48000, 2, etc.)
 */
int	CONVERTchannelstoAL(int nchannels);
int	CONVERTwidthtoAL(int width);
int	CONVERTratetoAL(int rate);
int	CONVERTALtochannels(int ALnchannels);
int	CONVERTALtowidth(int ALwidth);
int	CONVERTALtorate(int ALrate);

extern int	AIFFerrno;

void	AIFFerror(const char *s);
char*	AIFFstrerror(int err);

#ifdef __cplusplus
}
#endif

#endif

