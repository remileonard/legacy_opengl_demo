
#include <stdlib.h>
#include <math.h>
#include "../porting/iris2ogl.h"
#include "cycles.h"

Object tail_obj, engine_obj, chassis_top;
Object back_stuff[LEVELS];

#define WALL_HT_0 (0.5*DIM)
#define WALL_HT_1 DIM
#define WALL_HT_2 DIM

void init_circ_data(void);

/* bike shape defs */
#define SPOKES
#define WHEEL_WIDTH   CYCLE_WIDTH
#define CHASSIS_WIDTH CYCLE_WIDTH
#ifdef RGB_MODE
#   define SHADED_SPOKES
#endif
#define     GREY100 0x646464
#define     GREY150 0x969696
#define    BIKE_TOP 0x0
#define   BIKE_SIDE 0x0
#define    BIKE_BOT 0xffffff
#define BIKE_WINDOW 0xffffff
#define no_FLAT_SIDES
#define no_OUTLINING
#ifdef SPOKES
#   define NUM_SPOKES 3
#endif
#define CIRC_PTS 18
/*
 * wheel rotation speeds, eg. rear:
 * v/r=theta, radius 1.55,  1/1.55 = 0.645, and 180/pi = 57.3
 * and is in 10ths of degrees so 10* :
 * rot speed = 0.645*57.3*10 = 370
 */
#define ROT_SPEED_REAR  370.0
#define ROT_SPEED_FRONT 409.0


static float wheel_side[CIRC_PTS][3];
static float wheel_top[3*CIRC_PTS][3];
extern int wheel_angle[CYCLES][2];
extern int trail_col[COLOURS], level_col[LEVELS];
extern CPOINT up_hole[LEVELS], down_hole[LEVELS];

static float rear[8][3]  = {	{ 0.4*CHASSIS_WIDTH, 4.4,  0.0},/* back */
				{ 0.3*CHASSIS_WIDTH, 3.8, -0.5},
				{ 0.8*CHASSIS_WIDTH, 2.7, -2.9},/* front */
				{ 0.8*CHASSIS_WIDTH, 3.4, -2.5},
				{-0.4*CHASSIS_WIDTH, 4.4,  0.0},/* back */
				{-0.3*CHASSIS_WIDTH, 3.8, -0.5},
				{-0.8*CHASSIS_WIDTH, 2.7, -2.9},/* front */
				{-0.8*CHASSIS_WIDTH, 3.4, -2.5} };

static float engine[6][3] = {	{ 0.9*CHASSIS_WIDTH, 2.7, -5.1},
				{ 0.2*CHASSIS_WIDTH, 0.6, -4.3},/* bot */
				{ 0.5*CHASSIS_WIDTH, 3.3, -2.9},/* back */
				{-0.2*CHASSIS_WIDTH, 0.6, -4.3},/* bot */
				{-0.5*CHASSIS_WIDTH, 3.3, -2.9},/* back */
				{-0.9*CHASSIS_WIDTH, 2.7, -5.1} };

static float bubble[15][3] = {	{ 0.4*CHASSIS_WIDTH, 3.8, -2.5},/* back */
				{     CHASSIS_WIDTH, 3.3, -4.4},
				{ 0.5*CHASSIS_WIDTH, 4.7, -4.8},
				{     CHASSIS_WIDTH, 3.0, -5.5},
				{ 0.5*CHASSIS_WIDTH, 4.6, -6.0},
				{ 0.2*CHASSIS_WIDTH, 3.3, -7.0},/* front */
				{ 0.0, 3.9, -6.9},
				{-0.2*CHASSIS_WIDTH, 3.3, -7.0},/* front */
				{-0.5*CHASSIS_WIDTH, 4.6, -6.0},
				{    -CHASSIS_WIDTH, 3.0, -5.5},
				{-0.5*CHASSIS_WIDTH, 4.7, -4.8},
				{    -CHASSIS_WIDTH, 3.3, -4.4},
				{-0.4*CHASSIS_WIDTH, 3.8, -2.5},/* back */
				{ 0.2*CHASSIS_WIDTH, 3.0, -6.8},/* "chin"... */
				{-0.2*CHASSIS_WIDTH, 3.0, -6.8} };

/*
 * setup data for later circle drawing
 */
void init_circ_data(void) {
    int i;
    float radius = 1.4, x, z;
    double angle;

    for (i = 0; i < CIRC_PTS; i++) {
	angle = 2.0*M_PI*(float)i/(float)(CIRC_PTS - 1);
	x = radius*cos(angle);
	z = radius*sin(angle);

	/* a wheel side */
	wheel_side[i][0] = 0.0;
	wheel_side[i][1] = x;
	wheel_side[i][2] = z;

	/* the tyre */
	wheel_top[i*3][0] = -WHEEL_WIDTH;
	wheel_top[i*3][1] = x;
	wheel_top[i*3][2] = z;

	wheel_top[i*3+1][0] = 0.0;
	wheel_top[i*3+1][1] = 1.1*x;
	wheel_top[i*3+1][2] = 1.1*z;

	wheel_top[i*3+2][0] = WHEEL_WIDTH;
	wheel_top[i*3+2][1] = x;
	wheel_top[i*3+2][2] = z;
    }
}


/*
 * define and create all objects to be drawn to the screen
 * during play.
 */
void make_objs(void) {
    int i;

    for (i = 0; i < LEVELS; i++) back_stuff[i] = genobj();
    tail_obj = genobj();
    engine_obj = genobj();
    chassis_top = genobj();

    init_circ_data();

    /*
     * even newer disconnected bubble chassis
     */
    /* rear tail thing */
    makeobj(tail_obj);
#ifdef RGB_MODE
	bgntmesh();
	cpack(0xffff00);
	v3f(rear[2]);
	cpack(0xff00ff);
	v3f(rear[1]);
	cpack(0x00ff00);
	v3f(rear[4]);
	cpack(0xff0000);
	v3f(rear[3]);
	cpack(0x00ffff);
	v3f(rear[6]);
	cpack(0x0000ff);
	v3f(rear[5]);
	v3f(rear[0]);
	cpack(0xff00ff);
	v3f(rear[7]);
	cpack(0xffff00);
	v3f(rear[2]);
	endtmesh();
#else
	color(BLUE);
	bgntmesh();
	v3f(rear[2]);
	v3f(rear[1]);
	v3f(rear[4]);
	v3f(rear[3]);
	v3f(rear[6]);
	v3f(rear[5]);
	v3f(rear[0]);
	v3f(rear[7]);
	v3f(rear[2]);
	endtmesh();
#endif
    closeobj();

    /* engine */
    makeobj(engine_obj);
	/* covered front */
	bgntmesh();
#ifdef RGB_MODE
	cpack(GREY100);
#else
	color(BLACK);
#endif
	v3f(engine[2]);
#ifdef RGB_MODE
	cpack(BIKE_SIDE);
#endif
	v3f(engine[1]);
	v3f(engine[0]);
	v3f(engine[3]);
	v3f(engine[5]);
#ifdef RGB_MODE
	cpack(GREY100);
#endif
	v3f(engine[4]);
	endtmesh();
    closeobj();

    /* top */
    makeobj(chassis_top);
	bgntmesh();
#ifdef RGB_MODE
	cpack(BIKE_TOP);
#else
	color(BLACK);
#endif
	v3f(bubble[0]);
	v3f(bubble[1]);
	v3f(bubble[2]);
	v3f(bubble[3]);
	v3f(bubble[4]);
	v3f(bubble[5]);
#ifdef RGB_MODE
	cpack(BIKE_WINDOW);
#else
	color(WHITE);
#endif
	v3f(bubble[6]);
#ifdef RGB_MODE
	cpack(BIKE_TOP);
#else
	color(BLACK);
#endif
	v3f(bubble[7]);
	v3f(bubble[8]);
	v3f(bubble[9]);
	v3f(bubble[10]);
	v3f(bubble[11]);
	v3f(bubble[12]); /* end outer */
	endtmesh();
	bgntmesh(); /* top */
	v3f(bubble[0]);
	v3f(bubble[12]);
	v3f(bubble[2]);
	v3f(bubble[10]);
#ifdef RGB_MODE
	cpack(BIKE_WINDOW);
#else
	color(WHITE);
#endif
	v3f(bubble[4]);
	v3f(bubble[8]);
	v3f(bubble[6]);
	endtmesh();
	bgntmesh(); /* chin thing under the front */
	v3f(bubble[3]);
#ifdef RGB_MODE
	cpack(BIKE_TOP);
#else
	color(BLACK);
#endif
	v3f(bubble[5]);
	v3f(bubble[13]);
	v3f(bubble[7]);
	v3f(bubble[14]);
#ifdef RGB_MODE
	cpack(BIKE_WINDOW);
#else
	color(WHITE);
#endif
	v3f(bubble[9]);
	endtmesh();
    closeobj();

    /*
     * Draw some background objects around the place
     */
    makeobj(back_stuff[0]);
    {
	float v[3];

	zbuffer(FALSE);

	v[2] = DIM;

	/* draw a big wall up one side */
#ifdef RGB_MODE
	cpack(0x333333);
#else
	color(GREY50);
#endif
	bgntmesh();
	v[0] = -DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = DIM;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x666666);
#endif
	v[0] = -DIM;
	v[1] = WALL_HT_0;
	v3f(v);
	v[0] = DIM;
	v3f(v);
	endtmesh();

	/*
	 * then add some interesting shapes and outlines
	 */

	/* top corner triangle */
#ifdef RGB_MODE
	cpack(0x333333);
#else
	color(GREY25);
#endif
	bgntmesh();
	v[0] = -DIM;
	v[1] = 0.4*WALL_HT_0;
	v3f(v);
	v[1] = WALL_HT_0;
	v3f(v);
	v[0] = -0.6*DIM;
	v3f(v);
	endtmesh();
#ifdef RGB_MODE
	cpack(0xffffff);
#else
	color(WHITE);
#endif
	bgnline();
	v3f(v);
	v[0] = -DIM;
	v[1] = 0.25*DIM;
	v3f(v);
	endline();

	/* and tri way out to side */
#ifdef RGB_MODE
	cpack(0x333333);
#else
	color(GREY25);
#endif
	bgntmesh();
	v[0] = -DIM;
	v[1] = 0.4*WALL_HT_0;
	v3f(v);
	v[1] = WALL_HT_0;
	v3f(v);
	v[0] = -6.0*DIM;
	v3f(v);
	endtmesh();
#ifdef RGB_MODE
	cpack(0xffffff);
#else
	color(WHITE);
#endif
	bgnline();
	v3f(v);
	v[0] = -DIM;
	v[1] = 0.25*DIM;
	v3f(v);
	endline();

	/* dark triangle */
#ifdef RGB_MODE
	cpack(0x222222);
#else
	color(BLACK);
#endif
	bgntmesh();
	v[0] = -0.3*DIM;
	v[1] = 0.3*WALL_HT_0;
	v3f(v);
	v[0] = 0.0;
	v3f(v);
#ifdef RGB_MODE
	cpack(0);
#endif
	v[1] = 0.8*WALL_HT_0;
	v3f(v);
	endtmesh();

	/* hidden stripe */
#ifdef RGB_MODE
	cpack(GREY25);
#else
	color(GREY25);
#endif
	bgntmesh();
	v[0] = 0.2*DIM;
	v[1] = 0;
	v3f(v);
	v[0] = 0.3*DIM;
	v3f(v);
	v[0] = -0.2*DIM;
	v[1] = WALL_HT_0;
	v3f(v);
	v[0] = -0.1*DIM;
	v3f(v);
	endtmesh();

	/* stripe */
#ifdef RGB_MODE
	cpack(GREY50);
#else
	color(GREY25);
#endif
	bgntmesh();
	v[0] = -0.8*DIM;
	v[1] = 0;
	v3f(v);
	v[0] = -0.4*DIM;
	v3f(v);
	v[0] = -0.2*DIM;
	v[1] = WALL_HT_0;
	v3f(v);
	v[0] = 0.2*DIM;
	v3f(v);
	endtmesh();

	/* double line */
#ifdef RGB_MODE
	cpack(0xffffff);
#else
	color(WHITE);
#endif
	bgnclosedline();
	v[0] = -0.8*DIM;
	v[1] = 0;
	v3f(v);
	v[0] = -0.82*DIM;
	v3f(v);
	v[0] = -0.22*DIM;
	v[1] = WALL_HT_0;
	v3f(v);
	v[0] = -0.2*DIM;
	v3f(v);
	endclosedline();

	/* mid wall triangle */
#ifdef RGB_MODE
	cpack(GREY50);
#else
	color(GREY25);
#endif
	bgntmesh();
	v[0] = 0.3*DIM;
	v[1] = 0.3*WALL_HT_0;
	v3f(v);
	v[1] = 0.8*WALL_HT_0;
	v3f(v);
	v[0] = 0.1*DIM;
	v3f(v);
	endtmesh();

	/* shape out to one side */
#ifdef RGB_MODE
	cpack(0x333333);
#else
	color(GREY50);
#endif
	bgntmesh();
	v[0] = DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = 1.5*DIM;
	v[1] = 0.3*WALL_HT_0;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x666666);
#endif
	v[0] = DIM;
	v[1] = WALL_HT_0;
	v3f(v);
	v[0] = 1.5*DIM;
	v[1] = 0.7*WALL_HT_0;
	v3f(v);
	endtmesh();

	/* top rightangle */
#ifdef RGB_MODE
	cpack(0xffffff);
#else
	color(WHITE);
#endif
	bgnline();
	v[0] = 0.4*DIM;
	v[1] = WALL_HT_0;
	v3f(v);
	v[1] = 0.7*WALL_HT_0;
	v3f(v);
	v[0] = 1.5*DIM;
	v3f(v);
	endline();
	bgnline();
	v[0] = 0.6*DIM;
	v[1] = WALL_HT_0;
	v3f(v);
	v[1] = 0.9*WALL_HT_0;
	v3f(v);
	v[0] = 1.15*DIM;
	v3f(v);
	endline();

	/* bottom corner shaded dark box */
#ifdef RGB_MODE
	cpack(0x666666);
#else
	color(GREY25);
#endif
	bgntmesh();
	v[0] = 0.5*DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = 0.8*DIM;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x333333);
#endif
	v[0] = 0.5*DIM;
	v[1] = 0.5*WALL_HT_0;
	v3f(v);
	v[0] = 0.8*DIM;
	v3f(v);
	endtmesh();

	zbuffer(TRUE);
    }
    closeobj();

    /*
     * roughly hexagon
     */
    makeobj(back_stuff[1]);
    {
	float v[3];

	v[2] = DIM;

	zbuffer(FALSE);

	/* big wall */
#ifdef RGB_MODE
	cpack(0x808080);
#else
	color(GREY50);
#endif
	bgntmesh();
	v[0] = 1.15*DIM;
	v[1] = 0.25*WALL_HT_1;
#ifdef RGB_MODE
	cpack(0x444444);
#endif
	v3f(v);
	v[0] = 0.85*DIM;
	v[1] = 0.85*WALL_HT_1;
#ifdef RGB_MODE
	cpack(0x808080);
#endif
	v3f(v);
	v[0] = 0.4*DIM;
	v[1] = 0.0;
#ifdef RGB_MODE
	cpack(0x222222);
#endif
	v3f(v);
	v[0] = 0.69*DIM;
	v[1] = 0.9*WALL_HT_1;
#ifdef RGB_MODE
	cpack(0x808080);
#endif
	v3f(v);
	v[0] = -0.4*DIM;
	v[1] = 0.0;
#ifdef RGB_MODE
	cpack(0x222222);
#endif
	v3f(v);
	v[0] = 0.3*DIM;
	v[1] = 0.55*WALL_HT_1;
#ifdef RGB_MODE
	cpack(0x808080);
#endif
	v3f(v);
	v[0] = -1.0*DIM;
	v[1] = 0.2*WALL_HT_1;
#ifdef RGB_MODE
	cpack(0x444444);
#endif
	v3f(v);
	v[0] = -0.35*DIM;
	v[1] = 1.0*WALL_HT_1;
#ifdef RGB_MODE
	cpack(0x808080);
#endif
	v3f(v);
	v[0] = -0.65*DIM;
	v[1] = 0.9*WALL_HT_1;
	v3f(v);
	v[0] = -0.4*DIM;
	v[1] = 1.0*WALL_HT_1;
	v3f(v);
	endtmesh();

	/* two left side polygons  */
#ifdef RGB_MODE
	cpack(0x3f3f3f);
#endif
	bgnpolygon();
	v[0] = -0.8*DIM;
	v[1] = 0.6*WALL_HT_1;
	v3f(v);
	v[0] = -1.3*DIM;
	v3f(v);
	v[1] = 0.9*WALL_HT_1;
	v3f(v);
	v[0] = -0.45*DIM;
	v[1] = 1.2*WALL_HT_1;
	v3f(v);
	endpolygon();

#ifdef RGB_MODE
	cpack(0x4b4b4b);
#endif
	bgnpolygon();
	v[0] = -0.8*DIM;
	v[1] = 0.6*WALL_HT_1;
	v3f(v);
	v[0] = -1.3*DIM;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x606060);
#endif
	v[1] = 0.3*WALL_HT_1;
	v3f(v);
	v[0] = -1.0*DIM;
	v[1] = 0.2*WALL_HT_1;
	v3f(v);
	endpolygon();

	/* white lines */
#ifdef RGB_MODE
	cpack(0xffffff);
#endif
	bgnline();
	v[0] = -1.3*DIM;
	v[1] = 0.6*WALL_HT_1;
	v3f(v);
	v[0] = -0.4*DIM;
	v3f(v);
	v[1] = 0.5*WALL_HT_1;
	v3f(v);
	v[0] = -0.85*DIM;
	v[1] = 0.5*WALL_HT_1;
	v3f(v);
	endline();

	/* doorthingo  */
	bgnpolygon();
	v[0] = 0.25*DIM;
	v[1] = 0.5*WALL_HT_1;
	cpack(0x9e9e9e);
	v3f(v);
	v[0] = -0.25*DIM;
	v3f(v);
	v[0] = -0.2*DIM;
	v[1] = 0.0;
	cpack(0x555555);
	v3f(v);
	v[0] = 0.2*DIM;
	v3f(v);
	endpolygon();

	/*
	 * 2 black triangles
	 */
#ifdef RGB_MODE
        cpack(0);
#endif
        bgntmesh();
        v[0] = -0.5*DIM;
        v[1] = 0.2*WALL_HT_1;
        v3f(v);
        v[0] = -0.3*DIM;
        v3f(v);
        v[1] = 0.4*WALL_HT_1;
        v3f(v);
        endtmesh();

        bgntmesh();
        v[0] = -0.3*DIM;
        v[1] = 0.45*WALL_HT_1;
        v3f(v);
        v[0] = -0.5*DIM;
        v3f(v);
        v[1] = 0.25*WALL_HT_1;
        v3f(v);
        endtmesh();

	/* top polygon and triangle */
#ifdef RGB_MODE
	cpack(0x707070);
#endif
	bgnpolygon();
	v[0] = 0.06*DIM;
	v[1] = 0.71*WALL_HT_1;
	v3f(v);
	v[0] = 0.35*DIM;
	v[1] = 0.5*WALL_HT_1;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x565656);
#endif
	v[0] = 0.69*DIM;
	v[1] = 0.9*WALL_HT_1;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x505050);
#endif
	v[0] = 0.4*DIM;
	v[1] = 1.0*WALL_HT_1;
	v3f(v);
	v[0] = 0.35*DIM;
	v3f(v);
	endpolygon();

#if 0
#ifdef RGB_MODE
	cpack(0x1a1a1a);
#else
	color(GREY25);
#endif
	bgntmesh();
	v[0] = -0.35*DIM;
	v[1] = 1.0*WALL_HT_1;
	v3f(v);
	v[0] = 0.35*DIM;
	v3f(v);
	v[0] = 0.06*DIM;
	v[1] = 0.71*WALL_HT_1;
	v3f(v);
	endtmesh();
#endif

#ifdef RGB_MODE
	cpack(0xffffff);
#endif
	bgnline();
	v[0] = 0.35*DIM;
	v[1] = 0.5*WALL_HT_1;
	v3f(v);
	v[0] = -0.35*DIM;
	v[1] = 1.0*WALL_HT_1;
	v3f(v);
	endline();

	/* dark strip to left with white highlights */
#ifdef RGB_MODE
	cpack(0x232323);
#endif
	bgnpolygon();
	v[0] = 0.231*DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = 0.3*DIM;
	v3f(v);
	v[0] = 1.07*DIM;
	v[1] = 0.42*WALL_HT_1;
	v3f(v);
	v[0] = 1.0*DIM;
	v[1] = 0.56*WALL_HT_1;
	v3f(v);
	endpolygon();

#ifdef RGB_MODE
	cpack(0xffffff);
#else
	color(WHITE);
#endif
	bgnline();
	v[0] = 0.231*DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = 1.0*DIM;
	v[1] = 0.56*WALL_HT_1;
	v3f(v);
	endline();

#ifdef RGB_MODE
	cpack(0xffffff);
#else
	color(WHITE);
#endif
	bgnline();
	v[0] = 0.3*DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = 1.07*DIM;
	v[1] = 0.42*WALL_HT_1;
	v3f(v);
	endline();

	/* polygon to right */
#ifdef RGB_MODE
	cpack(0x393939);
#endif
	bgnpolygon();
	v[0] = 1.3*DIM;
	v[1] = 0.7*WALL_HT_1;
	v3f(v);
	v[1] = 0.3*WALL_HT_1;
	v3f(v);
	v[0] = 1.15*DIM;
	v[1] = 0.25*WALL_HT_1;
	v3f(v);
	v[0] = 0.85*DIM;
	v[1] = 0.85*WALL_HT_1;
	v3f(v);
	endpolygon();

	/* silly offsided square things */
	pushmatrix();
	translate(-0.4*DIM, 0.0, 0.0);
	rot(-19.0, 'z');

#ifdef RGB_MODE
	cpack(0x4e4e4e);
#endif
	bgnpolygon();
	v[0] = -0.25*DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = -0.45*DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = -0.45*DIM;
	v[1] = 0.18*WALL_HT_1;
	v3f(v);
	v[0] = -0.25*DIM;
	v[1] = 0.18*WALL_HT_1;
	v3f(v);
	endpolygon();

	bgnpolygon();
	v[0] = -0.25*DIM;
	v[1] = 0.31*WALL_HT_1;;
	v3f(v);
	v[0] = -0.45*DIM;
	v[1] = 0.31*WALL_HT_1;;
	v3f(v);
	v[0] = -0.45*DIM;
	v[1] = 0.52*WALL_HT_1;
	v3f(v);
	v[0] = -0.25*DIM;
	v[1] = 0.52*WALL_HT_1;
	v3f(v);
	endpolygon();

	bgnpolygon();
	v[0] = -0.25*DIM;
	v[1] = 0.6*WALL_HT_1;;
	v3f(v);
	v[0] = -0.45*DIM;
	v[1] = 0.6*WALL_HT_1;;
	v3f(v);
	v[0] = -0.45*DIM;
	v[1] = 0.8*WALL_HT_1;
	v3f(v);
	v[0] = -0.25*DIM;
	v[1] = 0.8*WALL_HT_1;
	v3f(v);
	endpolygon();

	zbuffer(TRUE);
	popmatrix();

    }
    closeobj();

    /*
     * 2 huge trangles
     */
    makeobj(back_stuff[2]);
    {
	float v[3];

	v[2] = DIM;

	zbuffer(FALSE);
/*
 *****************middle box
 */

#ifdef RGB_MODE
	cpack(0);
#endif
	bgntmesh();
	v[0] = 0.3*DIM;
	v[1] = 0.38*WALL_HT_2;
	v3f(v);
	v[1] = 0.0;
	v3f(v);
#ifdef RGB_MODE
	cpack(0xcccccc);
#endif
	
	v[0] = -0.3*DIM;
	v[1] = 0.38*WALL_HT_2;
	v3f(v);
	v[1] = 0.0;
	v3f(v);
	endtmesh();	

/*
 ****************left maintriangle
 */

#ifdef RGB_MODE
	cpack(0x666666);
#else
	color(GREY50);
#endif
	bgntmesh();
	v[0] = -1.0*DIM;
	v[1] = 1.0*WALL_HT_2;
	v3f(v);
	v[0] = 0.0*DIM;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x333333);
#endif
	v[0] = -0.5*DIM;
	v[1] = 0.0;
	v3f(v);
	endtmesh();

/*
 * ****************connecting strip
 */

#ifdef RGB_MODE
	cpack(0x888888);
#endif
	bgnpolygon();
	v[0] = 0.3*DIM;
	v[1] = 0.25*WALL_HT_2;
	v3f(v);
	v[0] = 0.4*DIM;
	v[1] = 0.4*WALL_HT_2;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x1e1e1e);
#endif
	v[0] = -0.4*WALL_HT_2;
	v[1] = 1.0*WALL_HT_2;
	v3f(v);
	v[0] = -0.7*DIM;
	v3f(v);
	endpolygon();
/*
 *********************right main triangle
 */
#ifdef RGB_MODE
	cpack(0x666666);
#endif
	bgntmesh();
	v[0] = 0.1*DIM;
	v[1] = 0.0;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x333333);
#endif
	v[0] = 1.5*DIM;
	v3f(v);
#ifdef RGB_MODE
	cpack(0x666666);
#endif
	v[0] = 0.6*DIM;
	v[1] = 0.8*WALL_HT_2;
	v3f(v);
	endtmesh();
/*
 * ************box in right triangle & its lines
 */

#ifdef RGB_MODE
	cpack(0xffffff);
#endif

	bgnline();
	v[0] = 0.75*DIM;
	v[1] = 0.35*WALL_HT_2;
	v3f(v);
	v[0] = 1.0*DIM;
	v[1] = 0.6*WALL_HT_2;
	v3f(v);
	v[0] = 5.0*DIM;
	v[1] = 0.6*WALL_HT_2;
	v3f(v);
	endline();

	bgnline();
	v[0] = 0.75*DIM;
	v[1] = 0.27*WALL_HT_2;
	v3f(v);
	v[0] = 1.05*DIM;
	v[1] = 0.55*WALL_HT_2;
	v3f(v);
	v[0] = 5.0*WALL_HT_2;
	v[1] = 0.55*DIM;
	v3f(v);
	endline();
	

#ifdef RGB_MODE
	cpack(0x0d0d0d);
#endif
	bgnpolygon();
	v[0] = 0.5*DIM;
	v[1] = 0.1*WALL_HT_2;
	v3f(v);
	v[0] = 0.75*DIM;
	v3f(v);
	v[1] = 0.4*WALL_HT_2;
	v3f(v);
	v[0] = 0.5*DIM;
	v3f(v);
	endpolygon();

/*
 * **************beams right triangle
 */


#ifdef RGB_MODE
	cpack(0x888888);
#endif
	bgntmesh();
	v[0] = 0.7*DIM;
	v[1] = 0.0;
	v3f(v);
	v[0] = 0.85*DIM;
	v3f(v);
	v[0] = 1.1*DIM;
	v[1] = 0.45*DIM;
	v3f(v);
	v[0] = 1.2*WALL_HT_2;
	v[1] = 0.4*WALL_HT_2;
	v3f(v);
	v[0] = 5.0*WALL_HT_2;
	v[1] = 0.45*DIM;
	v3f(v);
	v[0] = 5.0*WALL_HT_2;
	v[1] = 0.4*WALL_HT_2;
	v3f(v);
	endtmesh();

/*
 *********************shoots up left triangle
 */
#ifdef RGB_MODE
	cpack(0xffffff);
#endif

	bgntmesh();
	v[0] = -0.55*DIM;
	v[1] = 0.3*WALL_HT_2;
	v3f(v);
	v[0] = -0.53*DIM;
	v[1] = 0.3*WALL_HT_2;
	v3f(v);
	v[0] = -0.5*DIM;
	v[1] = 0.4*WALL_HT_2;
	v3f(v);
	endtmesh();
	bgntmesh();
	v[0] = -0.45*DIM;
	v[1] = 0.3*WALL_HT_2;
	v3f(v);
	v[0] = -0.47*DIM;
	v[1] = 0.3*WALL_HT_2;
	v3f(v);
	v[0] = -0.5*DIM;
	v[1] = 0.4*WALL_HT_2;
	v3f(v);
	endtmesh();

	bgntmesh();
	v[0] = -0.55*DIM;
	v[1] = 0.5*WALL_HT_2;
	v3f(v);
	v[0] = -0.53*DIM;
	v[1] = 0.5*WALL_HT_2;
	v3f(v);
	v[0] = -0.5*DIM;
	v[1] = 0.6*WALL_HT_2;
	v3f(v);
	endtmesh();
	bgntmesh();
	v[0] = -0.45*DIM;
	v[1] = 0.5*WALL_HT_2;
	v3f(v);
	v[0] = -0.47*DIM;
	v[1] = 0.5*WALL_HT_2;
	v3f(v);
	v[0] = -0.5*DIM;
	v[1] = 0.6*WALL_HT_2;
	v3f(v);
	endtmesh();

	bgntmesh();
	v[0] = -0.55*DIM;
	v[1] = 0.7*WALL_HT_2;
	v3f(v);
	v[0] = -0.53*DIM;
	v[1] = 0.7*WALL_HT_2;
	v3f(v);
	v[0] = -0.5*DIM;
	v[1] = 0.8*WALL_HT_2;
	v3f(v);
	endtmesh();
	bgntmesh();
	v[0] = -0.45*DIM;
	v[1] = 0.7*WALL_HT_2;
	v3f(v);
	v[0] = -0.47*DIM;
	v[1] = 0.7*WALL_HT_2;
	v3f(v);
	v[0] = -0.5*DIM;
	v[1] = 0.8*WALL_HT_2;
	v3f(v);
	endtmesh();

	zbuffer(TRUE);
    }
    closeobj();
}


/*
 * Draw the game grid
 */
void draw_coloured_grid(int level) {
    float i;
    float start[3], end[3], v1[3], v2[3], v4[3], v[3];

#ifdef RGB_MODE
    /* draw a polygonal background */
    cpack(0x330000);

    /* floor */
    bgnpolygon();
    v1[0] = -DIM;
    v1[1] = 0.0;
    v1[2] = -DIM;
    v3f(v1);
    v1[0] = DIM;
    v3f(v1);
    v1[2] = DIM;
    v3f(v1);
    v1[0] = -DIM;
    v3f(v1);
    endpolygon();

    /* walls */
    cpack(0x333333);
    bgntmesh();
    v1[0] = -DIM;
    v1[1] = 0.0;
    v1[2] = -DIM;
    v3f(v1);
    v1[1] = 10.0;
    v3f(v1);

    v1[0] = DIM;
    v1[1] = 0.0;
    v3f(v1);
    v1[1] = 10.0;
    v3f(v1);

    v1[2] = DIM;
    v1[1] = 0.0;
    v3f(v1);
    v1[1] = 10.0;
    v3f(v1);

    v1[0] = -DIM;
    v1[1] = 0.0;
    v3f(v1);
    v1[1] = 10.0;
    v3f(v1);

    v1[2] = -DIM;
    v1[1] = 0.0;
    v3f(v1);
    v1[1] = 10.0;
    v3f(v1);
    endtmesh();

    zbuffer(FALSE);
#endif

    /*
     * draw holes for level changes
     */
#ifdef RGB_MODE
    cpack(0xffffff);
#else
    color(WHITE);
#endif

    /* up hole */
    bgntmesh();
    v[0] = up_hole[level].x - HOLE_SIZE;
    v[1] = 0.0;
    v[2] = up_hole[level].z - HOLE_SIZE;
    v3f(v);
    /* centre */
#ifdef RGB_MODE
    cpack(level_col[(level+LEVELS-1)%LEVELS]);
#endif
    v[0] = up_hole[level].x;
    v[2] = up_hole[level].z;
    v3f(v);
#ifdef RGB_MODE
    cpack(0xffffff);
#endif
    v[0] = up_hole[level].x + HOLE_SIZE;
    v[2] = up_hole[level].z - HOLE_SIZE;
    v3f(v);
    swaptmesh();
    v[0] = up_hole[level].x + HOLE_SIZE;
    v[2] = up_hole[level].z + HOLE_SIZE;
    v3f(v);
    swaptmesh();
    v[0] = up_hole[level].x - HOLE_SIZE;
    v[2] = up_hole[level].z + HOLE_SIZE;
    v3f(v);
    swaptmesh();
    v[0] = up_hole[level].x - HOLE_SIZE;
    v[2] = up_hole[level].z - HOLE_SIZE;
    v3f(v);
    endtmesh();

    /* down hole */
    bgntmesh();
    v[0] = down_hole[level].x - HOLE_SIZE;
    v[1] = 0.0;
    v[2] = down_hole[level].z - HOLE_SIZE;
    v3f(v);
    /* centre */
#ifdef RGB_MODE
    cpack(level_col[(level+1)%LEVELS]);
#endif
    v[0] = down_hole[level].x;
    v[2] = down_hole[level].z;
    v3f(v);
#ifdef RGB_MODE
    cpack(0xffffff);
#endif
    v[0] = down_hole[level].x + HOLE_SIZE;
    v[2] = down_hole[level].z - HOLE_SIZE;
    v3f(v);
    swaptmesh();
    v[0] = down_hole[level].x + HOLE_SIZE;
    v[2] = down_hole[level].z + HOLE_SIZE;
    v3f(v);
    swaptmesh();
    v[0] = down_hole[level].x - HOLE_SIZE;
    v[2] = down_hole[level].z + HOLE_SIZE;
    v3f(v);
    swaptmesh();
    v[0] = down_hole[level].x - HOLE_SIZE;
    v[2] = down_hole[level].z - HOLE_SIZE;
    v3f(v);
    endtmesh();



    /* now draw the grid lines */
#ifdef RGB_MODE
    cpack(level_col[level]);
#else
    color(level_col[level]);
#endif
    start[1] = end[1] = 0.0;
    start[0] = -DIM;
    end[0] = DIM;
    v2[0] = -DIM;
    v2[1] = 10.0;
    v4[0] = DIM;
    v4[1] = 10.0;
    for (i = -DIM; i < DIM + 1.0; i += 10.0) {
	start[2] = end[2] = v2[2] = v4[2] = i;
	bgnline();
	v3f(v2);
	v3f(start);
	v3f(end);
	v3f(v4);
	endline();
    }
    start[2] = -DIM;
    end[2] = DIM;
    v2[2] = -DIM;
    v4[2] = DIM;
    for (i = -DIM; i < DIM + 1.0; i += 10.0) {
	start[0] = end[0] = v2[0] = v4[0] = i;
	bgnline();
	v3f(v2);
	v3f(start);
	v3f(end);
	v3f(v4);
	endline();
    }
    v1[0] = -DIM;
    v1[1] = 10.;
    v1[2] = -DIM;
    bgnclosedline();
    v3f(v1);
    v1[0] = DIM;
    v3f(v1);
    v1[2] = DIM;
    v3f(v1);
    v1[0] = -DIM;
    v3f(v1);
    endclosedline();
#ifdef RGB_MODE
    zbuffer(TRUE);
#endif
}


/*
 * draw a groovy spoked bulbous wheel
 */
void draw_wheel(int lo_res) {
	float v[3];
	register int i, j;

	if (2*CIRC_PTS > 255) {
	    printf("too many tyre pts %d\n", 2*CIRC_PTS);
	    exit(1);
	}

#ifdef RGB_MODE
	cpack(GREY100);
#else
	color(GREY25);
#endif

	/* tyre (CIRC_PTS*4 verts, CIRC_PTS*4 colours) */
	if (lo_res) {
	    /* draw a flat top */
	    bgntmesh();
	    for (i = 0; i < CIRC_PTS; i++) {
		v3f(wheel_top[i*3]);
		v3f(wheel_top[i*3+2]);
	    }
	    endtmesh();
	}
	else {
	    /* left side of tyre */
	    bgntmesh();
	    for (i = 0; i < CIRC_PTS; i++) {
#ifdef RGB_MODE
		cpack(GREY100);
#endif
		v3f(wheel_top[i*3]);
#ifdef RGB_MODE
		cpack(GREY150);
#endif
		v3f(wheel_top[i*3+1]);
	    }
	    endtmesh();

	    /* right side of tyre */
	    bgntmesh();
	    for (i = 0; i < CIRC_PTS; i++) {
#ifdef RGB_MODE
		cpack(GREY150);
#endif
		v3f(wheel_top[i*3+1]);
#ifdef RGB_MODE
		cpack(GREY100);
#endif
		v3f(wheel_top[i*3+2]);
	    }
	    endtmesh();
	}

#ifdef FLAT_SIDES
/* CIRC_PTS*2 verts, 1 colour */

#ifdef RGB_MODE
	cpack(GREY50);
#else
	color(GREY50);
#endif

	pushmatrix();
	translate(WHEEL_WIDTH - 0.1, 0.0, 0.0);
	bgnpolygon();
	for (i = 0; i < CIRC_PTS; i++) v3f(wheel_side[i]);
	endpolygon();
	popmatrix();

	pushmatrix();
	translate(-WHEEL_WIDTH + 0.1, 0.0, 0.0);
	bgnpolygon();
	for (i = 0; i < CIRC_PTS; i++) v3f(wheel_side[i]);
	endpolygon();
	popmatrix();

#else
/* non flat sides */
/* CIRC_PTS*2 verts, 3 colours (SHADED_SPOKES: CIRC_PTS*2 colours) */

	v[1] = 0.0;
	v[2] = 0.0;
	/* one concave side wall */
	pushmatrix();

	translate(0.5*WHEEL_WIDTH + 0.05, 0.0, 0.0);

#ifdef SPOKES
	/* a few spokes */
#ifndef SHADED_SPOKES
#ifdef RGB_MODE
	cpack(GREY50);
#else
	color(WHITE);
#endif
#endif
	v[0] = -0.5*WHEEL_WIDTH;
	v[1] = v[2] = 0.0;
	for (j = 0; j < NUM_SPOKES; j++) {
	    bgntmesh();
#ifdef SHADED_SPOKES
	    cpack(GREY50);
#endif
	    v3f(v);
#ifdef SHADED_SPOKES
	    cpack(0xffffff);
#endif
	    v3f(wheel_side[CIRC_PTS*j/NUM_SPOKES]);
	    v3f(wheel_side[CIRC_PTS*j/NUM_SPOKES + CIRC_PTS/(3*NUM_SPOKES)]);
	    endtmesh();
	}
#else
	/* solid concave wheel */
	bgntmesh();
#ifdef SHADED_SPOKES
	cpack(0);
#else
#ifdef RGB_MODE
	cpack(0);
#else
	color(GREY50);
#endif
#endif
	v3f(wheel_side[0]);
	cpack(0xffffff);
	/* wheel centre */
	v[0] = -0.5*WHEEL_WIDTH;
	v3f(v);
#ifndef SHADED_SPOKES
#ifdef RGB_MODE
	cpack(GREY50);
#else
	color(GREY50);
#endif
#endif
	for (i = 1; i < CIRC_PTS; i++) {
#ifdef SHADED_SPOKES
	    cpack((i%2) ? GREY50 : 0);
#endif
	    v3f(wheel_side[i]);
	    swaptmesh();
	}
	endtmesh();
/* end spokes: */
#endif
	popmatrix();

	/* the other side wall */
	pushmatrix();
	translate(-0.5*WHEEL_WIDTH - 0.05, 0.0, 0.0);

#ifdef SPOKES
	/* a few spokes */
#ifndef SHADED_SPOKES
#ifdef RGB_MODE
	cpack(GREY50);
#else
	color(WHITE);
#endif
#endif
	v[0] = 0.5*WHEEL_WIDTH;
	v[1] = v[2] = 0.0;
	for (j = 0; j < NUM_SPOKES; j++) {
	    bgntmesh();
#ifdef SHADED_SPOKES
	cpack(GREY50);
#endif
	    v3f(v);
#ifdef SHADED_SPOKES
	    cpack(0xffffff);
#endif
	    v3f(wheel_side[CIRC_PTS*j/NUM_SPOKES]);
	    v3f(wheel_side[CIRC_PTS*j/NUM_SPOKES + CIRC_PTS/(3*NUM_SPOKES)]);
	    endtmesh();
	}
#else
	/* solid concave wheel */
	bgntmesh();
#ifdef SHADED_SPOKES
	cpack(0);
#else
#ifdef RGB_MODE
	cpack(0xdd1111);
#else
	color(BLUE);
#endif
#endif
	v3f(wheel_side[0]);
#ifdef RGB_MODE
	cpack(0xffffff);
#else
	color(WHITE);
#endif
	/* wheel centre */
	v[0] = 0.5*WHEEL_WIDTH;
	v3f(v);
#ifndef SHADED_SPOKES
#ifdef RGB_MODE
	cpack(0xdd1111);
#else
	color(BLUE);
#endif
#endif
	for (i = 1; i < CIRC_PTS; i++) {
#ifdef SHADED_SPOKES
	    cpack((i%2) ? GREY50 : 0);
#endif
	    v3f(wheel_side[i]);
	    swaptmesh();
	}
	endtmesh();
/* end spokes: */
#endif
	popmatrix();
/* end not flat sides: */
#endif

#ifdef OUTLINING
#ifdef RGB_MODE
	cpack(0xffffff);
#else
	color(WHITE);
#endif

	pushmatrix();
	translate(WHEEL_WIDTH - 0.1, 0.0, 0.0);
	bgnline();
	for (i = 0; i < CIRC_PTS; i++) v3f(wheel_side[i]);
	endline();
	popmatrix();

	pushmatrix();
	translate(-WHEEL_WIDTH + 0.1, 0.0, 0.0);
	bgnline();
	for (i = 0; i < CIRC_PTS; i++) v3f(wheel_side[i]);
	endline();
	popmatrix();
#endif
}


void draw_cute_flag(int colour) {
    float v[3];

#ifdef RGB_MODE
    cpack(trail_col[colour]);
#else
    color(trail_col[colour]);
#endif

    v[0] = 0.0;
    v[2] = -0.1*CYCLE_LENGTH;
    bgnline();
    v[1] = 4.0;
    v3f(v);
    v[1] = 6.0;
    v3f(v);
    endline();

    bgntmesh();
    v3f(v);
    v[1] -= 0.5;
    v[2] += 1.0;
    v3f(v);
    v[1] -= 0.5;
    v[2] -= 1.0;
    v3f(v);
    endtmesh();
}


/*
 * draw a new cycle
 */
void draw_cycle(int id, float step, int colour, int lo_res, int robot, int flag_colour) {
    wheel_angle[id][0] += (int)(step*ROT_SPEED_REAR);
    wheel_angle[id][1] += (int)(step*ROT_SPEED_FRONT);
    wheel_angle[id][0] %= 3600;
    wheel_angle[id][1] %= 3600;

    pushmatrix();
    translate(0.0, 1.55, -1.3);
    rotate(-wheel_angle[id][0], 'x');
    draw_wheel(lo_res);
    popmatrix();

    pushmatrix();
    translate(0.0, 1.40, -6.4);
    rotate(-wheel_angle[id][1], 'x');
    scale(0.9, 0.9, 0.9);
    draw_wheel(lo_res);
    popmatrix();

    if (robot) draw_cute_flag(flag_colour);

    pushmatrix();
    callobj(tail_obj);
    callobj(engine_obj);
    callobj(chassis_top);
    popmatrix();
}
