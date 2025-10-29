/*
 * setup init positions and speeds etc of the cycles
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../porting/iris2ogl.h"
#include "cycles.h"

extern CYCLE *good, bike[CYCLES];
extern int used[CYCLES];
extern float vec[4][2];

static int num_names = 8;
static char *namelist[] = { "walter", "alan san", "sarah", "brett",
                            "claire", "nick", "ernie", "james" };

void init_pos(CYCLE *C) {
    int i;

    /*
     * if you want to start in the same spot every time
     * put the code here in place of the next 3 lines
     */
    C->origin.x = 1.8*DIM*(float)rand()/(float)RAND_MAX - 0.9*DIM;
    C->origin.y = HEIGHT;
    C->origin.z = 1.8*DIM*(float)rand()/(float)RAND_MAX - 0.9*DIM;

    C->step = 0.8*MIN_STEP + 0.2*MAX_STEP;
    C->jump = 0;
    C->jump_speed = 0.0;
    C->fall = 0.0;
    C->falling = 0;
    C->vec_ptr = rand()%4;
    C->direction.x = vec[C->vec_ptr][0];
    C->direction.y = 0.0;
    C->direction.z = vec[C->vec_ptr][1];
    C->alive = 1;
    C->who_we_hit = -1;
    C->quit = 0;
    C->level = rand()%LEVELS;
    C->type = ROBOT;
    C->lturn = 0;
    C->rturn = 0;
    C->behave = rand()%2;
    C->trail_colour = rand()%6;
    sprintf(C->name, "%s", namelist[rand()%num_names]);
    C->trail_ptr = 0;

#if 0
    C->origin.x = 0.1*DIM*(float)rand()/(float)RAND_MAX - 0.9*DIM;
    C->origin.y = HEIGHT;
    C->origin.z = 0.1*DIM*(float)rand()/(float)RAND_MAX - 0.9*DIM;
    C->step = MIN_STEP;
    C->level = 0;
#endif

    for (i = 0; i < TRAIL_LENGTH; i++) {
	C->trail[i].y = 0.0;
	C->trail[i].x = C->origin.x;
	C->trail[i].z = C->origin.z;
	C->trail[i].level = -1;
    }
    C->trail[0].level = C->level;
    bcopy((void *)&C->trail[0], (void *)&C->trail_update, (int)sizeof(CPOINT));
    C->view = vadd(C->origin, C->direction);
}
