/*
 * mekton.h - Mekton, the mech combat demo by Silicon Graphics (1996),
 * rebuilt from the IRIX mekton_player executable.
 *
 * Function and global names come from the symbol table of the original
 * executable. Structure fields keep the byte offset they had in the
 * original layout in their comment (or in their name when their role is
 * unknown).
 */
#ifndef MEKTON_H
#define MEKTON_H

#include "pf.h"

#define MAX_MEKTONS 16

/* game states (cd_app->state) */
#define STATE_LOADING 0
#define STATE_INTRO 1
#define STATE_WAIT 2
#define STATE_DEAD 3
#define STATE_SETUP 7
#define STATE_PLAY 8

/* mek types */
#define VANTAGE 0
#define EX_CENTINEL 1

/* mek modes (Mek.mode) */
#define MODE_JETS 0x1       /* jump jets lit */
#define MODE_GROUND 0x4     /* standing on the ground */
#define MODE_FOOT 0x8       /* has feet, can land */
#define MODE_WALK 0x10      /* walking */
#define MODE_AIR 0x20       /* jumping */
#define MODE_FLY 0x40       /* Ex-Centinel in plane form */

/* a drawable object: a switch above an optional DCS above the model */
typedef struct Obj {
    int type;               /* 0: 0 model only, 1 with a DCS */
    pfSwitch *sw;           /* 4 */
    pfDCS *dcs;             /* 8 */
    pfVec3 trans;           /* 12 */
    pfVec3 rot;             /* 24 */
    int pad36;
    pfNode *model;          /* 40 */
} Obj;

/* per type constants, copied into each mek (mek_types) */
typedef struct MekType {
    float mass;             /* 288 */
    float drag;             /* 292 */
    float max_jet;          /* 296 */
    float f300;
    pfVec3 model_offset;    /* 304: model origin from the reference point */
    float height;           /* 316: eye of the ground probe */
    float radius;           /* 320 */
    float max_fwd;          /* 324: forward speed */
    float max_back;         /* 328: backward speed (negative) */
    float max_acc;          /* 332 */
    float max_dec;          /* 336 */
    float turn_rate;        /* 340 */
    float strafe;           /* 344 */
    float jet_up, jet_down; /* 348, 352: jump jet thrust */
    float jet_fwd, jet_back;/* 356, 360 */
    float jet_side;         /* 364 */
    float max_pitch;        /* 368 */
    float max_roll;         /* 372 */
    float air_turn;         /* 376 */
} MekType;

typedef struct Mek {
    unsigned char status;   /* 0: 0 alive, 2 dead */
    unsigned char team;     /* 1 */
    short mode;             /* 2: MODE_* */
    float tps;              /* 4: ticks per second the rates are scaled for */
    pfVec3 pos;             /* 8 */
    pfVec3 hpr;             /* 20 */
    pfVec3 vel;             /* 32: body frame, per tick */
    pfVec3 ang;             /* 44: hpr rates, per tick */
    pfVec3 origin;          /* 56: world position of the model origin */
    pfVec3 cpos;            /* 68: colony coordinates */
    float throttle;         /* 80: plane throttle */
    int hit_wall;           /* 84 */
    unsigned char anim;     /* 88: animation state */
    unsigned char anim89;
    float frame;            /* 92: animation frame */
    int score;              /* 96 */
    char name[16];          /* 100 */
    int type;               /* 116 */
    int net_id;             /* 120 */
    short s124;             /* 124 */
    unsigned char can_fly;  /* 126 */
    unsigned char flying;   /* 127: Ex-Centinel transformed */
    pfVec3 old_pos;         /* 128 */
    pfVec3 old_hpr;         /* 140 */
    unsigned int travmask;  /* 152 */
    Obj *obj;               /* 156: current model */
    Obj *walk_obj;          /* 160 */
    Obj *trans_obj;         /* 164: Ex-Centinel transformation */
    Obj *cg_obj;            /* 168: collision geometry */
    void *snd;              /* 172 */
    float in_fwd;           /* 176 */
    float in_turn;          /* 180 */
    float in_side;          /* 184 */
    float in_pitch;         /* 188: pitch while jumping or flying */
    float in_roll;          /* 192: roll while jumping or flying */
    float in_throttle;      /* 196 */
    int in_brake;           /* 200 */
    pfVec3 chpr;            /* 204: colony hpr */
    pfVec3 cpos2;           /* 216 */
    pfVec3 vmin;            /* 228: velocity limits from the animation */
    pfVec3 vmax;            /* 240 */
    pfVec3 aim;             /* 252: aim of the guns, heading and pitch */
    float heat;             /* 264: of the guns, they stop at 1 */
    int i268;
    int flags;              /* 272 */
    float f276, f280;
    int hit_by;             /* 284 */
    MekType t;              /* 288: from max_fwd on, per tick (set_mek_tps) */
} Mek;

typedef struct Input {
    int x, y;               /* 0, 4: pointer */
    int left;               /* 8: left button */
    int right;              /* 12: right button */
    int middle;             /* 16: middle button */
    int shift;              /* 20 */
    int ctrl;               /* 24 */
    int alt;                /* 28 */
    int tab;                /* 32 */
    int space;              /* 36 */
    int a, d, e, j, q, s, w; /* 40 .. 64: bit 0 held, bit 1 pressed */
    float stick[8];         /* 68 .. */
    int stick_buttons;      /* 100 */
} Input;

/* channel data passed to the draw callback (pass_insrument_data) */
typedef struct ChanData {
    int i0;
    int pipe;               /* 4 */
    int state;              /* 8 */
    int status;             /* 12: of my mech */
    float colony_angle;     /* 16 */
    pfVec3 eye;             /* 20 */
    struct Mek *mek;        /* 32 */
    short comm;             /* 36 */
    short shadow;           /* 38 */
    short master;           /* 40 */
    short slave;            /* 42 */
    int master_state;       /* 44: 0 waiting, 1 countdown, 2 playing, 3 over */
    float tps;              /* 48 */
    pfVec3 pos;             /* 52 */
    pfVec3 vel;             /* 64 */
    pfVec3 cpos;            /* 76 */
    pfVec3 cpos2;           /* 88 */
    pfVec3 hpr;             /* 100 */
    pfVec3 chpr;            /* 112 */
    float fov_changed;      /* 124 */
    int time;               /* 128: game time in seconds */
    int help;               /* 132 */
    int hud;                /* 136: 3 full .. 0 off */
    float climb;            /* 140: speed bars, actual and wanted */
    float want_climb_bar;   /* 144 */
    float climb_bar;        /* 148 */
    float fwd;              /* 152 */
    float want_fwd_bar;     /* 156 */
    float fwd_bar;          /* 160 */
    float side;             /* 164 */
    float want_side_bar;    /* 168 */
    float side_bar;         /* 172 */
    float aim_x, aim_y;     /* 176, 180: on the screen */
    float f184;
    float throttle;         /* 188 */
    float heat;             /* 192 */
    int damage;             /* 196: flags of the mech */
    float armor;            /* 200 */
    float thrust;           /* 204 */
    int radar;              /* 208: 0 both, 1 horizontal, 2 vertical */
    float radar_zoom;       /* 212 */
    int nmeks;              /* 216 */
    pfVec2 vpos[MAX_MEKTONS];   /* 220: world y z */
    pfVec2 hpos[MAX_MEKTONS];   /* 348: colony x y */
    unsigned char team[MAX_MEKTONS];    /* 476 */
    char name[MAX_MEKTONS][16]; /* 492 */
    int score[MAX_MEKTONS];     /* 748 */
    pfVec2 spos[MAX_MEKTONS];   /* 812: on the screen, x -100000 if not seen */
    struct MsgList *msgs;   /* 940 */
    int fade;               /* 944 */
} ChanData;

typedef struct Plasma {
    int id;                 /* 0 */
    short s4;
    short turret;           /* 6: shot by a turret or another player */
    int state;              /* 8: 0 flying, 1 hit, 2 done */
    float damage;           /* 12 */
    pfVec3 pos;             /* 16 */
    pfVec3 hpr;             /* 28 */
    int owner;              /* 40 */
    int ttl;                /* 44 */
    pfVec3 vel;             /* 48 */
    pfVec3 old_pos;         /* 72 */
    unsigned int hit_mask;  /* 96 */
    pfVec3 hit_pt;          /* 100 */
    Obj *obj;               /* 112 */
} Plasma;

typedef struct Explosion {
    pfVec3 pos;             /* 0 */
    float scale;            /* 24 */
    short type;             /* 28 */
    short tied;             /* 30 */
    pfVec3 offset;          /* 36 */
    Obj *obj;               /* 52 */
    pfSequence *seq;        /* 56 */
    struct Mek *mek;        /* 60 */
    struct Explosion *prev, *next;
} Explosion;

/* a hit message: a shot of shooter met the mech target */
typedef struct HitMsg {
    int shooter;
    int target;
    short s12;
    short turret;
    float damage;
    pfVec3 pos, pt;
} HitMsg;

/* globals of the original executable */
extern Mek *mektons[MAX_MEKTONS];
extern Mek *my_mek;
extern int number_mektons;
extern pfMatrix mek_mat, old_mek_mat;
extern MekType mek_types[2];
extern Input mek_input_state;
extern float tps, tps_add;
extern int int_tps;
extern int ticks, check_ticks, total_frames;
extern ChanData *cd_app;
extern int user_mek_type, user_team, start_pos;
extern char user_name[16];
extern short shadow, comm, network, mekton_ui;
extern int use_stick;
extern float game_time;
extern float intro_start_time;
extern pfCoord camera;
extern int camera_mode, new_camera_mode;
extern pfGroup *isect_group, *isect_group_w_self;
extern pfScene *scene;
extern float fov;           /* sd->f184 */
extern float chan_fov;      /* sd->f444 */
extern int win_w, win_h;
extern int center_x, center_y;          /* sd->i620, i624 */
extern int chan_x, chan_y, chan_w, chan_h;
extern float time_limit;
extern int quit_game;

/* objects loaded by init_db */
extern Obj *eye, *vantage, *cg_vantage, *ex_centinel_trans, *ex_centinel_walk;
extern Obj *cg_ex_centinel, *pod, *cg_pod;
extern Obj *colony_int[9], *exterior_windows, *colony_windows, *colony_city;
extern Obj *colony_lights, *waterfall, *powerplant, *bolts, *obj_plasma;
extern Obj *obj_small_exp, *obj_big_exp, *obj_double_door, *obj_plasma_turret;

/* mek_math.c */
short xasin(float x);
void fsincos(float deg, float *s, float *c);
int frandom(int n);
float range(const pfVec3 a, const pfVec3 b);
float range_sq(const pfVec3 a, const pfVec3 b);
void vec_dist(const pfVec3 a, const pfVec3 b, float d, pfVec3 out);
void mat2hpr(pfMatrix m, pfVec3 hpr);
void orient(pfMatrix m, pfVec3 hpr, pfVec3 up);
void path_norm2hpr(const pfVec3 path, const pfVec3 norm, pfVec3 hpr);
void path2hp(const pfVec3 path, pfVec3 hp);
void hpr2path_norm(const pfVec3 hpr, pfVec3 path, pfVec3 norm);
void make_inv_euler_mat(const pfVec3 hpr, pfMatrix m);
void xyz_hpr2inv_mat(const pfVec3 xyz, const pfVec3 hpr, pfMatrix m);
void nearest_point_on_plane(const pfVec3 p, const pfVec3 n, const pfVec3 pt, pfVec3 out);
float dist_to_plane(const pfVec3 p, const pfVec3 n, const pfVec3 pt);
float sdist_to_plane(const pfVec3 p, const pfVec3 n, const pfVec3 pt);
int line_isect_plane(const pfVec3 p0, const pfVec3 p1, const pfVec3 n, const pfVec3 pt, pfVec3 out);
void reflect(const pfVec3 v, const pfVec3 n, pfVec3 out);
void identify_matrix(pfMatrix m);
void matrix_multiply(pfMatrix a, pfMatrix b, pfMatrix out);
void matrix_rotate(pfMatrix m, float deg, int axis);
void matrix_post_rotate(pfMatrix m, float deg, int axis);
void matrix_translate(pfMatrix m, float x, float y, float z);
void mult_vec(const pfVec3 v, pfMatrix m, pfVec3 out);
float vec_len(const pfVec3 v);

/* mek_db.c */
void init_db(void);
void update_db(void);
void adjust_origin(void);
Obj *make_obj(pfNode *model, int type, pfNode *parent);
Obj *load_obj(const char *file, int type, pfNode *parent);
Obj *clone_obj(Obj *o, pfNode *parent);
Obj *find_obj(int kind);
void free_obj(int kind, Obj *o);
void find_mek_objs(Mek *m);
void free_mek_objs(Mek *m);
void da_init(pfDCS *root);
void da_frame(pfDCS *root, float frame);
float da_get_frame(pfDCS *root);

/* mek_colony.c */
void update_colony(void);
void colony2world(const pfVec3 cpos, const pfVec3 chpr, const pfVec3 cvec,
                  pfVec3 pos, pfVec3 hpr, pfVec3 vec);
void world2colony(const pfVec3 pos, const pfVec3 hpr, const pfVec3 vec,
                  pfVec3 cpos, pfVec3 chpr, pfVec3 cvec);
void init_lighting(void);
void position_lights(float angle);
void light_adjust(void);
void init_background(void);
void draw_background(void);

/* mek_dynamics.c */
void init_mek(Mek *m, int type);
void set_mek_tps(Mek *m);
void mek_start_pos(Mek *m, int pos);
void mek_input(Mek *m);
void mek_dynamics(Mek *m);
void init_mek_fly(Mek *m);

/* mek_anim.c */
void animation_control(void);
void animate_mek(Mek *m);
void transform(void);

/* mek_camera.c */
void init_camera(void);
void init_camera_hist(void);
void reset_cameras(void);
void update_camera(Mek *m);
void cm_set(int mode);
void cc_zoom(void);
void cc_zoom_stop(void);
void cc_spin(void);
void cc_spin_stop(void);
void cc_height(void);
void cc_height_stop(void);

/* mek_main.c */
void mek_start(void);
void mek_restart(void);
void set_tps(float t);

/* mek_world.c */
void init_doors(void);
void update_doors(void);
void reset_doors(void);
void init_pods(void);
void start_pods(void);
void reset_pods(void);
void handle_pods(void);
void init_turrets(void);
void start_turrets(void);
void reset_turrets(void);
void handle_turrets(void);
void init_powerplant(void);

/* mek_weapons.c */
void init_projectiles(void);
void update_projectiles(void);
void reset_projectiles(void);
void init_explosions(void);
void update_explosions(void);
void update_plasma_objs(void);
void switch_dead(Mek *m);
void destroy_mek(Mek *m);
void fire_plasma(void);
Plasma *get_plasma(int mine);
void update_plasma_pos(Plasma *p, float dt, int stop);
Mek *find_mek_by_cid(unsigned int cid);
void hit_mek(Mek *m, const HitMsg *h);
Explosion *start_explosion(int type, const pfVec3 pos, float scale);
void tie_explosion(Explosion *e, int tied, Mek *m, const pfVec3 pos);
extern Plasma *plasma[];
extern int number_plasma, my_plasma, plasma_id;

/* mek_comm.c */
extern int my_id;
void send_plasma(Plasma *p);
void send_hit(Mek *m, Plasma *p);
void send_damage(int who, int what);

/* mek_hud.c */
void init_instruments(void);
void mek_post_draw(void);
void toggle_help(void);
void toggle_hud(void);
void toggle_radar(void);
void init_message_lists(void);
void add_message(const char *from, const char *text);
void hud_layout(int w, int h);
void pass_insrument_data(void);
extern char team_names[3][22];

/* mek_stubs.c */
void init_game_time(void);
void update_game_time(void);
void init_time(int n);
void check_time(void);

/* sounds: Mekton played them with the audio library, the port is silent */
#define left_foot_step_sound(m) ((void)(m))
#define right_foot_step_sound(m) ((void)(m))
#define both_feet_sound(m) ((void)(m))
#define clank_sound(p) ((void)(p))

#endif
