
#include "../porting/iris2ogl.h"
#include "cycles.h"

#ifndef MIN
#define MIN(x, y)  ((x)<(y)?(x):(y))
#endif

extern float speed_fac;

/*
 * attempt to set a scaling factor so that all
 * cycles move at the same speed, even on way faster machines
 */
void set_speed_fac(int starting) {
    static int count;
    static clock_t init_ticks, old_ticks[5], prev_ticks;
    clock_t now;
    int i, real_count;
    float last5_av, all_av, this_time, predict_time;
    struct tms t;

    /*
     * REAL_SPEED is the dist a cycle should move in one second
     * frame_ticks/HZ is the seconds it took to draw the last frame
     * so speed_fac = (frame_ticks/HZ)*REAL_SPEED
     */

    now = times(&t);

    if (starting) {
	count = 1;
	init_ticks = now;
	prev_ticks = now;

	old_ticks[0] = 20;    /* no idea so guess these... */
	speed_fac = 1.0;
	return;
    }

    count++;
    real_count = MIN(5, count);

    /* shuffle down last times */
    if (count >= 5)
	for (i = 0; i < 5-1; i++) old_ticks[i] = old_ticks[i+1];
    old_ticks[real_count-1] = now - prev_ticks;

    /* find average of last 5 loops, and all prev loops */
    last5_av = 0.0;
    for (i = 0; i < real_count; i++) last5_av += (float)old_ticks[i];

    /* convert all to seconds */
    this_time = (float)(now - prev_ticks)/(float)(HZ);
    last5_av /= (float)(HZ*real_count);
    all_av = (float)(now - init_ticks)/(float)(HZ*(count - 1));

    /*
     * some linear combination formula to hopefully get over
     * local and network hiccups
     */
    predict_time = 0.5*this_time + 0.2*last5_av + 0.1*all_av;

    /* set the speed_fac for this frame based upon the last time taken */
    speed_fac = REAL_SPEED*predict_time;

#ifdef DEBUG
printf("times: %d, last_ticks %d, speed_fac %g (real_count %d)\n",
 count, now - prev_ticks, speed_fac, real_count);
#endif

    prev_ticks = now;

#if 0
/* weird but cute */ {
int i;
extern CYCLE *good, bike[CYCLES];
  for (i = 0; i < CYCLES; i++)
     if (i*40 == count%200 && i != good->id) {
	bike[i].fall++;
	bike[i].falling++;
	bike[i].who_we_hit = -1;
    }
}
#endif
}
