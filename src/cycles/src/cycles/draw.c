/*
 * all the gl drawing routines
 */

#include "../porting/iris2ogl.h"
#include "cycles.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX
    #define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

extern Object tail_obj, engine_obj, chassis_top, back_stuff[LEVELS];
extern float speed_fac, vec[4][2];
extern CYCLE *good, bike[CYCLES];
extern int used[CYCLES], solo;
extern Matrix idmat;

#ifdef RGB_MODE
int trail_col[COLOURS] = {0xffff00, 0xff00ff, 0x00ffff, 0x55ff00, 0xff5500, 0x5500ff};
int level_col[LEVELS] = {0xff4444, 0x44cc44, 0x4444ff};
#else
int trail_col[COLOURS] = {102, 103, 104, 105, 106, 107};
int level_col[LEVELS] = {BLUE, GREEN, RED};
#endif

CYCLE start_pos[CYCLES];
static float start_angle[CYCLES], spin[CYCLES];

/* robin's soundtrack of choice is Velocity Girl */

void drawgrid(int level, int look_offset) {
    /* do crude clipping */
    if (((good->vec_ptr + look_offset) % 4) || good->falling != 0)
        callobj(back_stuff[level]);
    draw_coloured_grid(level);
}

/* draws the trail of Cycle C,  will also handle a falling wall of a dead cycle */
void drawtrail(CYCLE *C, int draw_level) {
    register int i;
    int segment;
    float v[3];
    float height;

#ifdef DEBUG
    printf("trail:\n");
#endif

    height = TRAIL_HEIGHT - TRAIL_HEIGHT * (C->fall) / WALL_FALL;

    /*
     * draw all but the 1st trail element
     */
    segment = 0;
    i = C->trail_ptr;
#ifdef RGB_MODE
    cpack(trail_col[C->trail_colour]);
#else
    color(trail_col[C->trail_colour]);
#endif
    if (C->fall < WALL_FALL) {
        do {
            if (C->trail[i].level == draw_level) {
                if (!segment) {
                    segment = 1;
                    bgntmesh();
                }
                v[0] = C->trail[i].x;
                v[1] = 0.0;
                v[2] = C->trail[i].z;
                v3f(v);
                v[1] = height;
                v3f(v);
            } else if (segment) {
                segment = 0;
                endtmesh();
            }
            i = (i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
#ifdef DEBUG
            printf(" %d", i);
#endif
        } while (i != C->trail_ptr && C->trail[i].level != -1);
        if (segment)
            endtmesh();

#ifdef DEBUG
        printf("\n");
#endif

        /*
         * draw the 1st trail element from the bike to the last corner
         */
        i = C->trail_ptr;
        v[1] = 0.0;
        if (C->trail[i].level == draw_level &&
            MAX(ABS(C->trail[i].x - C->origin.x), ABS(C->trail[i].z - C->origin.z)) > CYCLE_TAIL) {
            bgnpolygon();
            v[0] = C->trail[i].x;
            v[2] = C->trail[i].z;
            v3f(v);
            v[1] = height;
            v3f(v);
#if defined(SHADING) && defined(RGB_MODE)
            cpack(0xffffff);
#endif
            v[0] = C->origin.x - CYCLE_TAIL * vec[C->vec_ptr][0];
            v[2] = C->origin.z - CYCLE_TAIL * vec[C->vec_ptr][1];
            v3f(v);
            v[1] = 0.0;
            v3f(v);
            endpolygon();
        }
    }

    /*
     * do it all again, but for lines on top and bottom
     */
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif

    if (C->fall < WALL_FALL) {

        /*
         * don't zbuffer the top line 'cos it flickers...
         * unless it's below eye level in which case we have to
         */
        if (C->falling == 0)
            zbuffer(FALSE);

        /*
         * all but 1st trail element
         */
        segment = 0;
        i = C->trail_ptr;
        v[1] = height;
        do {
            if (C->trail[i].level == draw_level) {
                if (!segment) {
                    segment = 1;
                    bgnline();
                }
                v[0] = C->trail[i].x;
                v[2] = C->trail[i].z;
                v3f(v);
            } else if (segment) {
                segment = 0;
                endline();
            }
            i = (i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
        } while (i != C->trail_ptr && C->trail[i].level != -1);
        if (segment)
            endline();

        /*
         * 1st trail: from bike to 1st corner
         */
        i = C->trail_ptr;
        v[1] = height;
        if (C->trail[i].level == draw_level &&
            MAX(ABS(C->trail[i].x - C->origin.x), ABS(C->trail[i].z - C->origin.z)) > CYCLE_TAIL) {
            bgnline();
            v[0] = C->trail[i].x;
            v[2] = C->trail[i].z;
            v3f(v);
            v[0] = C->origin.x - CYCLE_TAIL * vec[C->vec_ptr][0];
            v[2] = C->origin.z - CYCLE_TAIL * vec[C->vec_ptr][1];
            v3f(v);
            endline();
        }

        if (C->falling == 0)
            zbuffer(TRUE);

        /* upright line segments */
        if (C->trail[i].level == draw_level) {
            linewidth(2);

            i = C->trail_ptr;
            do {
                if (C->trail[i].level == draw_level) {
                    v[0] = C->trail[i].x;
                    v[2] = C->trail[i].z;
                    bgnline();
                    v[1] = height;
                    v3f(v);
                    v[1] = 0.0;
                    v3f(v);
                    endline();
                }
                i = (i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
            } while (i != C->trail_ptr && C->trail[i].level != -1);

            linewidth(1);
        }
    }
}

/* Draws cycle C in its C->origin point */
void drawcycle(CYCLE *C, int level, int look_offset) {
    float dir;
    int dirx, dirz, lo_res;

    if (C->level != level)
        return;

    /* do crude clipping - behind us so don't draw */
    if (C != good && good->falling == 0) {
        /* one of these will be zero, return if both at 0 */
        dirx = (vec[(good->vec_ptr + look_offset) % 4][0] * (C->origin.x - good->origin.x) > 0);
        dirz = (vec[(good->vec_ptr + look_offset) % 4][1] * (C->origin.z - good->origin.z) > 0);
        if (!(dirx || dirz))
            return;
    }

    pushmatrix();
    /* have a simple algorithm if just running around on the grid */
    if (C->falling == 0)
        dir = 90.0 * (float)C->vec_ptr;
    else
        dir = -atan2(C->direction.z, C->direction.x) * 180.0 / M_PI - 90.0;

    /* set lo res or high res mode for wheel drawing */
    lo_res = 0;
    if (speed_fac > 1.5)
        lo_res = 1;

    translate(C->origin.x, C->origin.y - HEIGHT, C->origin.z);
    rot(dir, 'y');
    translate(0.0, 0.0, -0.5 + CYCLE_VIEW_PT);
    draw_cycle(C->id, C->step, C->trail_colour, lo_res, C->type, bike[C->owner].trail_colour);
    popmatrix();
}

/*
 * If C->fall > 0 this routine explodes a badguy cycle
 */
void explode(CYCLE *C, int level) {
    float dir;
    float dying;

    if (C->level != level)
        return;

    dying = C->fall * 100.0 / WALL_FALL;

    if (C->fall < WALL_FALL) {
        if (solo) {
            if (dying != 0.0)
                C->jump_speed = 2 * JUMP_POWER;
            C->origin.y += C->jump_speed;
            C->jump_speed -= GRAVITY;
            if (C->origin.y < HEIGHT)
                C->fall = WALL_FALL;
        } else {
            C->origin.y *= 3.0;
        }

        pushmatrix();
        dir = 90.0 * (float)C->vec_ptr;
        translate(C->origin.x, C->origin.y - HEIGHT, C->origin.z);
        rot(dir, 'y');
        translate(0.0, 0.0, -0.5);

        callobj(chassis_top);

        pushmatrix();
        translate(dying, 0.3 * C->origin.y, -dying);
        callobj(tail_obj);
        popmatrix();

        pushmatrix();
        translate(0.5 * dying, 0.3 * C->origin.y, dying);
        callobj(engine_obj);
        popmatrix();

        pushmatrix();
        translate(dying, C->origin.y, -dying);
        draw_wheel(0);
        popmatrix();

        pushmatrix();
        translate(dying, 1.5, -4.0 + dying);
        draw_wheel(0);
        popmatrix();

        popmatrix();
    }
}

void explode_me(CYCLE *C) {
    float dying, scaled_step;
    double cx, sx;
    static double angle, dir;

    dying = C->fall / WALL_FALL;

    if (C->fall < WALL_FALL) {
        if (C->falling == 1) {
            angle = -(float)(C->vec_ptr + 1) * 0.5 * M_PI;
            dir = ((double)rand() / (double)RAND_MAX - 0.5);
        }
        scaled_step = C->step * speed_fac;

        /* change height */
        C->origin.y += 0.5 * scaled_step * JUMP_POWER * (0.6 - dying);

        /* change direction */
        angle += dir * 3.0 * M_PI * speed_fac / (double)WALL_FALL;
        cx = cos(angle);
        sx = sin(angle);
        C->direction.x = cx;
        C->direction.y = -0.7 * dying;
        C->direction.z = sx;
    }
}

/*
 * save the initial position of the cycle so that we can
 * tumble down to it and restore it properly and then move C
 * to the initial position for falling
 */
void init_tumble(CYCLE *C) {
    double angle;

    bcopy((void *)C, (void *)&start_pos[C->id], sizeof(CYCLE));

    /* setup to match the stuff in tumble_down */
    C->fall = -IN_TUMBLE;
    C->falling = -10000;
    C->origin.x = 0.0;
    C->origin.y = TUMBLE_HEIGHT;
    C->origin.z = 0.0;

    start_angle[C->id] = -(float)(C->vec_ptr + 1) * 0.5 * M_PI;
    spin[C->id] = (float)rand() / (float)RAND_MAX < 0.5 ? -1.0 : 1.0;

    /* change direction - must match with the below */
    angle = spin[C->id] * 0.75 * M_PI + start_angle[C->id];
    C->direction.x = cos(angle);
    C->direction.y = -3.0;
    C->direction.z = sin(angle);

    C->view = vadd(C->origin, C->direction);
}

/*
 * handle the starting tumble down onto the grid
 */
void tumble_down(CYCLE *C) {
    float fallfact;
    double angle;

    C->fall += speed_fac;
    if (C->fall > 0.0) {
        C->fall = 0.0;
        C->falling = 0;
    }

    fallfact = -(C->fall) / IN_TUMBLE; /* 1 at start, 0 when down on grid */

    /* interp origin between high central point and final */
    C->origin.x = (1.0 - fallfact) * start_pos[C->id].origin.x;
    C->origin.z = (1.0 - fallfact) * start_pos[C->id].origin.z;
    C->origin.y = fallfact * TUMBLE_HEIGHT + (1.0 - fallfact) * HEIGHT;

    /* change direction */
    angle = spin[C->id] * 0.75 * M_PI * fallfact + start_angle[C->id];
    C->direction.x = cos(angle);
    C->direction.y = -3.0 * fallfact;
    C->direction.z = sin(angle);

    /* restore initial state... */
    if (C->falling == 0)
        bcopy((void *)&start_pos[C->id], (void *)C, sizeof(CYCLE));
}

#ifdef SHOW_TIMING
void show_time(double frame_time, float min) {
    char pic_time_str[128];

    zbuffer(FALSE);
#ifdef RGB_MODE
    cpack(0x00ff00);
#else
    color(GREEN);
#endif
    cmov(-3.0, 1.0, -2.0);
    sprintf(pic_time_str, "id: %d level: %d frames/sec: %.2g speed fac:%.2g min: %.2g bhv: %d, %s", good->id,
            good->level, (float)(1.0 / frame_time), speed_fac, min, good->behave, good->type == PERSON ? "P" : "R");
    charstr(pic_time_str);
    zbuffer(TRUE);
}
#endif

void write_player_list(float x, float y, float y_off) {
    int i;

    for (i = 0; i < CYCLES; i++) {
        if (used[i] && bike[i].alive) {
            /* write level patch */
#ifdef RGB_MODE
            cpack(level_col[bike[i].level]);
#else
            color(level_col[bike[i].level]);
#endif
            rectf(x, y, x - 0.5 * y_off, y - 0.8 * y_off);

            /* write up R in owner colour if it's there */
            if (bike[i].type == ROBOT) {
#ifdef RGB_MODE
                cpack(trail_col[bike[bike[i].owner].trail_colour]);
#else
                color(trail_col[bike[bike[i].owner].trail_colour]);
#endif
                cmov2(x - y_off, y);
                if (bike[i].behave)
                    charstr("R+");
                else
                    charstr("R");
            }

            /* write player name */
            if (bike[i].falling > 0)
#ifdef RGB_MODE
                cpack(0xffffff);
#else
                color(WHITE);
#endif
            else
#ifdef RGB_MODE
                cpack(trail_col[bike[i].trail_colour]);
#else
                color(trail_col[bike[i].trail_colour]);
#endif
            cmov2(x - 2 * y_off, y);
            charstr(bike[i].name);
            y += y_off;
        }
    }
}

void draw_info(int look_offset, int help_mode, float min, int pts, int big_kills, int trail_kills) {
    float speed, v[2];
    short speed255;
    char junk[64];
    static int min_colour = 0;

    zbuffer(FALSE);
    mmode(MVIEWING);
    ortho2(0.0, 1.0, 0.0, 1.0);

    if (!look_offset && good->falling == 0) {
        /* draw a little instrument backdrop thing */
#ifdef RGB_MODE
        cpack(GREY25);
#else
        color(GREY25);
#endif
        bgnpolygon();
        v[0] = 0.5 - 0.2;
        v[1] = 0.0;
        v2f(v);
        v[0] = 0.5 - 0.18;
        v[1] = 0.07;
        v2f(v);
#if defined(SHADING) && defined(RGB_MODE)
        cpack(GREY50);
#endif
        v[0] = 0.5 - 0.13;
        v[1] = 0.14;
        v2f(v);
        v[0] = 0.5 + 0.13;
        v[1] = 0.14;
        v2f(v);
#if defined(SHADING) && defined(RGB_MODE)
        cpack(GREY25);
#endif
        v[0] = 0.5 + 0.18;
        v[1] = 0.07;
        v2f(v);
        v[0] = 0.5 + 0.2;
        v[1] = 0.0;
        v2f(v);
        endpolygon();

        /* and draw a border around the console */
#ifdef RGB_MODE
        cpack(0xffffff);
#else
        color(WHITE);
#endif
        bgnclosedline();
        v[0] = 0.5 - 0.2;
        v[1] = 0.0;
        v2f(v);
        v[0] = 0.5 - 0.18;
        v[1] = 0.07;
        v2f(v);
        v[0] = 0.5 - 0.13;
        v[1] = 0.14;
        v2f(v);
        v[0] = 0.5 + 0.13;
        v[1] = 0.14;
        v2f(v);
        v[0] = 0.5 + 0.18;
        v[1] = 0.07;
        v2f(v);
        v[0] = 0.5 + 0.2;
        v[1] = 0.0;
        v2f(v);
        endclosedline();

        /* interp between blue and red */
        speed = good->step / MAX_STEP;
        speed255 = speed * 255;

#ifndef RGB_MODE
        color(WHITE);
#endif
        bgntmesh();
#ifdef RGB_MODE
        RGBcolor(0, 0, 255);
#endif
        v[0] = 0.5 - 0.17;
        v[1] = 0.01;
        v2f(v);
        v[1] = 0.03;
        v2f(v);
#ifdef RGB_MODE
        RGBcolor(speed255, 0, 255 - speed255);
#endif
        v[0] = 0.5 - 0.17 + speed * 0.34;
        v[1] = 0.01;
        v2f(v);
        v[1] = 0.03 + speed * 0.03;
        v2f(v);
        endtmesh();

        /* draw up our own colour patch */
#ifdef RGB_MODE
        cpack(trail_col[good->trail_colour]);
#else
        color(trail_col[good->trail_colour]);
#endif
        bgntmesh();
        v[0] = 0.5 - 0.165;
        v[1] = 0.07;
        v2f(v);
        v[0] = 0.5 - 0.125;
        v[1] = 0.125;
        v2f(v);
        v[0] = 0.5 + 0.165;
        v[1] = 0.07;
        v2f(v);
        v[0] = 0.5 + 0.125;
        v[1] = 0.125;
        v2f(v);
        endtmesh();

        /* write up a couple of score numbers */
#ifdef RGB_MODE
        cpack(0);
#else
        color(BLACK);
#endif
        sprintf(junk, "%d", pts);
        cmov2(0.5 - 0.12, 0.08);
        charstr(junk);

#if 0
	/* tmp @@@ some kill numbers */
	sprintf(junk, "b %d t %d", big_kills, trail_kills);
	cmov2(0.5, 0.08);
	charstr(junk);
#endif

        /*
         * draw a flashing proximity square if we're < some dist
         * from a collision
         */
        if (min < 0.05 * DIM) {
            min_colour++;
            min_colour %= 2;
            if (min_colour)
#ifdef RGB_MODE
                cpack(0);
            else
                cpack(0x0000ff);
#else
                color(BLACK);
            else
                color(RED);
#endif
            bgntmesh();
            v[0] = 0.52 + 0.03;
            v[1] = 0.095 - 0.015;
            v2f(v);
            v[0] = 0.62 - 0.03;
            v2f(v);
            v[0] = 0.52 + 0.01;
            v[1] = 0.095 - 0.012;
            v2f(v);
            v[0] = 0.62 - 0.01;
            v2f(v);
            v[0] = 0.52;
            v[1] = 0.095;
            v2f(v);
            v[0] = 0.62;
            v2f(v);
            v[0] = 0.52 + 0.01;
            v[1] = 0.095 + 0.012;
            v2f(v);
            v[0] = 0.62 - 0.01;
            v2f(v);
            v[0] = 0.52 + 0.03;
            v[1] = 0.095 + 0.015;
            v2f(v);
            v[0] = 0.62 - 0.03;
            v2f(v);
            endtmesh();
        }
    }

    /* write up a list of players */
    write_player_list(0.01, 0.97, -0.03);

    /* write a left/right/rear indicator if necessary */
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    cmov2(0.9, 0.95);
    switch (look_offset) {
    case 1:
        charstr("left");
        break;
    case 2:
        charstr("rear");
        break;
    case 3:
        charstr("right");
        break;
    }

    if (help_mode)
        instructions();

    /* restore normal settings */
    zbuffer(TRUE);
    set_win_coords();
}

void set_win_coords(void) { window(-2.0, 2.0, -1.0, 1.0, 1.0, DIM * 3.0); }

/* Open graphics window*/
void openwindow(void) {
    keepaspect(2, 1);
    minsize(600, 300);
    winopen("Cycles");

#ifdef RGB_MODE
    RGBmode();
#else
    cmode();
    mapcolor(GREY25, 25, 25, 25);
    mapcolor(GREY50, 50, 50, 50);
    mapcolor(trail_col[TRAIL0], 0, 255, 255);
    mapcolor(trail_col[TRAIL1], 255, 0, 255);
    mapcolor(trail_col[TRAIL2], 255, 255, 0); /* @@@ this is bad, rgb or restore */
    mapcolor(trail_col[TRAIL3], 0, 255, 86);
    mapcolor(trail_col[TRAIL4], 0, 86, 255);
    mapcolor(trail_col[TRAIL5], 255, 0, 86);
#endif

    doublebuffer();
    gconfig();

    zbuffer(TRUE);
#ifdef SHADING
    shademodel(GOURAUD);
#else
    shademodel(FLAT); /* GOURAUD for trail shading */
#endif

    mmode(MVIEWING);
    set_win_coords();
#ifdef RGB_MODE
    cpack(0x0);
#else
    color(BLACK);
#endif
    clear();
    zclear();
    swapbuffers();

    qdevice(LEFTMOUSE);
    qdevice(RIGHTMOUSE);
    qdevice(HKEY);
    qdevice(ESCKEY);
    qdevice(SPACEKEY);
    qdevice(LEFTARROWKEY);
    qdevice(RIGHTARROWKEY);
}

void instructions(void) {
    zbuffer(FALSE);
    mmode(MVIEWING);
    ortho2(0.0, 1.0, 0.0, 1.0);

#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif
    /* also found in screens.c */
    cmov2(0.2, 0.9);
    charstr("turn  - left and right mouse buttons");
    cmov2(0.2, 0.85);
    charstr("speed - hold down A or middle mouse to");
    cmov2(0.2, 0.8);
    charstr("        accelerate. lift off to slow down");
    cmov2(0.2, 0.75);
    charstr("jump  - space bar\n");
    cmov2(0.2, 0.7);
    charstr("look  - use left and right arrows to look around");
    cmov2(0.2, 0.65);
    charstr("help  - H toggles these instructions");
    cmov2(0.2, 0.6);
    charstr("quit  - ESC key");

    zbuffer(TRUE);
    set_win_coords();
}

void draw_all_2d(void) {
    int i, j, level, segment;
    float v[3];
    CYCLE *C;
    static float z_rot = 0;

    z_rot += speed_fac;
    if (z_rot > 360.0)
        z_rot -= 360.0;

    pushmatrix();
    rot(75, 'x');
    rot(z_rot, 'z');

    for (level = 0; level < LEVELS; level++) {

        zbuffer(TRUE);
        v[2] = level * 0.5 * DIM;

        /* draw black platform */
#ifdef RGB_MODE
        cpack(0);
#else
        color(BLACK);
#endif
        bgntmesh();
        v[0] = -DIM;
        v[1] = -DIM;
        v3f(v);
        v[1] = DIM;
        v3f(v);
        v[0] = DIM;
        v[1] = -DIM;
        v3f(v);
        v[1] = DIM;
        v3f(v);
        endtmesh();

        zbuffer(FALSE);

        /* border in level colours */
#ifdef RGB_MODE
        cpack(level_col[level]);
#else
        color(level_col[level]);
#endif
        bgnclosedline();
        v[0] = -DIM;
        v[1] = -DIM;
        v3f(v);
        v[1] = DIM;
        v3f(v);
        v[0] = DIM;
        v3f(v);
        v[1] = -DIM;
        v3f(v);
        endclosedline();

        /* draw trails */
        for (j = 0; j < CYCLES; j++) {
            C = &bike[j];

            if (C->falling == 0)
#ifdef RGB_MODE
                cpack(trail_col[C->trail_colour]);
            else
                cpack(0xffffff);
#else
                color(trail_col[C->trail_colour]);
            else
                color(WHITE);
#endif
            if (used[j] && C->alive && j != good->id && C->falling >= 0) {
                /*
                 * all but 1st trail element
                 */
                segment = 0;
                i = C->trail_ptr;
                do {
                    if (C->trail[i].level == level) {
                        if (!segment) {
                            segment = 1;
                            bgnline();
                        }
                        v[0] = C->trail[i].x;
                        v[1] = C->trail[i].z;
                        v3f(v);
                    } else if (segment) {
                        segment = 0;
                        endline();
                    }
                    i = (i - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
                } while (i != C->trail_ptr && C->trail[i].level != -1);
                if (segment)
                    endline();

                if (C->level == level) {
                    /*
                     * 1st trail: from bike to 1st corner
                     */
                    i = C->trail_ptr;
                    if (MAX(ABS(C->trail[i].x - C->origin.x), ABS(C->trail[i].z - C->origin.z)) > CYCLE_TAIL) {
                        bgnline();
                        v[0] = C->trail[i].x;
                        v[1] = C->trail[i].z;
                        v3f(v);
#if defined(SHADING) && defined(RGB_MODE)
                        cpack(0xffffff);
#endif
                        v[0] = C->origin.x - CYCLE_TAIL * vec[C->vec_ptr][0];
                        v[1] = C->origin.z - CYCLE_TAIL * vec[C->vec_ptr][1];
                        v3f(v);
                        endline();
                    }
                }
            }
            /* draw dot for jumping bike */
            if (C->level == level && used[j] && C->alive && j != good->id && C->jump) {
#ifdef RGB_MODE
                cpack(0xffffff);
#else
                color(WHITE);
#endif
                v[0] = C->origin.x;
                v[1] = C->origin.z;
                pushmatrix();
                translate(0.0, 0.0, v[2]);
                circf(v[0], v[1], 3);
                popmatrix();
            }
        }
    }

    popmatrix();
}
