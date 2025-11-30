/*
 * Copyright (C) 1992, 1993, 1994, Silicon Graphics, Inc.
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
#include <stdio.h>
#include <fcntl.h>
#ifdef SOUND
#include <audio.h>
#endif
#include <GL/gl.h>
#include "space.h"

#define CHUNK_ID          4
#define CHUNK_HEADER      8
#define FORM_CHUNK       12
#define FORM_CHUNK_DATA   4
#define COMM_CHUNK       26  /* including the header */
#define COMM_CHUNK_DATA  18
#define SSND_CHUNK       16  /* including the header */
#define SSND_CHUNK_DATA   8

typedef struct {
    schar8 id[4];
    uint32 size;
    uint32 nchannels;
    uint32 nsampframes;
    uint32 sampwidth;
    uint32 samprate;
} comm_chunk_t;

typedef struct {
    uint32 offset;
    uint32 blocksize;
    uint32 file_position;  /* not in AIFF file */
    uint32 sample_bytes;   /* not in AIFF file */
} ssnd_chunk_t;

typedef struct {
    uint32 leftover_bytes;
    uint32 leftover_samps;
    uint32 leftover_frames;
    uint32 bytes_per_buf;
    uint32 samps_per_buf;
    uint32 bytes_per_samp;
    uint32 num_bufs;
} play_t;

#ifdef SOUND
static ALport        audio_port;
static schar8        sampbuf[2*2*48000];
#endif

static sint32        fd;
static comm_chunk_t  comm_data;
static ssnd_chunk_t  ssnd_data;
static play_t        play;

/********************************************************************** 
*  sound_effect()  -  
**********************************************************************/
void sound_effect(schar8 *soundfile)

{  uchar8   buf[32];
   sint32   n,pvbuf[2];
#ifdef SOUND
   ALconfig audio_port_config;
    
   if ((fd = open(soundfile, O_RDONLY)) < 0)
     exit(0);

   if (read(fd, buf, CHUNK_HEADER) != CHUNK_HEADER)
     exit(0) ;

   comm_data.id[0] = buf[0];
   comm_data.id[1] = buf[1];
   comm_data.id[2] = buf[2];
   comm_data.id[3] = buf[3];
   comm_data.size = (buf[4]<<24) | (buf[5]<<16) | (buf[6]<<8) | buf[7] ;

   if (strncmp(comm_data.id, "FORM", 4) || comm_data.size <= 0)
     exit(0);

   if (read(fd, buf, FORM_CHUNK_DATA) != FORM_CHUNK_DATA) 
     exit(0);

   /* loop on the local chunks */
   while ((n = read(fd, buf, CHUNK_HEADER)) != 0)  {
     if (n != CHUNK_HEADER)
       exit(0);

     comm_data.id[0] = buf[0];
     comm_data.id[1] = buf[1];
     comm_data.id[2] = buf[2];
     comm_data.id[3] = buf[3];
     comm_data.size = (buf[4]<<24) | (buf[5]<<16) | (buf[6]<<8) | buf[7] ;

     if (!strncmp(comm_data.id, "COMM", 4))  {  /* common */
       if (read(fd, buf, COMM_CHUNK_DATA) != COMM_CHUNK_DATA)
         exit(0);

       comm_data.nchannels   = (buf[0]<<8) | buf[1] ;
       comm_data.nsampframes = (buf[2]<<24) | (buf[3]<<16) | (buf[4]<<8) | buf[5] ;
       comm_data.sampwidth   = (buf[6]<< 8) | buf[7] ;
       comm_data.samprate    = (buf[10]<<8) | buf[11] ;
       }

     else if (!strncmp(comm_data.id, "SSND", 4))  {  /* sound data */
       if (read(fd, buf, SSND_CHUNK_DATA) != SSND_CHUNK_DATA)
         exit(0) ;

       ssnd_data.offset    = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3] ;
       ssnd_data.blocksize = (buf[4]<<24) | (buf[5]<<16) | (buf[6]<<8) | buf[7] ;

       ssnd_data.file_position = lseek(fd, 0, SEEK_CUR);
       ssnd_data.sample_bytes  = comm_data.size - 8;
       lseek(fd, ssnd_data.sample_bytes, SEEK_CUR);
       }

     else if ((!strncmp(comm_data.id, "MARK", 4))  /* marker */
           || (!strncmp(comm_data.id, "INST", 4))  /* instrument */
           || (!strncmp(comm_data.id, "APPL", 4))  /* appl specific */
           || (!strncmp(comm_data.id, "MIDI", 4))  /* midi data */
           || (!strncmp(comm_data.id, "AESD", 4))  /* audio  rec */
           || (!strncmp(comm_data.id, "COMT", 4))  /* comments */
           || (!strncmp(comm_data.id, "NAME", 4))  /* text */
           || (!strncmp(comm_data.id, "AUTH", 4))  /* text */
           || (!strncmp(comm_data.id, "(c) ", 4))  /* text */
           || (!strncmp(comm_data.id, "ANNO", 4))) /* text */   {
       lseek(fd, comm_data.size, SEEK_CUR);
       }

     else exit(0) ;
     } 

   /* set output sample rate */
   pvbuf[0] = AL_OUTPUT_RATE;
   pvbuf[1] = comm_data.samprate;
   ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 2);

   /* configure and open audio port */
   audio_port_config = ALnewconfig();
   ALsetwidth(audio_port_config, comm_data.sampwidth>>3);
   ALsetchannels(audio_port_config, comm_data.nchannels);
   audio_port = ALopenport("space", "w", audio_port_config);

   /* make the buffer large enough to hold 1/2 sec of audio frames */
   play.bytes_per_samp  = comm_data.sampwidth>>3;
   play.bytes_per_buf   = play.bytes_per_samp * comm_data.nchannels * comm_data.samprate / 2;
   play.samps_per_buf   = play.bytes_per_buf / play.bytes_per_samp;
   play.leftover_bytes  = ssnd_data.sample_bytes % play.bytes_per_buf;
   play.leftover_samps  = play.leftover_bytes / play.bytes_per_samp;
   play.leftover_frames = play.leftover_samps / comm_data.nchannels;
   play.num_bufs        = ssnd_data.sample_bytes / play.bytes_per_buf;

   play_file();

/*
   for (n=400; n>0; n--)
     play_sample(n) ;
*/
#endif
}

/********************************************************************** 
*  play_file()  -  
**********************************************************************/
static void play_file(void)

{  register sint32 i;

#ifdef SOUND
   /* move the fileptr to the beginning of the sample data */
   lseek(fd, ssnd_data.file_position, SEEK_SET);

   for (i=0; i<play.num_bufs; i++) {
     read(fd, sampbuf, play.bytes_per_buf);
     ALwritesamps(audio_port, sampbuf, play.samps_per_buf);
     }

   read(fd, sampbuf, play.leftover_bytes);
   ALwritesamps(audio_port, sampbuf, play.leftover_samps);

   while (ALgetfilled(audio_port) > 0)
     sginap(1);
#endif
}

/********************************************************************** 
*  play_sample()  -  
**********************************************************************/
static void play_sample(uint32 index)

{  uint32 pos = ssnd_data.file_position + index*play.bytes_per_buf ;
  
#ifdef SOUND
   lseek(fd, pos, SEEK_SET);

   if (index < play.num_bufs)  {
     read(fd, sampbuf, play.bytes_per_buf);
reverse_buffer(sampbuf) ;
     ALwritesamps(audio_port, sampbuf, play.samps_per_buf);
     }
   else  {
     read(fd, sampbuf, play.leftover_bytes);
     ALwritesamps(audio_port, sampbuf, play.leftover_samps);
     }
#endif
}

/********************************************************************** 
*  reverse_buffer()  - 
**********************************************************************/
static void reverse_buffer(uchar8 *buf)

{  register uint16 *s1, *s2, tmp ;
   register uint32 i,count ;

   s1 = (uint16 *)buf ;
   s2 = (uint16 *) (uchar8 *) (buf + play.bytes_per_buf - 4);
   count = play.bytes_per_buf >> 3 ;

   for (i=0; i<count; s1++,s2--,i++)  {
     tmp = *s1 ;
     *s1 = *s2 ;
     *s2 = tmp ;
     }
}

