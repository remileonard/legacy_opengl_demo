/*
 * cycles
 *
 *    by Robin Humble <rjh@pixel.maths.monash.edu.au>
 *       Alan Lipton  <alan.j.lipton@eng.monash.edu.au>)
 *       Nick Fitton  <jalippo@pixel.maths.monash.edu.au>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../porting/iris2ogl.h"
#include "cycles.h"
#ifdef AUDIO
#include "sound.h"
short kaboom, boom[CYCLES], still_close, tumbling;
#endif

Matrix idmat = {1.0, 0.0, 0.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0};

Pattern16 quartertone = {0x8888, 0x2222, 0x4444, 0x1111, 0x8888, 0x2222,
                         0x4444, 0x1111, 0x8888, 0x2222, 0x4444, 0x1111,
                         0x8888, 0x2222, 0x4444, 0x1111};

/* This is a direction vector in (x,z) coordinates */
float vec[4][2] = { { 0.0, -1.0},
                    {-1.0,  0.0},
                    { 0.0,  1.0},
                    { 1.0,  0.0} };


float speed_fac;    /* for real-timing */
CYCLE *good, bike[CYCLES];
int used[CYCLES];   /* true if this slot filled in user list */
int robot[CYCLES];  /* list of robots running on this machine */
int solo, in_win, wheel_angle[CYCLES][2], neo_mort[CYCLES];
int audio, demo_mode;
clock_t last_sent;
CPOINT up_hole[LEVELS], down_hole[LEVELS];
extern CYCLE start_pos[CYCLES];


/*
 * main routine - this routine initializes the game, polls the input
 * devices, calls the moving routines and draws the scenery.
 */
int cycles_main_impl(int argc, char **argv) {
    int i, start_again, new_arrived, close_id, pts, total_pts, games;
    int help_mode, look_offset, num_robots, next_level;
    int big_kills, trail_kills, tot_big_kills, tot_trail_kills;
    int colour_choice;
    short val;
    long event;
    struct tms t;
    float min;
    char name[NAME_SIZE];
#ifdef DEBUG
    int remote;
#endif
#ifdef SHOW_TIMING
    double frame_time;
    int frame_count, frame_ave = 5;
    clock_t prev_ticks, now_ticks;
#endif

    /* parse networking args */
    parse_args(argc, argv);

    solo = 1;
    demo_mode = 0;
#ifdef AUDIO
    audio = 1;
#else
    audio = 0;
#endif
    num_robots = 2;
    colour_choice = -1;
    if (num_robots > CYCLES-1) num_robots = CYCLES-1;

    total_pts = 0;
    tot_big_kills = 0;
    tot_trail_kills = 0;
    games = 0;
    help_mode = 0;
    speed_fac = 1.0;
    for (i = 0; i < CYCLES; i++) {
	wheel_angle[i][0] = 0;
	wheel_angle[i][1] = 0;
    }
    bcopy(cuserid((char *)NULL), name, NAME_SIZE);

    init_fonts();

#ifdef DEBUG
foreground();	/* @@@ */
/* prefposition(300, 1100, 400, 800); */
#endif

    openwindow();

    srand((unsigned int)times(&t)); /* seed random numbers */
    make_objs();
    defpattern(1, 16, quartertone);
    in_win = 1;	/* assume have input focus */

    title_screen(0, name, &colour_choice, &num_robots);
    scale_fonts_to_win();
    title_screen(1, name, &colour_choice, &num_robots);
    title_screen(2, name, &colour_choice, &num_robots);
    title_screen(3, name, &colour_choice, &num_robots);
    title_screen(4, name, &colour_choice, &num_robots);
    if (!solo) title_screen(5, name, &colour_choice, &num_robots);

#ifdef DEBUG
/* @@@ */
if (solo)
  printf("solo, cycles: %d\n", num_robots);
else
  printf("network!!! cycles: %d\n", num_robots);
if (audio) printf("AUDIO!!!\n");
#endif

    if (!solo) init_comms();
    init_holes();
#ifdef AUDIO
    if (audio) {
	init_audio();
	init_sound_flags();
    }
#endif

    if (solo) {
	/* init us -> id = 0 */
	good = &bike[0];
	init_pos(good);
	good->id = 0;
	used[0] = 1;

	/* and our robots */
	for (i = 1; i <= num_robots; i++) {
	    robot[i] = 1;
	    do {    /* loop 'til get different colour */
		init_pos(&bike[i]);
	    } while (bike[i].trail_colour == good->trail_colour);
	    bike[i].id = i;
	    used[i] = 1;
	    bike[i].owner = good->id;
	    init_tumble(&bike[i]);
	}
    }
    else {
	init_network(&num_robots);  /* <- includes setting up our */
                                    /*    position and our robots */

	/* send a dead, but entering packet */
	good->alive = 0;
	for (i = 0; i < CYCLES; i++)
	    if (robot[i]) bike[i].alive = 0;

	/* wait for network replies to our entry to the grid */
	send_update_mcast();
	last_sent = times(&t);
	new_arrived = wait_for_replies();

	/* make us alive again */
	good->alive = 1;
	good->owner = good->id;
	for (i = 0; i < CYCLES; i++)
	    if (robot[i]) {
		bike[i].alive = 1;
		bike[i].owner = good->id;
		init_tumble(&bike[i]);
		send_full_mcast(&bike[i]);
	    }
    }

#ifdef DEBUG
/* @@@ */
if (demo_mode) printf("demo mode! %d\n", robot[good->id]);

/* @@@ tmp hack for dgl -- how do we do this properly??? */
remote = 0;
if (strcmp(getenv("DISPLAY"), ":0")) remote = 1;
if (remote) printf("displaying REMOTE!\n");
#endif

    while (1) {

	start_again = 1;

	/* setup tumble initial state */
	do {    /* loop until we're not gonna immediately hit */
	    init_pos(good);
	} while (fall_down_hole(good) == -1 && block(good->id, good->origin.x, good->origin.z, good->vec_ptr, &close_id) - CYCLE_NOSE < 3.0*good->step);
	if (demo_mode) {
	    good->type = ROBOT;
	    robot[good->id] = 1;
	}
	else {
	    good->type = PERSON;
	}
	sprintf(good->name, "%s", name);
	good->pts = total_pts;
	good->games = games;
	if (colour_choice != -1) good->trail_colour = colour_choice;
	init_tumble(good);

	/* send all our init info */
	if (!solo) send_full_mcast(good);

#ifdef DEBUG
printf("TOP CYC cycles: robot: \n");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", robot[i]); }
printf("\n");
printf("cycles: used: \n");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
printf("cycles: PLAYERS: \n");
{ int i; for (i=0;i<CYCLES;i++) if (used[i]) printf("%d: %s ", bike[i].id, bike[i].name); }
printf("\n");
#endif

	look_offset = 0;
	pts = 0;
	big_kills= 0;
	trail_kills = 0;
	for (i = 0; i < CYCLES; i++) neo_mort[i] = 0;
	min = 2.0*DIM;

	search_for_exit();  /* leave now if requested */

	do {   /* loop here until die */

#ifdef DEBUG
printf("\n");
printf("top cycles: robot: \n");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", robot[i]); }
printf("\n");
printf("top cycles: used: \n");
{ int i; for (i=0;i<CYCLES;i++) printf("%d ", used[i]); }
printf("\n");
printf("top cycles: players: \n");
{ int i; for (i=0;i<CYCLES;i++) if (used[i]) printf("%d: %s aliv? %d fall %g   ",
 bike[i].id, bike[i].name, bike[i].alive, bike[i].fall); }
printf("\n");
#endif

	    /* set the speed_fac for this frame, guess if first */
	    set_speed_fac(start_again);

#ifdef SHOW_TIMING
	    if (start_again) {
	        frame_time = 0.0;
		frame_count = 0;
		prev_ticks = times(&t);
	    }
	    else
		frame_count++; frame_count %= frame_ave;
#endif
	    start_again = 0;

	    /* handle buffered turn events */
	    if (!demo_mode) {
		if (good->lturn > 0) good->lturn--;
		if (good->rturn > 0) good->rturn--;

		/* if turn > 0 then we have a buffered turn */
		if (good->lturn && !good->rturn) turn(good, 1.0);
		if (good->rturn && !good->lturn) turn(good, -1.0);
	    }

	    /* take user input sequence */
	    while (qtest()) {
		switch(event = qread(&val)) {
		case SPACEKEY:
		    if (val && !good->jump) {
			jump(good);
#ifdef AUDIO
			if (audio) play_sound("jump.aiff");
#endif
		    }
		    break;
		case LEFTMOUSE:
		    if (val && good->falling == 0 && !good->jump && !demo_mode) {
			if (!good->lturn && !good->rturn) turn(good, 1.0);
			good->lturn++;
		    }
		    break;
		case RIGHTMOUSE:
		    if (val && good->falling == 0 && !good->jump && !demo_mode) {
			if (!good->lturn && !good->rturn) turn(good, -1.0);
			good->rturn++;
		    }
		    break;
		case LEFTARROWKEY:
		    if (val) {
			look_offset++;
			if (look_offset == 4) look_offset = 0;
		    }
		    break;
		case RIGHTARROWKEY:
		    if (val) {
			look_offset--;
			if (look_offset == -1) look_offset = 3;
		    }
		    break;
		case ESCKEY:	/* NOTE: this in search_for_exit and draw_score_screen also */
		    good->quit = 1;
		    for (i = 0; i < CYCLES; i++)
			if (robot[i]) bike[i].quit = 1;
		    if (!solo) send_update_mcast();
#ifdef AUDIO
		    if (audio) close_audio();
#endif
		    exit(0);
		    break;
		case HKEY:
		    if (val) {
			help_mode++;
			help_mode %= 2;
		    }
		    break;
		case REDRAW:
		    reshapeviewport();
		    break;
		case INPUTCHANGE:
		    in_win = val % 256;	    /* % for dgl stuff */
		    break;
		default:
		    printf("unhandled default event: %ld\n", event);
		    break;
		}
	    }   /* end of input queue */

	    /* speed up and slow down */
	    if (!demo_mode && good->falling == 0 && !good->jump) { /* fall things are pre-programmed and can't accel when jumped! */
		if (in_win && (getbutton(MIDDLEMOUSE) || getbutton(AKEY)))
		    good->step += 0.1*speed_fac;
		else
		    good->step -= 0.2*speed_fac;

		/* set min and max speeds */
		if (good->step < MIN_STEP)
		    good->step = MIN_STEP;
		else if (good->step > MAX_STEP)
		    good->step = MAX_STEP;
	    }
#ifdef AUDIO
	    if (audio) change_cycle_pitch(min);
#endif

#ifdef DEBUG
printf("\n\npre-die: alive %d, step %g, oy %g, ht %g\n", 
 good->alive, good->step, good->origin.y, (HEIGHT + TRAIL_HEIGHT));

printf("after input cycles: players: good->fall %g \n", good->fall);
{ int i; for (i=0;i<CYCLES;i++) if (used[i]) printf("%d: %s aliv? %d  ", bike[i].id, bike[i].name, bike[i].alive); }
printf("\n");
#endif
	    /*
	     * network - read others positions
	     * still read the network, even when doing fall things...
	     */
	    if (!solo) new_arrived = get_and_sort_mcasts();

#ifdef DEBUG
printf("after get mcast cycles: players: good->fall %g \n", good->fall);
{ int i; for (i=0;i<CYCLES;i++) if (used[i]) printf("%d: %s aliv? %d  ", bike[i].id, bike[i].name, bike[i].alive); }
printf("\n");
#endif

	    if (!demo_mode && good->falling == 0) {
		/* start a die if we'll hit anything */
		if (((min = (block(good->id, good->origin.x, good->origin.z, good->vec_ptr, &close_id) - CYCLE_NOSE)) <= good->step*speed_fac)
		  && (good->origin.y <= (HEIGHT + TRAIL_HEIGHT))) {
		    good->fall += speed_fac;
		    good->falling++;
		    good->who_we_hit = close_id;
		}
		/* also check if we made it ouside by mistake and fall if we did */
		check_outside(good);

		/* see if we've fallen down a hole */
		if ((next_level = fall_down_hole(good)) != -1)
		    new_level(good, next_level);

		/* move us... */
		mov(good);
	    }


	    /* move the bikes and let them avoid */
	    for (i = 0; i < CYCLES; i++)
		if (robot[i] && bike[i].falling == 0)
		    move_cycles(&bike[i]);

	    /* sort out our motion */
	    if (!demo_mode) {
		if (good->falling == 0) {
#ifdef AUDIO
		    tumbling = 0;
#endif
		    calcorg(good); /* handle jumping */
		}
		else if (good->falling > 0) { /* we've hit and are falling */
		    explode_me(good);
		    good->fall += speed_fac;
		    good->falling++;
#ifdef AUDIO
		    if (audio && !kaboom) play_sound("boom1.aiff");
		    kaboom = 1;
#endif
		}
		else { /* tumbling down onto grid */
		    tumble_down(good);
#ifdef AUDIO
		    tumbling = 1;
		    kaboom = 0;
#endif
		}
	    }
	    else {  /* demo mode explode */
		if (good->falling > 0) { /* we've hit and are falling */
		    explode_me(good);	/* incrementing fall handled in move_our_robots */
#ifdef AUDIO
		    if (audio && !kaboom) play_sound("boom1.aiff");
		    kaboom = 1;
#endif
		}
	    }

	    good->view = vadd(good->origin, good->direction);

	    /* ooops, we died */
	    if (!demo_mode && good->fall >= WALL_FALL) good->alive = 0;

	    /* see if we've scored any points */
	    if (good->falling == 0)
		do_scoring(good, &pts, &big_kills, &trail_kills);

	    /* network mode - send info about ourselves and our robots */
	    if (!solo) {
		/* if we've not sent for > NET_TIMEOUT seconds then request full */
		if (new_arrived || last_sent + NET_TIMEOUT*HZ < times(&t))
		    send_all_full_mcast();
		else
		    send_update_mcast();

		last_sent = times(&t);
	    }

#ifdef DEBUG
printf("after sends cycles: players: \n");
{ int i; for (i=0;i<CYCLES;i++) if (used[i]) printf("%d: %s aliv? %d fall %g   ",
 bike[i].id, bike[i].name, bike[i].alive, bike[i].fall); }
printf("\n");
#endif

	    /* set view */
	    loadmatrix(idmat);
	    if (good->falling == 0)
		lookat(good->origin.x, good->origin.y - 0.5, good->origin.z,
	           good->origin.x + vec[(good->vec_ptr + look_offset)%4][0], good->origin.y - 0.5, good->origin.z + vec[(good->vec_ptr + look_offset)%4][1], 0.0);
	    else
		lookat(good->origin.x, good->origin.y - 0.5, good->origin.z,
	                 good->view.x, good->origin.y - 0.5, good->view.z, 0.0);

	    /* lots of drawing... */
#ifdef RGB_MODE
	    cpack(0x0);
#else
	    color(BLACK);
#endif
	    clear();
	    zclear();

	    drawgrid(good->level, look_offset);

	    if (good->falling >= 0) drawtrail(good, good->level);

	    /* jump, restart, draw etc our robots */
	    move_our_robots();

#ifdef DEBUG
printf("afetr moves cycles: players: good->fall %g \n", good->fall);
{ int i; for (i=0;i<CYCLES;i++) if (used[i]) printf("%d: %s aliv? %d fall %g   ",
 bike[i].id, bike[i].name, bike[i].alive, bike[i].fall); }
printf("\n");
#endif
	    /* draw all */
	    for (i = 0; i < CYCLES; i++)
		if (used[i] && bike[i].alive && i != good->id && bike[i].falling >= 0)
		    drawtrail(&bike[i], good->level);
	    for (i = 0; i < CYCLES; i++)
		if (used[i] && bike[i].alive && i != good->id) {
		    if (bike[i].falling <= 0)
			drawcycle(&bike[i], good->level, look_offset);
		    else {
			explode(&bike[i], good->level);
#ifdef AUDIO
			if(audio && !boom[i]) {
			    if(bike[i].level == good->level)
				play_sound("boom2.aiff");
			    else
				play_sound("boom3.aiff");
			}
			boom[i] = 1;
#endif
		    }
		}

	    /* kill off any cycles that aren't talking any more */
	    if (!solo) kill_dead_cycle();

#ifdef DEBUG
printf("after draws cycles: players: good->fall %g \n", good->fall);
{ int i; for (i=0;i<CYCLES;i++) if (used[i]) printf("%d: %s aliv? %d  ", bike[i].id, bike[i].name, bike[i].alive); }
printf("\n");
#endif
	    /* fade to grey if dying */
	    if (good->falling > 0) {
#ifdef RGB_MODE
		cpack(0xaaaaff);
#else
		color(WHITE);
#endif
		setpattern(1);
		clear();
		setpattern(0);
	    }

	    loadmatrix(idmat);
	    draw_info(look_offset, help_mode, min, pts, big_kills, trail_kills);

#ifdef DEBUG
printf("after info cycles: players: good->fall %g \n", good->fall);
{ int i; for (i=0;i<CYCLES;i++) if (used[i]) printf("%d: %s aliv? %d  ", bike[i].id, bike[i].name, bike[i].alive); }
printf("\n");
#endif

#ifdef SHOW_TIMING
	    if (!frame_count) {
		now_ticks = times(&t);
		frame_time = (double)(now_ticks - prev_ticks)/(double)(HZ*frame_ave);
		prev_ticks = now_ticks;
	    }
	    loadmatrix(idmat);
	    show_time(frame_time, min);
#endif

	    swapbuffers();

#ifdef DEBUG
/* @@@ tmp hack for dgl */
if (remote && good->fall) finish();
#endif

	} while (good->alive);

	total_pts += pts;
	tot_big_kills += big_kills;
	tot_trail_kills += trail_kills;
	good->games = ++games;
	good->pts = total_pts;

	if (!demo_mode) {
	    good->alive = 0;
	    score_screen(pts, tot_big_kills, tot_trail_kills);
	}
#ifdef DEBUG
printf("\n\n");
#endif
	search_for_exit();
    }
}
