
#include "../porting/iris2ogl.h"
#include "cycles.h"
#define SIGN(x)   ( (x)<0.0 ? -1.0 : ((x)>0.0?1.0:0.0) )

CYCLE *find_nearest_person(CYCLE *);
int person_elsewhere();

extern float vec[4][2], speed_fac;
extern float speed_fac;
extern int used[CYCLES];
extern CYCLE bike[CYCLES];
extern CPOINT up_hole[LEVELS], down_hole[LEVELS];

/*
 * this routine contains all bad-guy evasive behaviour
 */
void move_cycles(CYCLE *C) {
    int l, r, next_level, target_level, levels_away, behave, close_id;
    CYCLE *target;
    CPOINT *hole;
    float front, left, right, margin, jump_dist;
    float wall_prox, jump_height, jump_speed, zdes=0, xdes=0;

    /* move levels if we're over a hole */
    if ((next_level = fall_down_hole(C)) != -1)
	new_level(C, next_level);

    margin = (CYCLE_NOSE + 2.5*C->step*speed_fac);
    behave = C->behave;

    /* find nearest non falling PERSON */
    if ((target = find_nearest_person(C)) != (CYCLE *)NULL) {
	/* ZDES & XDES are the desired directions to cut off the target */
	xdes = target->origin.x - (C->origin.x-1.5*margin*vec[C->vec_ptr][0]);
	zdes = target->origin.z - (C->origin.z-1.5*margin*vec[C->vec_ptr][1]);
    }
    else if ((target_level = person_elsewhere()) != -1) {
	/* look for another level to go to */
	levels_away = target_level - C->level;
	if ( (levels_away + LEVELS)%LEVELS < LEVELS/2 ) {
	    if (levels_away > 0)
		hole = &up_hole[C->level];
	    else
		hole = &down_hole[C->level];
	}
	else {
	    if (levels_away < 0)
		hole = &up_hole[C->level];
	    else
		hole = &down_hole[C->level];
	}
	xdes = hole->x - C->origin.x;
	zdes = hole->z - C->origin.z;
    }
    else {
	/* no one to head for so meander */
	behave = 0;
    }

    zdes = SIGN(zdes);
    xdes = SIGN(xdes);

#ifdef DEBUG
printf("this is BAD %d\n", C->id);
#endif

    front = block(C->id, C->origin.x, C->origin.z, C->vec_ptr, &close_id);

#ifdef DEBUG
printf("BAD %d front: %g\n", C->id, front);
#endif

    /* start the trail falling if hit something */
    if ((front - CYCLE_NOSE) <= C->step*speed_fac && (C->origin.y < (HEIGHT + TRAIL_HEIGHT))) {
	C->fall += speed_fac;
	C->falling++;
	C->who_we_hit = close_id;
    }

#ifdef DEBUG
if (C->fall) printf("BAD %d FALLING\n", C->id);
#endif

    if (front < margin) {
	l = C->vec_ptr + 1;
	if (l == 4) l = 0;
	r = C->vec_ptr - 1;
	if (r == -1) r = 3;
#ifdef DEBUG
printf("BAD %d sides:\n", C->id);
#endif
	left = block(C->id, C->origin.x, C->origin.z, l, &close_id);
	right = block(C->id, C->origin.x, C->origin.z, r, &close_id);
#ifdef DEBUG
printf("BAD %d finished sides: left %g right %g\n", C->id, left, right);
#endif
	jump_dist = 0.0;
	jump_height = C->origin.y;
	jump_speed = C->jump_speed*speed_fac;
	wall_prox = DIM - ABS(vec[C->vec_ptr][0]*C->origin.x)
	                - ABS(vec[C->vec_ptr][1]*C->origin.z);
	while (jump_height > (HEIGHT + TRAIL_HEIGHT)) {
	    jump_height += jump_speed*speed_fac;
	    jump_speed -= GRAVITY*speed_fac;
	    jump_dist += C->step*speed_fac;
	}
	
	/*
	 * I think this means the bad guys can do turns and jumps
	 * at the same time!!! Oh well - I guess they need all
	 * the help they can get...
	 */
	if ((left > margin) || (right > margin))
	    if (jump_dist < front)
		/* if in attack mode (and blocked) turn towards the good guy */
		if (behave) {
			if (((left>margin)&&((vec[l][0]==xdes)||
				(vec[l][1]==zdes)))||(right<=margin))
				turn(C,1.0);
			if (((right>margin)&&((vec[r][0]==xdes)||
				(vec[r][1]==zdes)))||(left<=margin))
				turn(C,-1.0);
		} else
		    if (left < right)
		        turn(C, -1.0);
		    else
		        turn(C, 1.0);

	if ((jump_dist > front) && (wall_prox < margin))
	    if (left < right)
		turn(C, -1.0);
	    else
		turn(C, 1.0);

	if ((left < margin) && (right < margin) && !C->jump) jump(C);
	if (C->step>MIN_STEP) C->step-=0.3*speed_fac;
    }
    else {
	/* if unblocked, attack if possible */
	if (behave && (zdes!=vec[C->vec_ptr][1])&&(xdes!=vec[C->vec_ptr][0])){
	    l=C->vec_ptr+1;
	    if(l==4)l=0;
	    r=C->vec_ptr-1;
	    if(r== -1) r=3;
	    left=block(C->id,C->origin.x,C->origin.z,l, &close_id);
	    right=block(C->id,C->origin.x,C->origin.z,r, &close_id);
	    if ((left>margin)&&((vec[l][0]==xdes)||(vec[l][1]==zdes))) {
		turn(C,1.0);
	    }
	    if ((right>margin)&&((vec[r][0]==xdes)||(vec[r][1]==zdes))) {
		turn(C,-1.0);
	    }
	}
	    if (!behave) {
        int random_divisor = (int)(200.0*speed_fac);
        if (random_divisor < 1) random_divisor = 1; // Éviter division par zéro
        
        if (!(rand()%random_divisor)) {
        l=C->vec_ptr+1;
        if(l==4)l=0;
        r=C->vec_ptr-1;
        if(r== -1) r=3;
        left=block(C->id,C->origin.x,C->origin.z,l, &close_id);
        right=block(C->id,C->origin.x,C->origin.z,r, &close_id);
        if ((left > margin) || (right > margin))
            if (left < right)
            turn(C, -1.0);
            else
            turn(C, 1.0);
        }
    }
	if ((front>0.66*DIM) && (C->step<MAX_STEP)) C->step += 0.1*speed_fac;
	if ((front<0.25*DIM) && (C->step>MIN_STEP)) C->step -= 0.2*speed_fac;
    }

    /* check we're not going too fast or slow */
    if (C->step>MAX_STEP) C->step = MAX_STEP;
    if (C->step<MIN_STEP) C->step = MIN_STEP;

    /*
     * move last 'cos speed fac can vary greatly
     * hence the stuff above can be wrong if we move first
     */
    mov(C);

    /* check if outside grid and fall if are */
    check_outside(C);
}


/* find nearest non falling PERSON to cycle C */
CYCLE *find_nearest_person(CYCLE *C) {
    int i;
    float dx, dz, dist2, min_d2;
    CYCLE *nearest;

    min_d2 = 1000.0*DIM;
    nearest = (CYCLE *)NULL;

    for (i = 0; i < CYCLES; i++) {
	if (used[i] && bike[i].alive && bike[i].type == PERSON
	 && bike[i].falling == 0 && bike[i].level == C->level) {
	    dx = bike[i].origin.x - C->origin.x;
	    dz = bike[i].origin.z - C->origin.z;
	    dist2 = dx*dx + dz*dz;
	    if (dist2 < min_d2) {
		nearest = &bike[i];
		min_d2 = dist2;
	    }
	}
    }

    return (nearest);
}


/* returns a level number where there is a person */
int person_elsewhere() {
    int i;

    for (i = 0; i < CYCLES; i++)
	if (used[i] && bike[i].alive && bike[i].type == PERSON && bike[i].falling == 0)
	    return(bike[i].level);

    return (-1);
}
