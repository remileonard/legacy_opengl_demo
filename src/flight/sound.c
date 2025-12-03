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
 *  flight/sound.c $Revision: 1.1 $
 */

#ifdef AUDIO

#include "sound.h"
#include "aifflib.h"
#include "audio.h"
#include "flight.h"
#include "malloc.h"
#include "math.h"
#include "stdlib.h"
#include <stdio.h>

static ALport outport[MAXAUDIOPORT];
static char portname[5] = "out?", soundpath[256], *soundfileptr;
static short num_open_ports = 0;
static long flightparams[2] = {AL_OUTPUT_RATE, AL_RATE_16000}, oldparams[2] = {AL_OUTPUT_RATE, 0};

static short *audiobuf = 0;
short *misslesample = 0, *cannonsample = 0, *diesample = 0, *locksample = 0;
int misslelength, cannonlength, dielength, locklength;
static short enginetype = 1, *explosionsample = 0, *jetthrust = 0, *pitchthrust = 0, *propthrust = 0, *thrustsample;
static int explosionlength, jetthrustlength, propthrustlength, thrustlength;
short sound_mode = 2;

void play_samps(short *buffer, int length) {
    int i, bestp, samps, smin;

    /*
     *  don't play if no good buffers or sound_mode not set to play
     */
    if (!length || !num_open_ports || sound_mode == 0)
        return;

    /*
     *  find the most empty audio port
     */
    if (sound_mode == 2)
        bestp = 1;
    else
        bestp = 0;
    smin = ALgetfilled(outport[bestp]);
    for (i = bestp + 1; i < num_open_ports; i++) {
        samps = ALgetfilled(outport[i]);
        if (samps == 0) {
            bestp = i; /* empty, so use it */
            break;
        }
        if (samps < smin) {
            smin = samps;
            bestp = i;
        }
    }

    ALwritesamps(outport[bestp], buffer, length);
}

void play_explosion(float dist) {
    int i;

    if (!explosionlength || dist <= 0.0)
        return;
    dist /= 5000000.0;
    if (dist < 1.0)
        dist = 1.0;
    else
        dist = 1.0 / dist;

    /*
     *  change amplitude by 1/distance**2
     */
    for (i = 0; i < explosionlength; i++)
        audiobuf[i] = (short)(dist * (float)explosionsample[i]);

    /*
     *  write samples
     */
    play_samps(audiobuf, explosionlength);
}

void thrust_type(short engine) { enginetype = (engine != 0); }

void play_thrust(void) {
    static int sampptr = 0;
    static int old_thrustlength = 0;

    if (thrustlength != old_thrustlength) {
        sampptr = 0;
        old_thrustlength = thrustlength;
    }

    if (sound_mode != 2 || thrustlength == 0)
        return;

    while (ALgetfilled(outport[0]) < 24000) {
        if (thrustlength - sampptr <= 24000) {
            ALwritesamps(outport[0], pitchthrust + sampptr, thrustlength - sampptr);
            sampptr = 0;
        } else {
            ALwritesamps(outport[0], pitchthrust + sampptr, 24000);
            sampptr += 24000;
        }
    }
}

void pitch_thrust(float pitch) {
    int i, index;
    float findex, step, vol;

    if (num_open_ports == 0 || !pitchthrust)
        return;

    play_thrust(); /* avoid pauses in sound */

    if (pitch == 0.0) {
        thrustlength = 0;
        return;
    }
    if (enginetype) {
        thrustlength = jetthrustlength;
        thrustsample = jetthrust;
    } else {
        thrustlength = propthrustlength;
        thrustsample = propthrust;
    }
    step = 4.0 - 3.0 * pitch;
    vol = 1.0 + pitch;
    thrustlength = (int)((float)thrustlength * step);
    thrustlength &= ~1;
    step = 2.0 / step;
    for (findex = 0.0, i = 0; i < thrustlength; i += 2) {
        index = (int)findex & ~1;
        findex += step;
        pitchthrust[i] = thrustsample[index] * vol;
        pitchthrust[i + 1] = thrustsample[index + 1] * vol;
    }
}

static int get_aiff_sound(char *filename, short **samplebuffer) {
    AIFFfile fd;
    int len;

    /*
     *  build path name
     */
    strcpy(soundfileptr, filename);

    /*
     *  open file, return no length on error
     */
    if ((fd = AIFFopen(soundpath, "r")) < 0) {
        if (strcmp("jthrust.aiff", filename) && /* XXX remove when */
            strcmp("pthrust.aiff", filename))   /* good thrust files */
        {                                       /* exist */
            fprintf(stderr, "Can't open \"%s\"\n", soundpath);
            return 0;
        }
    }

    /*
     *  get size of buffer (in samples) and malloc an array
     */
    len = AIFFgetlength(fd);
    if (len <= 0) {
        AIFFclose(fd);
        return 0;
    }
    *samplebuffer = (short *)malloc(len * 2 * sizeof(short));

    /*
     *  read the samples, close the file, return the number of samples
     */
    len = AIFFread(fd, *samplebuffer, len);
    AIFFclose(fd);
    return len;
}

static void flightALerror(long errcode, const char *str, ...) {
    /*
     *  don't do anything, we just want to prevent errors getting displayed
     */
}

void init_audio(void) {
    AIFFfile fd;
    ALconfig config;
    char *path;
    int i, pathlen;

    /*
     *  return if not first call
     */
    if (num_open_ports)
        return;

    /*
     *  capture errors so user doesn't see them
     */
    ALseterrorhandler(flightALerror);

    config = ALnewconfig();
    ALsetqueuesize(config, 64000);
    ALsetchannels(config, AL_STEREO);
    for (i = 0; i < MAXAUDIOPORT; i++) {
        portname[3] = '0' + num_open_ports;
        outport[num_open_ports] = ALopenport(portname, "w", config);
        if (outport[num_open_ports])
            if (++num_open_ports == 10) /* ten ports max */
                break;
    }

    /*
     *  return if audio ports can't be opened.  if the machine
     *  doesn't have audio capability we'll return here
     */
    if (num_open_ports == 0)
        return;

    ALgetparams(AL_DEFAULT_DEVICE, oldparams, 2);
    ALsetparams(AL_DEFAULT_DEVICE, flightparams, 2);

    /*
     *  get path for sound samples
     */
    path = getenv("FLIGHTSOUND");

    if (!path || (pathlen = strlen(path)) > 240) {
        /*
         *  no path or path too long
         *  use default directory
         */
        strcpy(soundpath, sounddir);
        soundfileptr = soundpath + strlen(soundpath);
    } else {
        strcpy(soundpath, path);           /* copy path */
        if (soundpath[pathlen - 1] != '/') /* only one trailing / */
            soundpath[pathlen++] = '/';
        soundfileptr = soundpath + pathlen;
        *soundfileptr = 0;
    }

    if (!cannonsample)
        cannonlength = get_aiff_sound("cannon.aiff", &cannonsample);
    if (!diesample)
        dielength = get_aiff_sound("die.aiff", &diesample);
    if (!explosionsample)
        explosionlength = get_aiff_sound("explosion.aiff", &explosionsample);
    if (!locksample)
        locklength = get_aiff_sound("lock.aiff", &locksample);
    if (!misslesample)
        misslelength = get_aiff_sound("missle.aiff", &misslesample);
    if (!jetthrust)
        jetthrustlength = get_aiff_sound("jthrust.aiff", &jetthrust);
    if (!propthrust)
        propthrustlength = get_aiff_sound("pthrust.aiff", &propthrust);

    /*
     *  make buffer to hold pitch shifted thrust sound
     */
    if (propthrustlength > jetthrustlength)
        pitchthrust = (short *)malloc(4 * propthrustlength * 2 * sizeof(short));
    else if (jetthrustlength > 0)
        pitchthrust = (short *)malloc(4 * jetthrustlength * 2 * sizeof(short));
    else
        pitchthrust = 0;

    if (explosionlength > 0 && !audiobuf)
        audiobuf = (short *)malloc(explosionlength * 2 * sizeof(short));
    else
        audiobuf = 0;
}

void close_audio(void) {
    int i;

    ALsetparams(AL_DEFAULT_DEVICE, oldparams, 2);
    for (i = 0; i < num_open_ports; i++)
        ALcloseport(outport[i]);
}

#endif
