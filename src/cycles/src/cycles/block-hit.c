
#include "../porting/iris2ogl.h"
#include "cycles.h"

#ifndef MIN
#define MIN(x, y)  ((x)<(y)?(x):(y))
#endif
#define SIGN(x)   ( (x)<0.0 ? -1.0 : ((x)>0.0?1.0:0.0) )

extern CYCLE *good, bike[CYCLES];
extern float vec[4][2];
extern int used[CYCLES];

/* welcome to the function from hell */

#define no_HALF_DEBUG
#define no_DEBUG_4

#ifdef DEBUG
#define HALF_DEBUG
#define VERY_DEBUG
#define BAD 1
#endif

#ifdef HALF_DEBUG
int routine, one, two, three;
#endif

/*
 * This routine finds the closest obstacle from (x, z) in
 * direction (in the direction vector) dir
 */
float block(int id, float x, float z, int dir, int *close_id) {
    float min, dist, a, b, c, nose;
    float dist2, a2;
    int i, j, d, e, f, level;

    level = bike[id].level;
    *close_id = -1;	/* the trail id we're heading for */

#ifdef DEBUG
#ifdef VERY_DEBUG
if (id == good->id) printf("this is GOOD:\n");
#else
if (id == good->id)
#endif
 printf("id %d good (%g, %g) bad (%g, %g)\n",
  id, good->origin.x, good->origin.z, bike[BAD].origin.x, bike[BAD].origin.z);
#endif

#ifdef HALF_DEBUG
routine = -1;
one = 0;
two = 0;
three = 0;
#endif

    if (vec[dir][0] == 0) {	/* we're looking in a z direction */
	min = DIM - vec[dir][1]*z;

	/* collide with "good" trail */
	i = good->trail_ptr;
	do {
	    dist = good->trail[i].z - z;
	    a = SIGN(dist);
	    b = good->trail[i].x;
	    i = (i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
	    if (good->trail[i].level == level) {
#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("x good ptr %d i %d dist %g\n", good->trail_ptr, i, dist);
#endif
		dist2 = good->trail[i].z - z;
		a2 = SIGN(dist2);
		if (f = (a*a2 < 0.0)) dist = 0.0;
		if ( (a == vec[dir][1] || f) && good->trail[i].level == level && i != good->trail_ptr) {
		    c = good->trail[i].x;
		    if (b > c) {
			d = (x - CYCLE_WIDTH <= b);
			e = (x + CYCLE_WIDTH >= c);
		    }
		    else {
			d = (x + CYCLE_WIDTH >= b);
			e = (x - CYCLE_WIDTH <= c);
		    }
		    if (d && e) {
			dist = ABS(dist);
			if (dist < min) {
			    min = dist;
			    *close_id = good->id;
			}
#ifdef HALF_DEBUG
routine = 0;
one = good->id;
two = good->trail_ptr;
three = i;
#endif
		    }
		}
	    }
	} while (i != good->trail_ptr && good->trail[i].level != -1);

	/* collide with "good" bike and leading trail */
	if (id != good->id && good->level == level) {
	    if (good->falling != 0)
		nose = 0.0;
	    else
		nose = CYCLE_NOSE;
	    i = good->trail_ptr;
	    dist = good->trail[i].z - z;
	    a = SIGN(dist);
	    dist2 = good->origin.z + nose*vec[good->vec_ptr][1] - z;
	    a2 = SIGN(dist2);
	    if (f = (a*a2 < 0.0)) dist = 0.0;
	    if (a == vec[dir][1] || f) {
		dist = MIN(a*dist, a2*dist2);
		b = good->trail[i].x;
		c = good->origin.x;
		if (b > c) {
		    d = (x - CYCLE_WIDTH <= b);
		    e = (x + CYCLE_WIDTH >= c);
		}
		else {
		    d = (x + CYCLE_WIDTH >= b);
		    e = (x - CYCLE_WIDTH <= c);
		}
		if (d && e) {
		    dist = ABS(dist);
		    if (dist < min) {
			min = dist;
			*close_id = good->id;
		    }
#ifdef HALF_DEBUG
routine = 1;
one = good->id;
two = good->trail_ptr;
three = i;
#endif
		}
	    }
	}
#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("x end of good: dist %g\n", dist);
#endif

	/* collide with "bad" trail */
	for (j = 0; j < CYCLES; j++) {
	    if (used[j] && j != good->id && bike[j].alive && bike[j].falling >= 0) {
		i = bike[j].trail_ptr;
		do {
		    dist = bike[j].trail[i].z - z;
		    a = SIGN(dist);
		    b = bike[j].trail[i].x;
		    i = (i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
		    if (bike[j].trail[i].level == level) {
#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("x bad  i %d dist %g\n", i, dist);
#endif
			dist2 = bike[j].trail[i].z - z;
			a2 = SIGN(dist2);
			if (f = (a*a2 < 0.0)) dist = 0.0;
			if ( (a == vec[dir][1] || f) && bike[j].trail[i].level == level && i != bike[j].trail_ptr) {
			    c = bike[j].trail[i].x;
			    if (b > c) {
				d = (x - CYCLE_WIDTH <= b);
				e = (x + CYCLE_WIDTH >= c);
			    }
			    else {
				d = (x + CYCLE_WIDTH >= b);
				e = (x - CYCLE_WIDTH <= c);
			    }
			    if (d && e) {
				dist = ABS(dist);
				if (dist < min) {
				    min = dist;
				    *close_id = bike[j].id;
				}
#ifdef HALF_DEBUG
routine = 2;
one = bike[j].id;
two = bike[j].trail_ptr;
three = i;
#endif
			    }
			}
		    }
		} while (i != bike[j].trail_ptr && bike[j].trail[i].level != -1);

#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("x bad pre last seg: dist %g\n", dist);
#endif
		/* collide with "bad" bike and leading trail */
		if (id != bike[j].id && bike[j].level == level) {
		    if (bike[j].falling != 0)
			nose = 0.0;
		    else
			nose = CYCLE_NOSE;
		    i = bike[j].trail_ptr;
		    dist = bike[j].trail[i].z - z;
		    a = SIGN(dist);
		    dist2 = bike[j].origin.z + nose*vec[bike[j].vec_ptr][1] - z;
		    a2 = SIGN(dist2);
		    if (f = (a*a2 < 0.0)) dist = 0.0;
		    if (a == vec[dir][1] || f) {
			dist = MIN(a*dist, a2*dist2);
			b = bike[j].trail[i].x;
			c = bike[j].origin.x + nose*vec[bike[j].vec_ptr][0];
			if (b > c) {
			    d = (x - CYCLE_WIDTH <= b);
			    e = (x + CYCLE_WIDTH >= c);
			}
			else {
			    d = (x + CYCLE_WIDTH >= b);
			    e = (x - CYCLE_WIDTH <= c);
			}
			if (d && e) {
			    dist = ABS(dist);
			    if (dist < min) {
				min = dist;
				*close_id = bike[j].id;
			    }
#ifdef HALF_DEBUG
routine = 3;
one = bike[j].id;
two = bike[j].trail_ptr;
three = i;
#endif
			}
		    }
		}
	    }
	}
    }
    else {	/* we're looking in an x direction */
	min = DIM - vec[dir][0]*x;

	/* collide with "good" trail */
	i = good->trail_ptr;
	do {
	    dist = good->trail[i].x - x;
	    a = SIGN(dist);
	    b = good->trail[i].z;
	    i = (i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
	    if (good->trail[i].level == level) {
#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("z good i %d dist %g\n", i, dist);
#endif
		dist2 = good->trail[i].x - x;
		a2 = SIGN(dist2);
		if (f = (a*a2 < 0.0)) dist = 0.0;

#ifdef DEBUG_4
if (id == good->id) printf("in4: tp %d, i %d (i level %d) dist %g a %g (dir %g), b %g\n",
 good->trail_ptr, i, good->trail[i].level, dist, a, vec[dir][0], b);
#endif
		if ( (a == vec[dir][0] || f) && good->trail[i].level == level && i != good->trail_ptr) {
		    c = good->trail[i].z;
		    if (b > c) {
			d = (z - CYCLE_WIDTH <= b);
			e = (z + CYCLE_WIDTH >= c);
		    }
		    else {
			d = (z + CYCLE_WIDTH >= b);
			e = (z - CYCLE_WIDTH <= c);
		    }
#ifdef DEBUG_4
if (id == good->id) printf("in4:   c %g, d %d, e %d\n", c, d, e);
#endif
		    if (d && e) {
			dist = ABS(dist);
			if (dist < min) {
			    min = dist;
			    *close_id = good->id;
			}

#ifdef DEBUG_4
if (id == good->id) printf("in4:      dist %g\n", dist);
#endif

#ifdef HALF_DEBUG
routine = 4;
one = good->id;
two = good->trail_ptr;
three = i;
#endif
		    }
		}
	    }
	} while (i != good->trail_ptr && good->trail[i].level != -1);

	/* collide with "good" bike and leading trail */
	if (id != good->id && good->level == level) {
	    if (good->falling != 0)
		nose = 0.0;
	    else
		nose = CYCLE_NOSE;
	    i = good->trail_ptr;
	    dist = good->trail[i].x - x;
	    a = SIGN(dist);
	    dist2 = good->origin.x + nose*vec[good->vec_ptr][0] - x;
	    a2 = SIGN(dist2);
	    if (f = (a*a2 < 0.0)) dist = 0.0;
	    if (a == vec[dir][0] || f) {
		dist = MIN(a*dist, a2*dist2);
		b = good->trail[i].z;
		c = good->origin.z;
		if (b > c) {
		    d = (z - CYCLE_WIDTH <= b);
		    e = (z + CYCLE_WIDTH >= c);
		}
		else {
		    d = (z + CYCLE_WIDTH >= b);
		    e = (z - CYCLE_WIDTH <= c);
		}
		if (d && e) {
		    dist = ABS(dist);
		    if (dist < min) {
			min = dist;
			*close_id = good->id;
		    }
#ifdef HALF_DEBUG
routine = 5;
one = good->id;
two = good->trail_ptr;
three = i;
#endif
		}
	    }
	}

#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("z end of good: dist %g\n", dist);
#endif

	/* collide with "bad" trail */
	for (j = 0; j < CYCLES; j++) {
	    if (used[j] && j != good->id && bike[j].alive && bike[j].falling >= 0) {
		i = bike[j].trail_ptr;
		do {
		    dist = bike[j].trail[i].x - x;
		    a = SIGN(dist);
		    b = bike[j].trail[i].z;
		    i = (i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
		    if (bike[j].trail[i].level == level) {
#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("z bad  i %d dist %g\n", i, dist);
#endif
			dist2 = bike[j].trail[i].x - x;
			a2 = SIGN(dist2);
			if (f = (a*a2 < 0.0)) dist = 0.0;
			if ( (a == vec[dir][0] || f) && bike[j].trail[i].level == level && i != bike[j].trail_ptr) {
			    c = bike[j].trail[i].z;
			    if (b > c) {
				d = (z - CYCLE_WIDTH <= b);
				e = (z + CYCLE_WIDTH >= c);
			    }
			    else {
				d = (z + CYCLE_WIDTH >= b);
				e = (z - CYCLE_WIDTH <= c);
			    }
			    if (d && e) {
				dist = ABS(dist);
				if (dist < min) {
				    min = dist;
				    *close_id = bike[j].id;
				}
#ifdef HALF_DEBUG
routine = 6;
one = bike[j].id;
two = bike[j].trail_ptr;
three = i;
#endif
			    }
			}
		    }
		} while (i != bike[j].trail_ptr && bike[j].trail[i].level != -1);

#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("z bad pre last seg: dist %g\n", dist);
#endif
		/* collide with "bad" bike and leading trail */
		if (id != bike[j].id && bike[j].level == level) {
		    if (bike[j].falling != 0)
			nose = 0.0;
		    else
			nose = CYCLE_NOSE;
		    i = bike[j].trail_ptr;
		    dist = bike[j].trail[i].x - x;
		    a = SIGN(dist);
		    dist2 = bike[j].origin.x + nose*vec[bike[j].vec_ptr][0] - x;
		    a2 = SIGN(dist2);
		    if (f = (a*a2 < 0.0)) dist = 0.0;
		    if (a == vec[dir][0] || f) {
			dist = MIN(a*dist, a2*dist2);
			b = bike[j].trail[i].z;
			c = bike[j].origin.z + nose*vec[bike[j].vec_ptr][1];
			if (b > c) {
			    d = (z - CYCLE_WIDTH <= b);
			    e = (z + CYCLE_WIDTH >= c);
			}
			else {
			    d = (z + CYCLE_WIDTH >= b);
			    e = (z - CYCLE_WIDTH <= c);
			}
			if (d && e) {
			    dist = ABS(dist);
			    if (dist < min) {
				min = dist;
				*close_id = bike[j].id;
			    }
#ifdef HALF_DEBUG
routine = 7;
one = bike[j].id;
two = bike[j].trail_ptr;
three = i;
#endif
			}
		    }
		}
	    }
	}
    }
#ifdef DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 printf("min %g\n", min);
#endif

#ifdef HALF_DEBUG
#ifndef VERY_DEBUG
if (id == good->id)
#endif
 {
  printf("id %d good (%g, %g) dir (%g, %g) trails %d\n",
   id, good->origin.x, good->origin.z,
   good->direction.x, good->direction.z, good->trail_ptr);
  printf("routine %d\n", routine);
  printf("bike %d headtrail %d trail %d\n", one, two, three);
  printf("min %g\n", min);
}
#endif

    return min;
}
