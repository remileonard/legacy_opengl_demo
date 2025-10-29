/*
 * sound.c                         (including aifflib by Chris Schoeneman)
 *
 * OK - here is the stuff specifically written for _cycles_.
 * These routines are the only ones we will be calling from cycles.
 * The aifflib code is at the end,  and is clearly marked,  but we don't
 * need to fiddle with it.
 * 
 * The design of the following code is based on the sound code in _flight_
 * provided with the source CD. Let me tell you straight that this is
 * my first attempt at audio stuff and is a pretty lame hack, so don't get
 * pissed at me if you think you can do better - just do it, be my guest.
 *
 * Nick Fitton 18/01/1994
 * 
 *          play_sound() - general routine to play an aiff soundfile.
 *    play_cycle_sound() - play cycle engine note.
 *   pitch_cycle_sound() - set the pitch of the cycle engine.
 *     proximity_alert() - check for and play a proximity alert
 *    init_sound_flags() - initialize all the flags so SFX will happen
 *          init_audio() - start up the audio h/w and read the aiff's
 *         close_audio() - clean up your mess boy!
 */
 
/*
 * DEL_BUF needs to be small enough to allow the pitch to change
 * in real time, but large enough to provide a continous noise when
 * the games slows down (machine is slow or background job starts up.
 * It's a compromise, and I prefer too large... shit sound is better
 * than no sound (to quote Robin Humble)
 */
#define DEL_BUFF 4000

#include <stdio.h>
#include <stdlib.h> /* for getenv() */
#include <string.h>
#include <errno.h>
#include <math.h>
#include <audio.h>
#include "cycles.h"
#include "sound.h"

/*
 * GLOBALS
 */
static ALport	outport[MAXAUDIOPORT];
static char	portname[5] = "out?",
		soundpath[256], *soundfileptr;
static short	num_open_ports = 0;
static long	cycleparams[2] = { AL_OUTPUT_RATE, AL_RATE_8000 },
		oldparams[2] = { AL_OUTPUT_RATE, 0 };

static short	*audiobuf = 0;

static short	*cyclesound = 0, *pitchcycle = 0, *thrustsample;
static int	cyclelength, thrustlength=0;
short		sound_mode = 2;

/*
 * I fork a playaiff 'coz it seems to be faster than ALwritesamps when
 * im sending my channel hogging sound snippets down the wire. Maybe
 * this is 'coz the parent doesn't wait for the sound to play before it
 * get's on with the drawing?
 */
void play_sound(const char *sound)
{
    int pid;
    char temp[128];
   
    if(num_open_ports == 0) return;
   
    /* build path name */
    strcpy(soundfileptr, sound);
    sprintf(temp, "/usr/sbin/playaiff %s &", soundpath);
    system(temp);
}

/*
 * I do the cycle noise like it's done in 'flight' 'coz it's a cool way
 * to interactively change the pitch while we continously dump the sound.
 * BUT
 * There is a catch... you have to make sure that you don't dump for too
 * long at an old pitch value. This depends on the size of the buffer.
 * We have a dynamic buff_thing depending on how fast the machine is running, 
 * so if the machine is an Indy, or has jobs running, the sound is still
 * smooth (or at least consistent with the frame rate). speed_fac is an
 * average of previous frame rates then scaled. see lightcycles.c
 * The result of all this is, if the machine is slow, we can keep the sound
 * continous but not the pitch. If the machine is fast, we have no problems.
 * Nick Fitton & Robin Humble
 */
void    play_cycle_sound(void)
{
  static int sampptr = 0;
  long samplecount;
  extern float speed_fac;
  long buff_thing;

/* calculate our dynamic buff_thing and make sure its even so that
 * both channels are always online. SFX suck if this is odd.
 */
  buff_thing = (long)(DEL_BUFF*speed_fac)&(~(long)1);

  if (sound_mode != 2 || thrustlength == 0) return;
  while (ALgetfilled(outport[0]) < buff_thing) {
    samplecount = thrustlength - sampptr;
    if (samplecount <= buff_thing) {
/* always send an even amount of samples to keep both channels noisy */
      if(samplecount < 2) samplecount=2;
      ALwritesamps(outport[0], pitchcycle+sampptr, samplecount);
      sampptr = 0;
    }
    else {
      ALwritesamps(outport[0], pitchcycle+sampptr, buff_thing);
      sampptr += buff_thing;
    }
  }
}

/* fiddle with these for a different sounding bike */
#define MIN_PITCH 0.5
#define MAX_PITCH 2.0

/*
 * This ones a doosy. We parse 'pitch' which is a measure of the speed
 * of the bike scaled between 0-1. First we rescale pitch,  so fiddle with
 * the MAX_PITCH, MIN_PITCH values if you wanna change the sound.
 * The idea is, as pitch increases, resample the cycle noise but only send
 * every 'step'th sample at a higher volume, to increase the actual pitch
 * of the sound. Make sense?
 * Nick Fitton & Robin Humble
 */
void	pitch_cycle_sound(float pitch)
{
  int	i, index;
  float findex, step, vol;

  if (num_open_ports == 0 || !pitchcycle) return;
  play_cycle_sound();		/* avoid pauses in sound */

  pitch = (MAX_PITCH - MIN_PITCH)*pitch + MIN_PITCH;
  thrustlength = cyclelength;
  thrustsample = cyclesound;

  step = 0.5/pitch;
  vol = (1.0 + pitch)/2.0;  /* softer than explosions */

  thrustlength = (int)((float)thrustlength * step);
  thrustlength &= ~1;
  step = 2.0/step;
  for (findex = 0.0, i = 0; i < thrustlength; i+=2) {
    index = (int)findex & ~1;
    findex += step;
    pitchcycle[i] = thrustsample[index] * vol;
    pitchcycle[i+1] = thrustsample[index+1] * vol;
  }
}

static int	get_aiff_sound(char* filename, short** samplebuffer)
{
  AIFFfile	fd;
  int		len;

  /* build path name */
  strcpy(soundfileptr, filename);

  /* open file, return no length on error */
  if ((fd = AIFFopen(soundpath, "r")) < 0) return 0;

  /* get size of buffer (in samples) and malloc an array */
  len = AIFFgetlength(fd);
  if (len <= 0) { AIFFclose(fd); return 0; }
  *samplebuffer = (short*)malloc(len * 2 * sizeof(short));

  /* read the samples, close the file, return the number of samples */
  len = AIFFread(fd, *samplebuffer, len);
  AIFFclose(fd);
  return len;
}

static void	silentALerror(long errnum, const char* fmt, ...)
{
/* ignore all AL errors */
}


void change_cycle_pitch(float min) {
    float pitch;
    int i;
    extern CYCLE *good, bike[CYCLES];
    extern int used[CYCLES];
#ifdef AUDIO
    extern 
#endif
    short kaboom, boom[CYCLES], still_close, tumbling;

/* calculate pitch of our cycle then play the sound */
    pitch = (float)(good->step - MIN_STEP)/(MAX_STEP - MIN_STEP);
    if(tumbling) pitch = 0;
    if(!kaboom) pitch_cycle_sound(pitch);
    if(!kaboom) play_cycle_sound();

/* reset the boom flag for 'bots being restarted */
    for (i = 0; i < CYCLES; i++)
	if (used[i] && i != good->id && bike[i].falling < 0) boom[i] = 0;

/* sound the proximity alert if we need it */
/* I dont like it, but the code is here if you want it
    if (min < 0.05*DIM){
	if(!still_close) play_sound("alarm.aiff");
	still_close = 1;
    }
    else
	still_close = 0;
*/
}


void init_sound_flags(void)
{
    int i;
#ifdef AUDIO
    extern 
#endif
    short kaboom, boom[CYCLES], still_close, tumbling;
    
    /* init sound flags */
    kaboom=1;
    still_close=0;
    tumbling=0;
    for(i=0; i<CYCLES; i++) boom[i]=0.0;
}

void	init_audio(void)
{
  AIFFfile	fd;
  ALconfig	config;
  char*		path;
  int		i, pathlen;

  if (num_open_ports) return;		/* return if not first call */
  
  /* capture errors so user doesn't see them */
  ALseterrorhandler(silentALerror);

  config = ALnewconfig();
  ALsetqueuesize(config, 64000);
  ALsetchannels(config, AL_STEREO);
  for (i = 0; i < MAXAUDIOPORT; i++) {
    portname[3] = '0' + num_open_ports;
    outport[num_open_ports] = ALopenport(portname, "w", config);
    if (outport[num_open_ports])
      if (++num_open_ports == 10)	/* ten ports max */
	break;
  }

  /* return if audio ports can't be opened.  if the machine
   * doesn't have audio capability we'll return here */
  if (num_open_ports == 0) {
      /* insert lame messages to those suckers without audio here */
      return;
  }

  ALgetparams (AL_DEFAULT_DEVICE, oldparams, 2);
  ALsetparams (AL_DEFAULT_DEVICE, cycleparams, 2);

/*
 * all of this stuff is just for when we win and the game gets
 * distributed with IRIX... :-)
 * oh like, fer sure, in yer dreams dude....!
 */
  path = getenv("CYCLESOUND");

  if (!path || (pathlen = strlen(path)) > 240) { /* no path or path too long */
    /* use default directory */
    strcpy(soundpath, "/usr/demos/IndiZone/.data/cycles/");
    soundfileptr = soundpath + strlen(soundpath);
  }
  else {
    strcpy(soundpath, path);			/* copy path */
    if (soundpath[pathlen-1] != '/')		/* only one trailing / */
	soundpath[pathlen++] = '/';
    soundfileptr = soundpath + pathlen;
    *soundfileptr = 0;
  }

/*
 * read cycle noise from cycle.aiff
 */
  cyclelength = get_aiff_sound("cycle.aiff", &cyclesound);

  /* make buffer to hold pitch shifted cycle sound */
  pitchcycle = (short*) malloc(4 * cyclelength * 2 * sizeof(short));

  if (cyclelength > 0 && !audiobuf)
    audiobuf = (short*) malloc( cyclelength * 2 * sizeof( short));
  else audiobuf = 0;
}

void	close_audio(void)
{
  int i;

  ALsetparams (AL_DEFAULT_DEVICE, oldparams, 2);

  for (i = 0; i < num_open_ports; i++)
    ALcloseport(outport[i]);
}

/*------------------------ start of aifflib code -------------------------*/
/*
 * aifflib --
 *
 *	A library for reading and writing AIFF files
 *
 *		Chris Schoeneman 	- 1991
 *		(borrowed heavily from playaiff and recordaiff)
 */

/* AIFF file typedef's and defines */
typedef struct
{
    long samprate;
    long nchannels;
    long sampwidth;
} audio_params_t;

typedef struct
{
    char id[4];
    long size;
} chunk_header_t;

#define CHUNK_ID     4
#define CHUNK_HEADER 8

typedef struct
{
    chunk_header_t header;
    int file_position;  /* not in AIFF file */
    char type[4]; /* should contain 'AIFF' for any audio IFF file */
} form_chunk_t;

#define FORM_CHUNK      12  /* including the header */ 
#define FORM_CHUNK_DATA 4   

#define COMM_CHUNK      26   /* including the header */
#define COMM_CHUNK_DATA 18

typedef struct
{
    chunk_header_t header;
    int file_position;            /* not in AIFF file */
    short nchannels;
    unsigned long nsampframes;
    short sampwidth;
    long samprate;              /* not in AIFF file */
} comm_chunk_t;


#define SSND_CHUNK      16   /* including the header */
#define SSND_CHUNK_DATA 8

typedef struct
{
    chunk_header_t header;
    unsigned long offset;
    unsigned long blocksize;

    long file_position; /* not in AIFF file */
    long sample_bytes; /* not in AIFF file */
} ssnd_chunk_t;
/* end of AIFF typedef's and defines */

#define	MAXFILES	20

enum { FORM = 0, COMM = 1, SSND = 2};

#define	DEFAULT_CHANNELS	2
#define	DEFAULT_WIDTH		16
#define	DEFAULT_RATE		48000

static int		busy[MAXFILES],
			ssnd_remaining[MAXFILES],
			files = -1;
static long		writepos[MAXFILES][3];
static FILE*		aifffd[MAXFILES];
static audio_params_t	audioparams[MAXFILES];

int		AIFFerrno;
static char	*AIFFerrstr[]={
			"",
			"too many open files",
			"cannot open file",
			"invalid file id",
			"invalid data size",
			"invalid header",
			"not an AIFF file",
			"no FORM header",
			"invalid chunk",
			"write failure",
			"audio parameters are fixed",
			"unsupported number of channels",
			"unsupported sample width",
			"unsupported sample rate"};

static int	widthtobytes(int width);
static double	ConvertFromIeeeExtended(char*);
static void	ConvertToIeeeExtended(double, char*);

/*************************************************************************
 **	routines to read and write the AIFF format chunks		**
 *************************************************************************/
/* convert bytes in big endian order to a short */
static short	align_short(char* buf)
{
  int i;
  union { unsigned char b[sizeof(short)]; short s; } align_short;

  for (i = 0; i < sizeof(short); i++) align_short.b[i] = *buf++;
  return align_short.s;
}

/* convert bytes in big endian order to a long */
static long	align_long(char* buf)
{
  int i;
  union { unsigned char b[sizeof(long)]; long l; } align_long;

  for (i = 0; i < sizeof(long); i++) align_long.b[i] = *buf++;
  return align_long.l;
}

/* convert short to bytes in big endian order */
static void	short_align(char* buf, short s)
{
  int i;
  char *scan = (char*)&s;

  for (i = 0; i < sizeof(short); i++) *buf++ = *scan++;
}

/* convert long to bytes in big endian order */
static void     long_align(char* buf, long l)
{
  int i;
  char *scan = (char*)&l;

  for (i = 0; i < sizeof(long); i++) *buf++ = *scan++;
}

static int      skip_chunk(AIFFfile fd, chunk_header_t* chunk_header)
{
  fseek(aifffd[fd], chunk_header->size, SEEK_CUR);
}

static int	read_chunk_header(AIFFfile fd, chunk_header_t *chunk_header)
{
  char buf[CHUNK_HEADER];
  int i;

  if ((i = fread(buf, 1, CHUNK_HEADER, aifffd[fd])) != CHUNK_HEADER) return i;

  for (i=0; i<4; i++) chunk_header->id[i] = buf[i];
  chunk_header->size = align_long(buf+4);

  return CHUNK_HEADER;
}

static int	read_form_chunk(AIFFfile fd, chunk_header_t *chunk_header,
			form_chunk_t *form_data)
{
  char buf[FORM_CHUNK_DATA];

  if (chunk_header->size < 0) {
    AIFFerrno = AIFF_BADDATASIZE;
    return -1;
  }
  else if (chunk_header->size == 0) {
    AIFFerrno = AIFF_BADDATASIZE;
    return -1;
  }

  if (fread(buf, 1, FORM_CHUNK_DATA, aifffd[fd]) != FORM_CHUNK_DATA) {
    AIFFerrno = AIFF_BADHEADER;
    return -1;
  }
  if (strncmp(buf, "AIFF", 4)) {
    AIFFerrno = AIFF_NOTAIFF;
    return -1;
  }
  return 0;
}

static int	read_comm_chunk(AIFFfile fd, chunk_header_t *chunk_header)
{
  int i;
  char buf[COMM_CHUNK_DATA+1];
  comm_chunk_t comm_data;

  if (fread(buf, 1, COMM_CHUNK_DATA, aifffd[fd]) != COMM_CHUNK_DATA) {
    AIFFerrno = AIFF_BADCHUNK;
    return -1;
  }

  comm_data.nchannels = align_short(buf);
  comm_data.nsampframes = align_long(buf+4);
  comm_data.sampwidth = align_short(buf+6);

    /*
     * the sample rate value from the common chunk is an 80-bit IEEE extended
     * floating point number:
     * [s bit] [15 exp bits (bias=16383)] [64 mant bits (leading 1 not hidden)]
     *
     * turns out we can just grab bytes 2 and 3 (if bytes numbered 0 ... 9)
     * and cast them as an integer: the integer value equals the sample rate
     */
  comm_data.samprate = (long)ConvertFromIeeeExtended(buf+8);

  audioparams[fd].samprate = comm_data.samprate;
  audioparams[fd].nchannels = comm_data.nchannels;
  audioparams[fd].sampwidth = comm_data.sampwidth;

  if (widthtobytes(audioparams[fd].sampwidth) == -1) {
    AIFFerrno = AIFF_BADWIDTH;
    audioparams[fd].sampwidth = DEFAULT_WIDTH;
    return -1;
  }
  return 0;
}

/* AIFFread calls this if there is no more sound data in the current
 * SSND chunk.  It searches for the next block and sets the
 * ssnd_remaining entry to the length of the block */
static void	read_ssnd_chunk(AIFFfile fd)
{
  char buf[SSND_CHUNK_DATA];
  int i;
  chunk_header_t chunk_header;

  /* if no sound left, search for next SSND block */
  /* NOTE: all chunks have been read through and checked for 
   * validity, so we assume read_chunk_header either returns
   * zero for end of file or CHUNK_HEADER for a good block */ 
  while (read_chunk_header(fd, &chunk_header) != 0) {
    if (!strncmp(chunk_header.id, "SSND", 4)) {
      fread(buf, 1, SSND_CHUNK_DATA, aifffd[fd]);
      ssnd_remaining[fd] = chunk_header.size - 2*sizeof(long);
      if (ssnd_remaining[fd] != 0) break;
    }
    else skip_chunk(fd, &chunk_header);
  }
}

static int      write_form_chunk(AIFFfile fd)
{
  char buf[FORM_CHUNK];
  int i;

  strncpy(buf, "FORM", 4);        /* form header id */
  for (i=0; i<sizeof(long); i++)  /* form header size  - do later */
    buf[i+4] = 0x00;
  strncpy(buf+4+sizeof(long), "AIFF", 4);

  writepos[fd][FORM] = ftell(aifffd[fd]);

  if (fwrite(buf, 1, FORM_CHUNK, aifffd[fd]) != FORM_CHUNK)
    { AIFFerrno = AIFF_WRITEFAILURE; return -1; }

  return 0;
}

static int	write_comm_chunk(AIFFfile fd)
{
  char buf[COMM_CHUNK];
  int i;

  strncpy(buf, "COMM", 4);			/* chunk id */
  long_align(buf+4,COMM_CHUNK_DATA);		/* chunk data size */
  short_align(buf+4+sizeof(long),		/* number channels */
	audioparams[fd].nchannels);
  long_align(buf+4+sizeof(long)+sizeof(short),	/* nsample frames */
	0L);
  short_align(buf+4+2*sizeof(long)+sizeof(short), /* sample width */
	audioparams[fd].sampwidth);
  ConvertToIeeeExtended((double)audioparams[fd].samprate, /* sample rate */
	buf + 4 + 2*sizeof(long) + 2*sizeof(short));

  writepos[fd][COMM] = ftell(aifffd[fd]);

  if (fwrite(buf, 1, COMM_CHUNK, aifffd[fd]) != COMM_CHUNK)
    { AIFFerrno = AIFF_WRITEFAILURE; return -1; }

  ssnd_remaining[fd] = 0;

  return 0;
}

static int	write_ssnd_chunk(AIFFfile fd)
{
  char buf[SSND_CHUNK];
  int i;

  strncpy(buf, "SSND", 4);
  for (i=0; i<3*sizeof(long); i++)  /* chunk data size */
	buf[i+4] = 0x00;

  writepos[fd][SSND] = ftell(aifffd[fd]);

  if (fwrite(buf, 1, SSND_CHUNK, aifffd[fd]) != SSND_CHUNK)
    { AIFFerrno = AIFF_WRITEFAILURE; return -1; }

  return 0;
}

static int	update_form_chunk(AIFFfile fd, long total_bytes)
{
  fseek(aifffd[fd], writepos[fd][FORM]+4, SEEK_SET);
  total_bytes -= CHUNK_HEADER;

  if (fwrite(&total_bytes, 1, sizeof(long), aifffd[fd]) != sizeof(long))
    { AIFFerrno = AIFF_WRITEFAILURE; return -1; }

  return 0;
}

static int	update_comm_chunk(AIFFfile fd, long sample_frames)
{
  fseek(aifffd[fd], writepos[fd][COMM]+CHUNK_HEADER+sizeof(short), SEEK_SET);

  if (fwrite(&sample_frames, 1, sizeof(long), aifffd[fd]) != sizeof(long))
    { AIFFerrno = AIFF_WRITEFAILURE; return -1; }

  return 0;
}

static int	update_ssnd_chunk(AIFFfile fd, long sample_bytes)
{
  fseek(aifffd[fd], writepos[fd][SSND] + CHUNK_ID, SEEK_SET);
  sample_bytes += SSND_CHUNK_DATA;

  if (fwrite(&sample_bytes, 1, sizeof(long), aifffd[fd]) != sizeof(long))
    { AIFFerrno = AIFF_WRITEFAILURE; return -1; }

  return 0;
}

static AIFFfile	myopen(const char* filename, const char* dir)
{
  int i;
  FILE* fd;

  /* find first available space in table */

  for (i = 0; i < MAXFILES; i++)
    if (!busy[i]) break;
  if (i == MAXFILES) {
    AIFFerrno = AIFF_NOMEM;
    return -1;
  }

  /* open file */

  fd = fopen(filename, dir);
  if (!fd) {
    AIFFerrno = AIFF_OPENFAILURE;
    return -1;
  }

  /* fill table entries */

  aifffd[i] = fd;
  busy[i] = *dir;
  files++;

  return (AIFFfile)i;
}

static int	myclose(AIFFfile fd)
{
  busy[fd] = 0;
  files--;
  return fclose(aifffd[fd]);
}

AIFFfile	AIFFopen(const char* filename, const char* dir)
{
  AIFFfile fd;
  int n;
  chunk_header_t chunk_header;
  form_chunk_t form_data;

  fd = myopen(filename, dir);
  if (fd < 0) return fd;

  switch (dir[0]) {
    case 'r': {
      chunk_header_t chunk_header;
      form_chunk_t form_data;
      long past_header;

      if (read_chunk_header(fd, &chunk_header) != CHUNK_HEADER) {
	myclose(fd);
	AIFFerrno = AIFF_BADHEADER;
	return -1;
      }

      if (strncmp(chunk_header.id, "FORM", 4)) {	/* form container */
	AIFFerrno = AIFF_NOFORMCHUNK;
	myclose(fd);
	return -1;
      }

      if (read_form_chunk(fd, &chunk_header, &form_data) < 0) {
	myclose(fd);
	return -1;
      }

      /* remember where interesting stuff starts */

      past_header = ftell(aifffd[fd]);

      /* search for last COMM chunk */

      while ((n = read_chunk_header(fd, &chunk_header)) != 0) {
	if (n == CHUNK_HEADER) {
	  if (!strncmp(chunk_header.id, "COMM", 4))
	    read_comm_chunk(fd, &chunk_header);
	  else 
	    skip_chunk(fd, &chunk_header);
	}
	else {
	  myclose(fd);
	  AIFFerrno = AIFF_BADHEADER;
	  return -1;
	}
      }

      /* set file pointer to beginning of interesting stuff */

      fseek(aifffd[fd], past_header, SEEK_SET);
      ssnd_remaining[fd] = 0;
      break;
    }
    case 'w': {
      /* write the form chunk */

      if (write_form_chunk(fd) < 0) {
	myclose(fd);
	return -1;
      }

      /* set ssnd_remaining to a bogus value as a flag to write the COMM */

      ssnd_remaining[fd] = -1;

      /* set default values for audio parameters */
      AIFFsetchannels(fd,0);
      AIFFsetwidth(fd,0);
      AIFFsetrate(fd,0);

      break;
    }
  }

  return fd;
}

int	AIFFclose(AIFFfile fd)
{
  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  if (busy[fd] == 'w') {
    long total = ftell(aifffd[fd]);

    if (update_form_chunk(fd, total) < 0)
      goto badwrite;
    total -= FORM_CHUNK + COMM_CHUNK + SSND_CHUNK;
    if (update_comm_chunk(fd, total / AIFFgetchannels(fd) 
		/ widthtobytes(AIFFgetwidth(fd))) < 0)
      goto badwrite;
    if (update_ssnd_chunk(fd, total) < 0)
      goto badwrite;
  }

  return myclose(fd);

badwrite:
  myclose(fd);
  return -1;
}

int	AIFFwrite(AIFFfile fd, const void* buf, unsigned nsamp)
{
  int bytes_written;
  unsigned nbyte;

  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  /* write COMM and SSND block headers if necessary */

  if (ssnd_remaining[fd] == -1) {
    if (write_comm_chunk(fd) < 0)
      { AIFFerrno = AIFF_WRITEFAILURE; return -1; }
    if (write_ssnd_chunk(fd) < 0)
      { AIFFerrno = AIFF_WRITEFAILURE; return -1; }
  }

  /* convert number of samples to number of bytes */
  nbyte = nsamp * widthtobytes(audioparams[fd].sampwidth);

  bytes_written = fwrite(buf, 1, nbyte, aifffd[fd]);

  /* convert bytes to samples */
  return bytes_written / widthtobytes(audioparams[fd].sampwidth);
}

int	AIFFread(AIFFfile fd, void* buf, unsigned nsamp)
{
  int bytes_read, nbyte;

  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  /* skip to next SSND block if necessary */

  if (ssnd_remaining[fd] == 0) read_ssnd_chunk(fd);

  /* convert number samples to number bytes */
  nbyte = nsamp * widthtobytes(audioparams[fd].sampwidth);

  /* get samples */
  /* FIXME:  AIFFread should always attempt to fill the buffer
   *		even if has to read the next SSND block to
   *		do it (and the one after that, etc.) */

  if (ssnd_remaining[fd] < nbyte)
    bytes_read = fread(buf, 1, ssnd_remaining[fd], aifffd[fd]);
  else
    bytes_read = fread(buf, 1, nbyte, aifffd[fd]);
  ssnd_remaining[fd] -= bytes_read;

  /* convert bytes to samples */
  return bytes_read / widthtobytes(audioparams[fd].sampwidth);
}

int	AIFFgetlength(AIFFfile fd)
{
  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  /* skip to next SSND block if necessary */
  if (ssnd_remaining[fd] == 0) read_ssnd_chunk(fd);

  return ssnd_remaining[fd] / widthtobytes(audioparams[fd].sampwidth);
}

int	AIFFgetchannels(AIFFfile fd)
{
  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  return audioparams[fd].nchannels;
}

int	AIFFgetwidth(AIFFfile fd)
{
  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  return audioparams[fd].sampwidth;
}

int	AIFFgetrate(AIFFfile fd)
{
  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  return audioparams[fd].samprate;
}

int	AIFFsetchannels(AIFFfile fd, int channels)
{
  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  /* make sure file is write and we haven't written any sound yet */

  if (ssnd_remaining[fd] != -1)
    { AIFFerrno = AIFF_PARAMSFIXED; return -1; }

  if (channels == 0) channels = DEFAULT_CHANNELS;
  audioparams[fd].nchannels = channels;

  return 0;
}

int	AIFFsetwidth(AIFFfile fd, int width)
{
  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  /* make sure file is write and we haven't written any sound yet */

  if (ssnd_remaining[fd] != -1)
    { AIFFerrno = AIFF_PARAMSFIXED; return -1; }

  if (width == 0) width = DEFAULT_WIDTH;
  audioparams[fd].sampwidth = width;

  return 0;
}

int	AIFFsetrate(AIFFfile fd, int rate)
{
  if (fd < 0 || fd >= MAXFILES || !busy[fd])
    { AIFFerrno = AIFF_BADFD; return -1; }

  /* make sure file is write and we haven't written any sound yet */

  if (ssnd_remaining[fd] != -1)
    { AIFFerrno = AIFF_PARAMSFIXED; return -1; }

  if (rate == 0) rate = DEFAULT_RATE;
  audioparams[fd].samprate = rate;

  return 0;
}

static int	widthtobytes(int width)
{
  switch (width) {
    case 8: return 1;
    case 16: return 2;
    case 24: return 4;		/* FIXME: is this 3 or 4 */
    default: return -1;
  }
}

int	CONVERTchannelstoAL(int channels)
{
  switch (channels) {
    case 1: return AL_MONO;
    case 2: return AL_STEREO;
    default: AIFFerrno = AIFF_BADCHANNELS; return -1;
  }
}

int	CONVERTALtochannels(int ALchannels)
{
  switch (ALchannels) {
    case AL_MONO: return 1;
    case AL_STEREO: return 2;
    default: AIFFerrno = AIFF_BADCHANNELS; return -1;
  }
}

int	CONVERTwidthtoAL(int width)
{
  switch (width) {
    case 8: return AL_SAMPLE_8;
    case 16: return AL_SAMPLE_16;
    case 24: return AL_SAMPLE_24;
    default: AIFFerrno = AIFF_BADWIDTH; return -1;
  }
}

int	CONVERTALtowidth(int ALwidth)
{
  switch (ALwidth) {
    case AL_SAMPLE_8: return 8;
    case AL_SAMPLE_16: return 16;
    case AL_SAMPLE_24: return 24;
    default: AIFFerrno = AIFF_BADWIDTH; return -1;
  }
}

int	CONVERTratetoAL(int rate)
{
  switch (rate) {
    case 48000: return AL_RATE_48000;
    case 44100: return AL_RATE_44100;
    case 32000: return AL_RATE_32000;
    case 22050: return AL_RATE_22050;
    case 16000: return AL_RATE_16000;
    case 11025: return AL_RATE_11025;
    case 8000: return AL_RATE_8000;
    default: AIFFerrno = AIFF_BADRATE; return -1;
  }
}

int	CONVERTALtorate(int ALrate)
{
  switch (ALrate) {
    case AL_RATE_48000: return 48000;
    case AL_RATE_44100: return 44100;
    case AL_RATE_32000: return 32000;
    case AL_RATE_16000: return 16000;
    case AL_RATE_8000: return 8000;
    default: AIFFerrno = AIFF_BADRATE; return -1;
  }
}

void	AIFFerror(const char* s)
{
  if (s && *s) fprintf(stderr,"%s: ",s);
  fprintf(stderr,"%s\n",AIFFerrstr[-AIFFerrno]);
  AIFFerrno = 0;
}

char*	AIFFstrerror(int err)
{
  return AIFFerrstr[-err];
}

/*
 * Copyright (C) 1988-1991 Apple Computer, Inc.
 * All rights reserved.
 *
 * Machine-independent I/O routines for IEEE floating-point numbers.
 *
 * NaN's and infinities are converted to HUGE_VAL or HUGE, which
 * happens to be infinity on IEEE machines.  Unfortunately, it is
 * impossible to preserve NaN's in a machine-independent way.
 * Infinities are, however, preserved on IEEE machines.
 *
 * These routines have been tested on the following machines:
 *    Apple Macintosh, MPW 3.1 C compiler
 *    Apple Macintosh, THINK C compiler
 *    Silicon Graphics IRIS, MIPS compiler
 *    Cray X/MP and Y/MP
 *    Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 */
#ifndef HUGE_VAL
# define HUGE_VAL HUGE
#endif

# define UnsignedToFloat(u)    \
     (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)
/* stuffed ...
# define FloatToUnsigned(f)  \
    ((unsigned long)(((long)(f - 2147483648.0)) + 2147483647L + 1))
*/
/****************************************************************
 * Extended precision IEEE floating-point conversion routines.
 ****************************************************************/

static double	ConvertFromIeeeExtended(char *bytes)
{
    double    f;
    long    expon;
    unsigned long hiMant, loMant;

    expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hiMant    =    ((unsigned long)(bytes[2] & 0xFF) << 24)
            |    ((unsigned long)(bytes[3] & 0xFF) << 16)
            |    ((unsigned long)(bytes[4] & 0xFF) << 8)
            |    ((unsigned long)(bytes[5] & 0xFF));
    loMant    =    ((unsigned long)(bytes[6] & 0xFF) << 24)
            |    ((unsigned long)(bytes[7] & 0xFF) << 16)
            |    ((unsigned long)(bytes[8] & 0xFF) << 8)
            |    ((unsigned long)(bytes[9] & 0xFF));

    if (expon == 0 && hiMant == 0 && loMant == 0) {
        f = 0;
    }
    else {
        if (expon == 0x7FFF) {    /* Infinity or NaN */
            f = HUGE_VAL;
        }
        else {
            expon -= 16383;
            f  = ldexp(UnsignedToFloat(hiMant), expon-=31);
            f += ldexp(UnsignedToFloat(loMant), expon-=32);
        }
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}

static void	ConvertToIeeeExtended(double num, char *bytes)
{
    int    sign;
    int expon;
    double fMant, fsMant;
    unsigned long hiMant, loMant;

    if (num < 0) {
        sign = 0x8000;
        num *= -1;
    } else {
        sign = 0;
    }

    if (num == 0) {
        expon = 0; hiMant = 0; loMant = 0;
    }
    else {
        fMant = frexp(num, &expon);
        if ((expon > 16384) || !(fMant < 1)) {    /* Infinity or NaN */
            expon = sign|0x7FFF; hiMant = 0; loMant = 0; /* infinity */
        }
        else {    /* Finite */
            expon += 16382;
            if (expon < 0) {    /* denormalized */
                fMant = ldexp(fMant, expon);
                expon = 0;
            }
            expon |= sign;
            fMant = ldexp(fMant, 32);
            fsMant = floor(fMant);
printf("aifflib: this \"FloatToUnsigned\" routine is stuffed... commented out\n");
/*            hiMant = FloatToUnsigned(fsMant); */
            fMant = ldexp(fMant - fsMant, 32);
            fsMant = floor(fMant);
/*            loMant = FloatToUnsigned(fsMant); */
        }
    }

    bytes[0] = expon >> 8;
    bytes[1] = expon;
    bytes[2] = hiMant >> 24;
    bytes[3] = hiMant >> 16;
    bytes[4] = hiMant >> 8;
    bytes[5] = hiMant;
    bytes[6] = loMant >> 24;
    bytes[7] = loMant >> 16;
    bytes[8] = loMant >> 8;
    bytes[9] = loMant;
}
/*-------------------------- end of aifflib code -------------------------*/
