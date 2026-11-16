/*
 * mek_world.c - the things in the colony: the door of the city, the
 * power plant, and the sentry pods and turrets.
 *
 * In this version of Mekton the pods and the turrets are never placed
 * (start_pods and start_turrets are empty in the original), so the
 * fights are between players only.
 */
#include <stdlib.h>

#include "mekton.h"

typedef struct Door {
    short state;            /* 0 closed, 1 opening, 2 waiting, 3 open, 4 closing */
    short kind;
    pfVec3 pos;             /* 4 */
    pfVec3 hpr;             /* 16 */
    float scale;            /* 28 */
    float radius_sq;        /* 32 */
    Mek *opener;            /* 36 */
    float frame;            /* 40: 0 closed, 1 open */
    float open_time;        /* 44: seconds */
    float wait;             /* 48 */
    float wait_time;        /* 52 */
    Obj *obj;               /* 64 */
} Door;

#define NDOORS 1
static Door doors[NDOORS];

static void init_door(Door *d, int kind, float scale, float open_time, float radius)
{
    static int first = 1;
    d->kind = (short)kind;
    d->radius_sq = radius * radius;
    d->scale = scale;
    d->open_time = open_time;
    if (first) {
        d->obj = obj_double_door;
        first = 0;
    } else {
        d->obj = clone_obj(obj_double_door, scene);
    }
    da_init(d->obj->dcs);
    pfDCSScale(d->obj->dcs, scale);
    pfAddChild(isect_group, d->obj->dcs);
    pfCopyVec3(d->obj->trans, d->pos);
    pfDCSTrans(d->obj->dcs, d->pos[0], d->pos[1], d->pos[2]);
    pfCopyVec3(d->obj->rot, d->hpr);
    pfDCSRot(d->obj->dcs, d->hpr[0], d->hpr[1], d->hpr[2]);
}

void init_doors(void)
{
    pfSetVec3(doors[0].pos, -258.339294f, -812.952393f, -4823.16064f);
    pfSetVec3(doors[0].hpr, 329.351501f, -11.2317181f, 353.415222f);
    init_door(&doors[0], 0, 1.41421f, 2.0f, 60.0f);
}

static void open_door(Door *d, Mek *m)
{
    d->opener = m;
    d->state = 1;
}

static void close_door(Door *d)
{
    d->opener = NULL;
    d->state = 4;
}

static void position_door(Door *d)
{
    da_frame(d->obj->dcs, d->frame);
}

/* the doors open when a mech comes near */
void update_doors(void)
{
    int i;
    for (i = 0; i < NDOORS; i++) {
        Door *d = &doors[i];
        switch (d->state) {
        case 0:
            if (range_sq(my_mek->pos, d->pos) < d->radius_sq && !shadow)
                open_door(d, my_mek);
            break;
        case 1:
            d->frame += 1.0f / (d->open_time * tps);
            if (1 <= d->frame) {
                d->frame = 1;
                d->state = 0 < d->wait_time ? 2 : 3;
            }
            position_door(d);
            break;
        case 2:
            d->wait += 1.0f / (d->wait_time * tps);
            if (1 <= d->wait) {
                d->wait = 0;
                d->state = 3;
            }
            break;
        case 3:
            if (d->radius_sq < range_sq(d->opener->pos, d->pos) || d->opener->s124 <= 0)
                close_door(d);
            break;
        case 4:
            if (range_sq(my_mek->pos, d->pos) < d->radius_sq && !shadow) {
                open_door(d, my_mek);
                break;
            }
            d->frame -= 1.0f / (d->open_time * tps);
            if (d->frame <= 0) {
                d->frame = 0;
                d->state = 0;
            }
            position_door(d);
            break;
        }
    }
}

void reset_doors(void) {}

int powerplant_destroyed;

void init_powerplant(void)
{
    pfSwitchVal(bolts->sw, PFSWITCH_ON);
    powerplant_destroyed = 0;
}

void init_pods(void) {}
void start_pods(void) {}
void reset_pods(void) {}
void handle_pods(void) {}
void init_turrets(void) { pfSwitchVal(obj_plasma_turret->sw, PFSWITCH_OFF); }
void start_turrets(void) {}
void reset_turrets(void) {}
void handle_turrets(void) {}
