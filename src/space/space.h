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
#undef SP_IRIS_GL
#undef SP_OPEN_GL
#define SP_OPEN_GL 1

#include <stdint.h>

typedef int8_t schar8;
typedef uint8_t uchar8;
typedef int16_t sint16;
typedef uint16_t uint16;
typedef int32_t sint32;
typedef uint32_t uint32;
typedef float flot32;
typedef double flot64;

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>

#define fsqrt(x) ((flot32)sqrt((double)(x)))
#define fcos(x) ((flot32)cos((double)(x)))
#define fsin(x) ((flot32)sin((double)(x)))
#define fasin(x) ((flot32)asin((double)(x)))
#define ftan(x) ((flot32)tan((double)(x)))
#define flog10(x) ((flot32)log10((double)(x)))
#define flog(x) ((flot32)log((double)(x)))
#define fatan2(y, x) ((flot32)atan2((double)(y), (double)(x)))
#define fatan(x) ((flot32)atan((double)(x)))
#define ftanh(x) ((flot32)tanh((double)(x)))
#define fsinh(x) ((flot32)sinh((double)(x)))
#define fexp(x) ((flot32)exp((double)(x)))
#define fpow(x, y) ((flot32)pow((double)(x), (double)(y)))
#ifdef SP_OPEN_GL
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>


typedef float Matrix[4][4];

typedef struct {
    sint32 win; /* GLUT window ID */
    uint32 hwid;
} t_wndw;

#define SP_IO_esc 0x001b
#define SP_IO_a 0x0061
#define SP_IO_s 0x0073
#define SP_IO_x 0x0078
#define SP_IO_h 0x0068
#define SP_IO_l 0x006c
#define SP_IO_v 0x0076
#define SP_IO_r 0x0072
#define SP_IO_t 0x0074
#define SP_IO_y 0x0079
#define SP_IO_d 0x0064
#define SP_IO_n 0x006e
#define SP_IO_z 0x007a
#define SP_IO_i 0x0069
#define SP_IO_o 0x006f
#define SP_IO_q 0x0071
#define SP_IO_b 0x0062
#define SP_IO_u 0x0075
#define SP_IO_pri 0xff61
#define SP_IO_tid 0x002d
#define SP_IO_tco 0xdddd /* xxx */
#define SP_IO_up 0xff52
#define SP_IO_dwn 0xff54
#define SP_IO_snd 0xeeee /* xxx */
#define SP_IO_0 0x0030
#define SP_IO_1 0x0031
#define SP_IO_2 0x0032
#define SP_IO_3 0x0033
#define SP_IO_4 0x0034
#define SP_IO_5 0x0035
#define SP_IO_6 0x0036
#define SP_IO_7 0x0037
#define SP_IO_8 0x0038
#define SP_IO_9 0x0039
#define SP_IO_pnt 0x002e
#define SP_IO_com 0x002c
#define SP_IO_ret 0x000d
#define SP_IO_lsh 0x00e1
#define SP_IO_rsh 0x00e2
#define SP_IO_lct 0x00e3
#define SP_IO_rct 0x00e4
#endif

#ifdef SP_IRIS_GL
#include <device.h>
#include <gl/gl.h>

typedef struct {
    sint32 wid;
    uint32 hwid;
} t_wndw;

#define SP_IO_esc ESCKEY
#define SP_IO_a BUT10
#define SP_IO_s BUT11
#define SP_IO_x BUT20
#define SP_IO_h BUT26
#define SP_IO_l BUT41
#define SP_IO_v BUT28
#define SP_IO_r BUT23
#define SP_IO_t BUT24
#define SP_IO_y BUT31
#define SP_IO_d BUT17
#define SP_IO_n BUT36
#define SP_IO_z BUT19
#define SP_IO_i BUT39
#define SP_IO_o BUT40
#define SP_IO_q BUT9
#define SP_IO_b BUT35
#define SP_IO_u BUT32
#define SP_IO_pri BUT157
#define SP_IO_tid BUT46
#define SP_IO_tco 0xf0
#define SP_IO_up BUT80
#define SP_IO_dwn BUT73
#define SP_IO_snd BUT145
#define SP_IO_0 BUT45
#define SP_IO_1 BUT7
#define SP_IO_2 BUT13
#define SP_IO_3 BUT14
#define SP_IO_4 BUT21
#define SP_IO_5 BUT22
#define SP_IO_6 BUT29
#define SP_IO_7 BUT30
#define SP_IO_8 BUT37
#define SP_IO_9 BUT38
#define SP_IO_pnt BUT51
#define SP_IO_com BUT44
#define SP_IO_ret BUT50
#define SP_IO_lsh BUT5
#define SP_IO_rsh BUT4
#define SP_IO_lct BUT2
#define SP_IO_rct BUT144
#endif

#define DEBUG_FLAG 0x00000001
#define PRBIT_FLAG 0x00000002
#define MRBIT_FLAG 0x00000004
#define PRINT_FLAG 0x00000008
#define HELPP_FLAG 0x00000010
#define FRACT_FLAG 0x00000020
#define REGEN_FLAG 0x00000040
#define FLSCR_FLAG 0x00000080
#define STNAM_FLAG 0x00000100
#define ECLIP_FLAG 0x00000200
#define RINGW_FLAG 0x00000400
#define SHADE_FLAG 0x00000800
#define VELOC_FLAG 0x00001000
#define PANEL_FLAG 0x00002000
#define NOTXT_FLAG 0x00004000
#define AUTOP_FLAG 0x00008000
#define SPACB_FLAG 0x00010000
#define SHIFT_FLAG 0x00020000
#define TMREV_FLAG 0x00040000
#define CNTRL_FLAG 0x00080000
#define STATS_FLAG 0x00100000
#define ZODAC_FLAG 0x00200000
#define FREEZ_FLAG 0x00400000
#define TEXTR_FLAG 0x00800000
#define NSUBL_FLAG 0x01000000
#define SOUND_FLAG 0x02000000
#define HIRES_FLAG 0x04000000
#define NDPNT_FLAG 0x08000000
#define SLOWZ_FLAG 0x10000000
#define GEOSP_FLAG 0x20000000
#define DPATH_FLAG 0x40000000
#define USERM_FLAG 0x80000000

#define ACCUM_FLAG 0x00000000

#define HW_AAPNT 0x00000002
#define HW_AALIN 0x00000004
#define HW_TEXTU 0x00000008
#define HW_MMODE 0x00000010
#define HW_FATPT 0x00000020
#define HW_SOUND 0x00000040
#define HW_HIRES 0x00000080
#define HW_MULSA 0x00000100

#define SND_BACK 0
#define SND_BUTT 1
#define SND_AUON 2
#define SND_AUOF 3

#define SP_HW_RE 0x0001
#define SP_HW_VGX 0x0002
#define SP_HW_GT 0x0004
#define SP_HW_LG 0x0008
#define SP_HW_XG 0x0010
#define SP_HW_PI 0x0020
#define SP_HW_G 0x0040
#define SP_HW_UNKNOWN 0x0080

#define SP_LMOUSE 0x0001
#define SP_MMOUSE 0x0002
#define SP_RMOUSE 0x0004

#define STELL_STAT 2 /* stellar space */
#define GALAC_STAT 3 /* galactic space */
#define COSMC_STAT 4 /* cosmic space */

#define ECLPS_COLR 0xff00ff00 /* eclipse  color */
#define STELL_COLR 0xff0000ff /* stellar  color */
#define GALAC_COLR 0xffff0000 /* galactic color */
#define COSMC_COLR 0xff00ffff /* cosmic   color */

#define DTOR (M_PI / 180.0)
#define RTOD (180.0 / M_PI)
#define I2PI (0.5 / M_PI)
#define LIGHTSPEED 299792.458
#define GRAV 6.672e-20 /* (km*km*km)/(kg*s*s) */
#define LN2 log(2.0)
#define EARTHG 0.009808

#define AUTOKM 149597870.0
#define KMTOAU (1.0 / AUTOKM)
#define LYTOAU 63239.7
#define AUTOLY (1.0 / LYTOAU)
#define PRTOAU (180.0 * 3600.0 / M_PI)
#define AUTOPR (1.0 / PRTOAU)
#define PRTOLY 3.261633
#define LYTOPR (1.0 / PRTOLY)
#define KMTOPR (KMTOAU * AUTOPR)
#define PRTOKM (1.0 / KMTOPR)
#define KMTOLY (KMTOAU * AUTOLY)
#define LYTOKM (1.0 / KMTOLY)

#define GALAXY_EDGE 30000.0
#define STARSQ 256
#define EDGESQ (GALAXY_EDGE / STARSQ)
#define HEIGHT_ABOVE (0.005 * GALAXY_EDGE * GALAXY_EDGE)
#define SOLSYS_EDGE 0.02
#define RINGWEDGE (1.0 / 200.0)

#define SOL_X_GRID 192
#define SOL_Z_GRID 44

#define DODO_SIZE 2048
#define NUM_CLIP_PLANES 5
#define NUMBER_OF_STARS 2711
#define ANGULAR_SIZE 0.001

#define MAX_ELEV_BYTES (1024 * 1024 * 9)
#define MAX_COLR_BYTES (1024 * 1024 * 1)

#define FM_MERC_1B 1
#define FM_SINU_1B 2
#define FM_MERC_2B 3
#define FM_SINU_2B 4
#define FM_MERC_3B 5
#define FM_SINU_3B 6
#define FM_MERC_4B 7
#define FM_SINU_4B 8

#define FLAT_SPHERE 0
#define LIGT_SPHERE 1
#define TEXX_SPHERE 2
#define FRAC_PLANET 3
#define ELEV_PLANET 4
#define FRAC_RINGWO 5
#define ELEV_RINGWO 6

typedef struct {
    flot32 s, t;
} T2;
typedef struct {
    flot32 x, y, z;
} P3;
typedef struct {
    flot64 x, y, z;
} D3;
typedef struct {
    flot32 x, y, z, w;
} P4;
typedef struct {
    flot32 r, g, b;
} C3;
typedef struct {
    flot32 x, y, z;
} V3;
typedef struct {
    flot32 x, y, z, w;
} V4;
typedef struct {
    flot32 r, g, b, a;
} C4;

typedef struct {
    T2 t;
    flot32 a, b;
    V3 n;
    flot32 c;
    P3 p;
    flot32 d;
} P8;

typedef struct { /*-- 32 BYTES --*/
    P3 plan;
    flot32 delta;
    uint32 seed;
    sint32 label;
    uint32 cpack1, cpack2;
} TrigPoint;

typedef struct {
    sint32 x1, y1, x2, y2;
    uint32 butt;
    uint32 mask;
    uint32 col;
    char mes0[128];
    char mes1[128];
    char mes2[128];
} t_menu;

typedef struct {
    flot32 x, y, z;
    sint32 count;
    sint32 index[40];
    char name[32];
} t_bordr;

typedef struct {
    P4 arr[512];
    sint32 arr_count;
    sint32 brd_count;
    t_bordr brd[90];
} t_const;

typedef struct {
    char name[32];
    flot32 orb;
    flot32 ecc;
    flot32 inc;
    flot32 rad;
    flot64 mas;
    flot32 apo;
    flot32 yer;
    flot32 day;
    flot32 ee;
    flot32 ww;
    flot32 omega;
    flot32 r1, r2;
    char ring[32];

    uint32 col;
    uint32 tess;
    uint32 texsz;
    flot32 scale;
    sint32 lxsize, lysize;
    sint32 lformat;
    char elev[32];
    sint32 cxsize, cysize;
    sint32 cformat;
    char colr[32];

    sint32 moon_count;
    void *moons;
} t_system;

typedef struct t_body {
    t_system *ptr;
    D3 posit;
    flot32 distan;
    flot32 angsiz;
    flot32 orbsiz;
    flot32 cliprad;
    sint32 color;
    sint32 texdf;
    sint32 texrn;
    sint32 bodyobj[3];
    sint32 orbtobj[3];
    sint32 ringobj[3];
    char *land;
    char *colr;
    void *next[32];
} t_body;

typedef struct {
    sint32 plan_current, plan_old;
    sint32 moon_current, moon_old;
    sint32 suun_current, suun_old;
    sint32 suncount;
    t_body star[2];
    t_body stat;    /* cheap space station */
    t_body freedom; /* space station freedom */
} t_boss;

typedef struct { /*--- 48 Bytes ---*/
    flot32 x, y, z, abs_mag;
    flot32 r, g, b, a;
    flot32 scass, fny_mag;
    flot32 abs_mag2, scass2;
} t_stars;

typedef struct {
    sint32 count;
    t_stars *stars;
} t_galaga;

typedef struct {
    D3 p1;
    V3 v1;
    flot64 t1;
    D3 p2;
    V3 v2;
    flot64 t2;
    sint32 newstar[3];
} t_spline;

typedef struct {
    t_wndw winst;

    sint32 shmid;
    sint32 *shmad;
    sint32 mouse_x, mouse_y, mouse_b, mouse_n;
    sint32 rotsizex, rotsizey;
    sint32 winsizex, winsizey;
    sint32 winorigx, winorigy;
    sint32 x, y, z;
    flot32 cutoff;
    flot32 fps;
    sint32 stars_per_square;

    sint32 hw_graphics;
    uint32 alpha;

#ifdef SP_IRIS_GL
    uint32 sky_clear_color;
#endif
#ifdef SP_OPEN_GL
    sint32 fontbase;
    flot32 sky_clear_color[4];
#endif

    uint32 infoco;
    uint32 flags;
    flot32 feclipse;
    flot32 skyfrac;
    flot32 acdx, acdy;
    uint32 status;
    sint32 star_current;
    sint32 corona, galaxy, galobj[2], starobj, locun;
    sint32 constobj;
    sint32 shadowsqobj;
    sint32 noroll, attach;
    sint32 proj_count;

    D3 eye;
    V3 enorm;
    V3 vnorm;
    V3 light_vector;
    flot32 light_angle;
    flot32 viewangle, aspcratio, fov;
    flot32 timacc;
    flot64 D;
    flot64 S;

    sint32 click;
    flot32 timer, timer_old;

    char date[64];

    Matrix mat;
    t_spline spline;
    V4 clop[NUM_CLIP_PLANES];
} t_stopwatch;

/*************************** objects.c *****************************/
sint32 object_space_station(sint32);
static void make_toroid(flot32, flot32, sint32);
static void make_cylind(flot32, flot32, sint32, sint32);
void object_planet_orbit(t_body *, flot32);
sint32 object_planet_rings(t_system *, sint32);
sint32 object_shadow_squares(void);
sint32 make_constellations(void);

/***************************** watch.c *****************************/
flot64 check_timer(void);
flot64 delta_timer(void);
void reverse_julian_date(flot64, char *);

/***************************** main.c ******************************/
static void initialize_graphics(void);
static void fly(void);
static void print_screen_text(flot32);
static void blur_galaxy(uint32[256][256]);
sint32 make_galaxy_object(sint32);
void set_window_view(flot32 vian);
static void print_opening_credit(void);
char *datatrail(char *);

/****************************** god.c ******************************/
t_system *create_solar_system(sint32, t_boss *, t_galaga *);
void destroy_solar_system(t_boss *);
void scan_star_system(t_boss *);
static void create_one(t_body *, flot32);
static void destroy_one(t_body *);
void p_t_syst(t_system *);
void p_t_body(t_body *);
void p_t_boss(t_boss *);
static void generate_solar_system(t_boss *, sint32, t_stars *);
static uint32 generate_full_color(void);
static sint32 do_da_eclipse(t_boss *, D3 *, flot32);
static t_system *generate_ringworld_system(sint32, t_stars *);
void star_params(t_stars *, flot32 *, flot32 *, flot64 *);
static flot64 newton(flot64, flot64);
flot32 float_rand(void);
void calculate_orbital_params(D3 *, flot32 GMm, t_system *);

/***************************** sols.c ******************************/
void actually_do_graphics(t_boss *);
static void display_stellar_background(t_boss *);
static void display_galactic_background(void);
static void display_cosmic_background(void);
static void display_foreground(t_boss *);
static void display_ringworld_foreground(t_boss *);
static void draw_me(t_body *, t_boss *);
static sint32 sphere_clipp(D3 *, flot32);
static void prepare_for_fractal_planet(t_boss *, sint32);
void calc_posit(D3 *, t_system *, D3 *, flot32);
static void polyline_orbits(t_boss *);
static void calculate_luna(D3 *, D3 *);
sint32 find_closest_star(sint32 *, t_galaga *);
static void display_star_names(void);
static void total_solar_eclipse(t_boss *);
static void prepare_for_fractal_ringworld(t_boss *, sint32);
void draw_all_them_stars(sint32);
static void read_data_file(t_body *);
static void stall_message(char *, flot32, flot32);
static void special_perspective(flot32, flot32);
static void print_sub_geosphere_credit(void);
void accumulation(t_boss *flaggs);

/**************************** fract.c ******************************/
void generate_fractsphere(sint32, sint32);
void display_fractsphere(void);
void destroy_fractsphere(void);

/**************************  ringworld.c  **************************/
void generate_ringworld(sint32, sint32);
void display_ringworld(void);
void destroy_ringworld(void);

/**************************** world.c ******************************/
static void world_mid(sint32, sint32, sint32, sint32);
sint32 generate_sphere(sint32, sint32, sint32);

/*************************** events.c ******************************/
uint32 spOpenWindow(void);
void spGetWindowGeometry(void);
uint32 spInitFont(char *);
void spDrawString(float, float, float, char *);
void spInitKeyboard(void);
void spReadEvents(t_boss *);
void spInitSpaceball(void);
void spReadSpaceball(t_boss *);
void spInitCursor(void);
void spInitBell(void);
void spRingBell(void);
void spWaitForLeftButton(void);
void spSwapBuffers(void);
void spIdentifyMachine(void);
void spQuantifyMachine(void);
flot32 spReadFloat(void);
sint32 spReadStar(sint32[3]);

/**************************** input.c ******************************/
void initialize_time(void);
void initialize_shmem(t_boss *);
void read_time(void);
void evaluate_mouse(t_boss *);
void matrix_mouse(t_boss *);
void read_spaceball(t_boss *);
void key_press(t_boss *, sint32);
static void el_cheato(t_boss *, sint32, Matrix);
static void take_me_there(t_spline *);
void scan_galactic_system(t_boss *);
static void init_stars(void);
static sint32 init_cnstl(void);
static void generate_star_squares();
static void destroy_star_square(t_stars *);
void star_square_clip_check(sint32[4], sint32[18][18], P3 *);
static sint32 generate_galaxy_stars(void);
void sound_control(sint32, sint32);
static void stats_menu_stuff(t_boss *);
static sint32 autopilot_menu_stuff(t_boss *);

/**************************** menu.c ******************************/
void draw_menu(void);
void check_menu(t_boss *);
void draw_item(sint32, t_menu *);
void draw2_menu(sint32, t_menu *);
void draw_popup_menu(void);
void clear_popup_menu(void);
sint32 check2_menu(sint32, t_menu *);
void make_new_item(t_menu *, sint32, uint32, char *);

/************************** matrix.c ******************************/
void spMultMatrix(flot32 *, flot32 *, flot32 *);
void spTranMatrix(flot32 *, flot32 *);
void spCopyMatrix(flot32 *, flot32 *);
void spLookMatrix(flot32, flot32, flot32, flot32, flot32, flot32, Matrix);
void spPerspMatrix(flot32, flot32);

/************************** light.c ******************************/
void spInitLight(void);
void spLightMaterial(uint32, uint32);
void spSetLight(V3 *);

/************************* texture.c *****************************/
void spTevDef(void);
uint32 spTexDef(uint32, uint32, uint32, void *, uint32 flag);
void spFlipTex(uint32, uint32);

/*************************** sound.c ******************************/
void sound_effect(char *);
static void play_file(void);
static void play_sample(uint32);
static void reverse_buffer(unsigned char *);
