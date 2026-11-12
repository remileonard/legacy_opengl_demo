/*
 * mek_stubs.c - the game clock, and local stand-ins for the network
 * messages of Mekton, which is not ported yet.
 */

#include <stdio.h>
#include <string.h>

#include "mekton.h"

static double game_start;

void init_game_time(void)
{
    game_start = pfGetTime();
}

/* the time played, or left to play with a time limit */
void update_game_time(void)
{
    float t = (float)(pfGetTime() - game_start);
    if (0 < time_limit) {
        game_time = time_limit - t;
        if (game_time < 0) {
            game_time = 0;
            cd_app->state = 9;
        }
    } else {
        game_time = t;
    }
    cd_app->time = (int)(game_time + 0.5f);
}

void enter_key(void)
{
    if (cd_app->state != STATE_SETUP)
        mek_restart();
}

void give_up(void)
{
    if (!shadow && cd_app->state == STATE_PLAY && my_mek->status != 2)
        destroy_mek(my_mek);
}

int my_id;
void send_plasma(Plasma *p) { (void)p; }
void send_hit(Mek *m, Plasma *p)
{
    HitMsg h;
    if (m != my_mek)
        return;
    memset(&h, 0, sizeof(h));
    h.shooter = my_id;
    h.target = m->net_id;
    h.turret = p->turret;
    h.damage = p->damage;
    hit_mek(m, &h);
}
void send_damage(int who, int what) { (void)who; (void)what; }
