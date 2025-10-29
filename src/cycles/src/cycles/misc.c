
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../porting/iris2ogl.h"
#include "cycles.h"
#include "sound.h"

extern CYCLE *good, bike[CYCLES];
extern float vec[4][2];
extern float speed_fac;
extern int used[CYCLES], robot[CYCLES], solo, in_win;
extern int audio, demo_mode, neo_mort[CYCLES];
extern CPOINT up_hole[LEVELS], down_hole[LEVELS];

void new_hole(CPOINT *);
float dist(CYCLE *, CYCLE *);

/*
 * vadd: returns the sum of two vectors
 * note: pass two CPOINTS,  not pointers to CPOINTS
 */
CPOINT vadd (CPOINT a, CPOINT b) {
    CPOINT temp;

    temp.x = a.x + b.x;
    temp.y = a.y + b.y;
    temp.z = a.z + b.z;

    return temp;
}


/*
 * loop through the event queue looking for exits
 * and discarding the rest
 * NOTE: this is in main loop and draw_score_screen also
 */
void search_for_exit(void) {
    short val;
    int i;

    while (qtest()) {
	switch(qread(&val)) {
	case REDRAW:
	    reshapeviewport();
	    break;
	case INPUTCHANGE:
	    in_win = val % 256;	    /* % for dgl stuff */
	    break;
	case ESCKEY:
	    good->quit = 1;
	    for (i = 0; i < CYCLES; i++)
		if (robot[i]) bike[i].quit = 1;
	    if (!solo) send_update_mcast();
#ifdef AUDIO
	    if (audio) close_audio();
#endif
	    exit(0);
	    break;
	default:
	    break;
	}
    }
}


/* Handles jumps for cycle C */
void calcorg(CYCLE *C) {
    if (C->jump) {
	C->origin.y += C->jump_speed*speed_fac;
	C->jump_speed -= GRAVITY*speed_fac;
	if (C->origin.y <= HEIGHT) {
	    C->origin.y = HEIGHT;
	    if ((C->jump_speed = -0.2*C->jump_speed) <= 1.0)
		C->jump = 0;	/* stop jumping */
	}
    }
}


/*if a turn has occured for a cycle (C) record it for trail drawing puposes */
void newtrail(CYCLE *C) {
    C->trail_ptr++;
    C->trail_ptr %= TRAIL_LENGTH;
    C->trail[C->trail_ptr].level = C->level; /* tag the trail as being used */
    C->trail[C->trail_ptr].x = C->origin.x; /* set values */
    C->trail[C->trail_ptr].y = 0.0;
    C->trail[C->trail_ptr].z = C->origin.z;

    /*
     * copy this turn (max one per frame) into the update
     * struct for later broadcast to the other players
     */
    bcopy((void *)&C->trail[C->trail_ptr], (void *)&C->trail_update, (int)sizeof(CPOINT));
}


/* Handle a cycle turn */
void turn(CYCLE *C, float dir) { 
    newtrail(C);
    C->vec_ptr += (int)dir;
    if (C->vec_ptr == 4) C->vec_ptr = 0;
    if (C->vec_ptr == -1) C->vec_ptr = 3;
    C->direction.x = vec[C->vec_ptr][0];
    C->direction.z = vec[C->vec_ptr][1]; 
}


/* Initiate a cycle jump */
void jump(CYCLE *C) {
    if (!C->jump) {
	C->jump = 1;
	C->jump_speed = JUMP_POWER;
    }
}


/* check if a bike is outside the grid and start it falling if it is */
void check_outside(CYCLE *C) {
    if (C->origin.x <= -DIM || C->origin.x >= DIM
     || C->origin.z <= -DIM || C->origin.z >= DIM) {
	C->fall += speed_fac;
	C->falling++;
	C->who_we_hit = -1;
    }
}


/* physically move a cycle (laterally, not vertically) */
void mov(CYCLE *C) {
    float scaled_step;

    scaled_step = C->step*speed_fac;
    C->origin.x += scaled_step*C->direction.x;
    C->origin.z += scaled_step*C->direction.z;
}


/* set the bike onto a new level */
void new_level(CYCLE *C, int next_level) {
    newtrail(C);
    if (!solo) send_update_mcast();
    C->level = next_level;
    newtrail(C);
    C->jump = 1;
    C->origin.y = HEIGHT + 0.05*DIM;
    C->jump_speed = 0.0;
    if (!solo) send_update_mcast();
#ifdef AUDIO
    if (audio && C == good) play_sound("fall.aiff");
#endif
}


/* set up one new hole */
void new_hole(CPOINT *P) {
    int size = (int)DIM;

    /* round off to nearest 10.0 */
    P->x = 10.0*(int)((1.6*(float)(rand()%size) - 0.8*DIM)/10.0);
    P->z = 10.0*(int)((1.6*(float)(rand()%size) - 0.8*DIM)/10.0);    
}


/* setup the init position of our holes */
void init_holes(void) {
    int i;

    for (i = 0; i < LEVELS; i++) {
	new_hole(  &up_hole[i]);
	new_hole(&down_hole[i]);
    }
}


/*
 * check if we've fallen through a hole into the next level
 * NOTE: these holes must match up with those in draw_coloured_grid
 */
int fall_down_hole(CYCLE *C) {
    int i, go_to;

    if (!C->jump && C->falling == 0) {  /* can't change levels whilst jumping or dying */
	i = C->level;

	/* up teleport holes */
	go_to = (i+LEVELS-1) % LEVELS;
	if (go_to == -1) go_to = LEVELS-1;
	if ( (C->origin.x > up_hole[i].x - HOLE_SIZE)
	  && (C->origin.x < up_hole[i].x + HOLE_SIZE)
	  && (C->origin.z > up_hole[i].z - HOLE_SIZE)
	  && (C->origin.z < up_hole[i].z + HOLE_SIZE) ) {
	    new_hole(&up_hole[i]);
	    return(go_to);
	}

	/* down teleport holes */
	go_to = (i+1) % LEVELS;
	if ( (C->origin.x > down_hole[i].x - HOLE_SIZE)
	  && (C->origin.x < down_hole[i].x + HOLE_SIZE)
	  && (C->origin.z > down_hole[i].z - HOLE_SIZE)
	  && (C->origin.z < down_hole[i].z + HOLE_SIZE) ) {
	    new_hole(&down_hole[i]);
	    return(go_to);
	}
    }
    return(-1);
}


void move_our_robots(void) {
    int i;
    for (i = 0; i < CYCLES; i++) {
	if (robot[i]) {
	    if (bike[i].alive) {
		if (bike[i].falling == 0)
		    calcorg(&bike[i]);
		else if (bike[i].falling > 0) {
		    bike[i].fall += speed_fac;
		    bike[i].falling++;
		}
		else /* tumbling down onto grid */
		    tumble_down(&bike[i]);
	    }

	    if (bike[i].fall >= WALL_FALL) {
		bike[i].alive = 0;
		/* restart robot */
		if (!(demo_mode && i == good->id)) {
		    if (solo)
			do {    /* loop 'til get different colour */
			    init_pos(&bike[i]);
			} while (bike[i].trail_colour == good->trail_colour);
		    else	/* don't bother if network */
			init_pos(&bike[i]);

		    init_tumble(&bike[i]);
		    if (!solo) send_full_mcast(&bike[i]);
		}
	    }
	}
    }
}


/* distance between 2 bikes */
float dist(CYCLE *C, CYCLE *D) {
    float dx, dz;

    dx = D->origin.x - C->origin.x;
    dz = D->origin.z - C->origin.z;

    return( (float)sqrt((double)(dx*dx + dz*dz)) );
}


/* do scoring here */
void do_scoring(CYCLE *C, int *pts, int *big_kills, int *trail_kills) {
    float distance;
    int i;

    /* give us points for still being around and going fast */
    *pts += (int)( (5.0 + 5.0*(C->step - MIN_STEP)/(MAX_STEP - MIN_STEP))*speed_fac );

    /*
     * look at the other bikes and if they're newly starting
     * to fall then maybe add stuff to our score
     */
    for (i = 0; i < CYCLES; i++) {
	if (used[i]) {
	    if (!bike[i].alive)
		neo_mort[i] = 0;    /* old dead */
	    else {
		if (bike[i].falling > 0 && !neo_mort[i]) { /* they just started dying ... */

		    /* see if we get points for being close */
		    distance = dist(C, &bike[i]);
		    if (bike[i].level == good->level && distance < 0.3*DIM) {
			(*pts) += 10000;
			(*big_kills)++;
		    }

		    /* see if they hit our trail */
		    if (bike[i].who_we_hit == C->id) {
			(*pts) += 3000;
			(*trail_kills)++;
		    }

		    neo_mort[i] = 1;
		}
	    }
	}
    }
}
