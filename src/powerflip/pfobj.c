/*
 * pfobj.c - the objects of powerflip: YAODL models
 *
 * The flip application (powerflip.c) reads, measures and draws its objects
 * with these functions; the models are read and executed by yaodl.c.
 */
#include <stdio.h>
#include <stdlib.h>

#include "powerflip.h"
#include "pfopts.h"

flipobj *readflipobj(char *name) {
    flipobj *obj;
    YModel *m = yaodl_read(name);

    if (!m)
        return NULL;
    obj = (flipobj *)calloc(1, sizeof(flipobj));
    obj->model = m;
    /* flip counts its polygons as 4 points each */
    obj->npoints = 4 * m->npolys;
    return obj;
}

/*
 * The polygons are filled, or drawn as closed lines in the line modes;
 * the colors of the model replace the diffuse color of the material.
 */
void drawflipobj(flipobj *obj) {
    YAODL_DrawStyle = pf_draw_style(obj->type);
    if (pf_nolighting) {
        /* -L: lines model, no lighting */
        lmbind(LMODEL, 0);
        cpack(0xffffffff);
        yaodl_draw(obj->model);
        resetmodel();
        return;
    }
    work_around_lmcolor_feature(LMC_DIFFUSE);
    yaodl_draw(obj->model);
    lmcolor(LMC_COLOR);
}

/*
 * The vertex farthest from the origin, so we can set the near and far
 * clipping planes tightly.
 */
float objmaxpoint(flipobj *obj) {
    return obj->model->radius;
}
