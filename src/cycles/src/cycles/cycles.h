
#include "../porting/iris2ogl.h"

#define DIM 200.0	    /* size of playing field */
#define TRAIL_HEIGHT 3.0    /* height of trails  */
#define TRAIL_LENGTH 50     /* only remembers last TRAIL_LENGTH trail sections
                               (MAX 128 'cos of drawing) */
#define NAME_SIZE 16	    /* max size of persons name */
#define GRAVITY 1.0	    /* acceleration of gravity */
#define JUMP_POWER 5.0	    /* how fast you take off */
#define HEIGHT 5.0	    /* cycle height */
#define WALL_FALL 25	    /* number of units it takes for an exploded
                               cycle's trail to drop */
#define TUMBLE_HEIGHT 100   /* height that drop onto grid from */
#define IN_TUMBLE 30	    /* time to drop onto grid */
#define CYCLES 20	    /* Total max number of players (including us) */
#define REAL_SPEED 10.0	    /* arbitrary speed ratio between frame
                               updates, real time and speed of cycles
			       ie. the frame rate at which the cycle
			       speeds need no adjustment */
#define MIN_STEP 1.0	    /* allowable cycle steps */
#define MAX_STEP 10.0

#define CYCLE_WIDTH 0.7	    /* cycle width */
#define CYCLE_LENGTH 8.0    /* length of bike */
#define CYCLE_VIEW_PT 5.0   /* pos on the cycle we look from (measured from rear) */
#define CYCLE_TAIL CYCLE_VIEW_PT
#define CYCLE_NOSE (CYCLE_LENGTH - CYCLE_TAIL)

#define NET_TIMEOUT 5	    /* seconds before we've lost the network */
#define LEVELS 3	    /* number of different playing grids */
#define HOLE_SIZE 10.0	    /* size of teleport holes (0.05*DIM) */


/* debugging, timing, shading etc flags */
#define no_SHOW_TIMING
#define RGB_MODE
#define no_DEBUG

#ifdef RGB_MODE
#   define SHADING
#else
#   define no_SHADING
#endif


/* indexes into trail colour array */
#define COLOURS 6
#define TRAIL0 0
#define TRAIL1 1
#define TRAIL2 2
#define TRAIL3 3
#define TRAIL4 4
#define TRAIL5 5

#ifdef RGB_MODE
#define GREY25 0x191919
#define GREY50 0x323232
#else
#define GREY25 100
#define GREY50 101
#endif

// Rename POINT to CPOINT to avoid conflict with Windows POINT
typedef struct {
    Coord x,y,z;
    short level;
} CPOINT;

/*
 * CYCLE STRUCTURE:
 *  direction: where the cycle is facing
 *  view: used in 'lookat' statement (what is cycle lookint towards)
 *  origin: where is cycle
 *  trail: last TRAIL_LENGTH turning points of cycle
 *  step: speed of cycle
 *  jump_speed: vertical speed of cycle during jump
 *  trail_ptr: how many trail points have been used
 *  jump: flag for determining whether cycle is jumping or not
 *  fall: index for counting the fall of a wall at cycle death
 *        and for counting the initial fall down onto the grid
 *  falling: is the same as a fall, just it's an integer
 *  trail_colour: colour of trail
 *  lturn,rturn: flags to tell if a left or right turn has been recorded
 *  vec_ptr: pointer to direction vector
 *  alive: flag for aliveness
 *  id: unique network id for this cycle (between 0 and CYCLES-1)
 * 
 * NOTE: all networked information must come before the trail[] data,
 *       as this (and all data after it) are not always sent.
 */

typedef struct {
    /* networked data */
    CPOINT direction, origin;
    int trail_ptr, jump, trail_colour, alive, falling;
    int quit, id, vec_ptr, level, who_we_hit;
    float fall, step;
    CPOINT trail_update;
    /* sometimes networked data */
    CPOINT trail[TRAIL_LENGTH];
    char name[NAME_SIZE];
    /* local data */
    CPOINT view;
    float jump_speed;
    int lturn, rturn, behave, type, owner, pts, games;
} CYCLE;

/* for use in the mcast routines */
#define SHORT_PACKET (3*sizeof(CPOINT) + 10*sizeof(int) + 2*sizeof(float))
#define FULL_PACKET  sizeof(CYCLE)
#define PERSON 0
#define ROBOT  1

#define ABS(x)    ((x)<0.0?-(x):(x))

/*
 * prototypes
 */
#ifdef SHOW_TIMING
void show_time(double, float);
#endif

/* externally used mcast stuff */
int get_and_sort_mcasts(void);
void send_all_full_mcast(void);
void send_full_mcast(CYCLE *);
void send_update_mcast(void);
void init_network(int *);
void init_comms(void);
void parse_args(int, char **);
int wait_for_replies(void);
void kill_dead_cycle(void);

/* screen stuff */
void score_screen(int, int, int);
void title_screen(int, char *, int *, int *);
void init_fonts(void);
void scale_fonts_to_win(void);

/* the rest... */
void do_scoring(CYCLE *, int *, int *, int *);
void draw_coloured_grid(int );
void init_holes(void);
int fall_down_hole(CYCLE *);
void new_level(CYCLE *, int );
void draw_cute_flag(int );
void write_player_list(float, float,  float);
void move_our_robots(void);
void draw_all_2d(void);
void check_outside(CYCLE *);
void draw_cycle(int, float, int, int, int, int);
void draw_wheel(int);
void set_win_coords(void);
void search_for_exit(void);
void init_tumble(CYCLE *);
void tumble_down(CYCLE *);
void draw_info(int, int, float, int, int, int);
void set_speed_fac(int);
void explode_me(CYCLE *);
void drawgrid(int, int);
void make_objs(void);
void init_pos(CYCLE *);
CPOINT vadd (CPOINT, CPOINT);
void drawtrail(CYCLE *, int);
void drawcycle(CYCLE *, int, int);
void calcorg(CYCLE *);
void explode(CYCLE *, int);
void openwindow(void);
void instructions(void);
void newtrail(CYCLE *C);
void turn(CYCLE *, float); 
void jump(CYCLE *);
void mov(CYCLE *);
float block(int, float, float, int, int *);
void move_cycles(CYCLE *);
int main(int, char **);
