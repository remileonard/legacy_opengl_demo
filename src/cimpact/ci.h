/*
 * ci.h - Certain Impact (Paradigm Simulation, 1996), shared declarations of
 * the port. The game classes of the original (mardi, vgAirPlane,
 * vgEnemyAirPlane, vgBattleRegion, vgTrain, ...) are C structures here.
 */
#ifndef CI_H
#define CI_H

#include "vg.h"

/* --- isect classes ---------------------------------------------------------- */

#define CL_GROUND 0x1
#define CL_WATER 0x2
#define CL_TREES 0x4
#define CL_BUNKER 0x8
#define CL_BRIDGE 0x10
#define CL_HANGAR 0x100
#define CL_HOUSE 0x200
#define CL_CASTLE 0x1000
#define CL_CRATES 0x2000
#define CL_ARTIL 0x4000
#define CL_USER 0x10000
#define CL_AMMO_BALLOON 0x40000
#define CL_FUEL_BALLOON 0x80000
#define CL_HEALTH_BALLOON 0x100000
#define CL_ALLIED_PLANE 0x200000
#define CL_GERMAN_PLANE 0x400000
#define CL_BLIMP 0x800000
#define CL_ENGINE 0x1000000
#define CL_COALCAR 0x2000000
#define CL_CAR 0x4000000
#define CL_ARTILCAR 0x8000000
#define CL_CABOOSE 0x10000000
#define CL_ALLIED_SHIP 0x20000000
#define CL_AXIS_SHIP 0x40000000

/* --- airplane types (mardi + 172, 336 bytes each) ------------------------------ */

typedef struct PlaneType {
    float vmax;              /* m/s */
    float range;             /* miles */
    float invrange;
    float agility;
    int ammo, bombs;
    float tether[3];         /* follow view offset */
    char name[32];
} PlaneType;

/* --- user airplane (vgAirPlane) ------------------------------------------------- */

typedef struct Plane {
    float init[6];
    vgObject *obj, *shadow;
    int shadow_on;
    pfNode *lwing, *rwing, *tailv, *tailh;
    float eye[3];
    PlaneType t;
    int dead;                /* i100 */
    int code, side;          /* 0 German, 1 Allied */
    float gnd_pitch, gear;
    int state;               /* 0 ground, 1 flying, 2 stalled, 3 water, 4 crashed, 5 dive */
    float fuel, damagef;
    int damage, ammo, bombs;
    float speedfrac, prev_speedfrac, fwdfrac, speed;
    pfVec3 dir, vel;
    float pos[6];            /* x y z h p r */
    float stall_pitch, stall_spin;
    float throttle, thrust_eff, stick_y, yaw, stick_x;
    pfMatrix mat;
    /* motion parameters */
    float vmax, vstall, lift, landh, invvmax, gturn, roll_rate, pitch_rate;
    float thr_a, thr_b, stall_p, stall_s;
    int score;
    float ground;            /* f532: ground + gear height */
    float dive_t, dive_z;
    int screamed, engine_on;
    double start_time;
} Plane;

/* --- ballistic ammo -------------------------------------------------------------- */

typedef struct Ammo Ammo;
typedef int (*AmmoHitCb)(Ammo *a, vgHit *h, void *data);

struct Ammo {
    int active;
    vgObject *obj;           /* bombs and flak shells */
    int tracer;
    float tracer_color[3];
    pfVec3 start, pos, hpr, vel;
    float seglen, range2, accel;
    unsigned int mask;       /* 0: no intersection */
    AmmoHitCb cb;
    void *data;
};

typedef struct AmmoBank {
    int n;
    Ammo *ammo;
    float range, accel;
    unsigned int mask;
    int tracers;
    float tracer_color[3];
    vgObject *proto;
} AmmoBank;

/* --- globals of the game (mardi) ----------------------------------------------- */

extern vgScene *gMainScene, *gCockpitScene, *gSelScene, *gSelCockpitScene;
extern Plane *gUserPlane;
extern PlaneType gTypes[6];
extern int gDifficulty;      /* i160: 0 invincible, 1 cadet, 2 ace */
extern int gPlaneType;       /* i152 */
extern double gNow;          /* f2584 */
extern float gDt;            /* f128 */
extern float gDt90;          /* f124 */
extern float gRand;          /* f2612 */
extern int gLayout;          /* i2284 */
extern int gTrainFront;      /* i2292 */
extern int gShowMap;
extern AmmoBank *gGunBank, *gBombBank, *gEnemyBank, *gFlakBank;

/* ci_main.c */
void ciChangeLayout(int layout);
void ciTetherTrainBack(void);
void ciResetMotionModels(void);
void ciCreateFlakExplosion(const float *p);
void ciCreateAirExplosion(const float *p);
void ciCreateSmallGroundExplosion(const float *p);
void ciCreateBigGroundExplosion(const float *p);
void ciSwitchToDamaged(pfNode *n);
pfNode *ciGetSwitchNode(vgHit *h);
double ciRand(void);
void ciCopyMatHPR(pfMatrix m, float *hpr);

/* ci_plane.c */
Plane *planeNew(int code, const float *init);
void planeDelete(Plane *p);
void planeReset(Plane *p);
void planeResetMotion(Plane *p);
void planeUpdate(Plane *p, float sx, float sy, int up, int down);
void planeAddScore(Plane *p, int n);
void planeHit(Plane *p, int n, int bullet);
void planeExplode(Plane *p);
void planeSink(Plane *p);
void planeDiveAndExplode(Plane *p);

AmmoBank *ammoBankNew(int n, float range, float accel, unsigned int mask, int tracers,
                      float r, float g, float b, vgObject *proto);
Ammo *ammoFire(AmmoBank *b, const pfVec3 pos, const pfVec3 vel, AmmoHitCb cb, void *data);
int ammoUpdate(Ammo *a, float dt);
int ammoUpdateNoIsect(Ammo *a, float dt, float maxdist2);
void ammoKill(Ammo *a);
void ammoDraw(AmmoBank *b);

void gunsNew(Plane *p);
void gunsDelete(void);
void gunsTrigger(void);
void gunsUpdate(double t, float dt);
void bombsTrigger(void);
void bombsUpdate(double t, float dt);

/* ci_world.c */
void worldConfigure(void);   /* terrain and the permanent objects */
void worldCreate(Plane *user, int nenemies);
void worldDestroy(void);
void worldUpdate(double t, float dt);
void worldReset(void);
void worldResetMotion(void);
void worldDetonate(int kind, const float *pos, vgHit *h);
void worldCollision(vgObject *o, unsigned int cls);
void worldMapPositions(float *train, int *nplanes, float planes[][4], int *nfriends, float friends[][4]);
void worldTerrainAnimations(void);
void worldTerrainInitDoors(void);
void worldTerrainUndamage(void);

#endif
