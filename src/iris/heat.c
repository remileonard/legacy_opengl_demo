#include "porting/iris2ogl.h"
#include <stdio.h>
/* Texture environmnet */
float tevprops[] = {TV_MODULATE, TV_NULL};
/* RGBA texture map representing temperature as color and
 * opacity */
float texheat[] = {TX_WRAP, TX_CLAMP, TX_NULL};
/* Black->blue->cyan->green->yellow->red->white */
unsigned long heat[] = /* Translucent -> Opaque */
    {0x00000000, 0x55ff0000, 0x77ffff00, 0x9900ff00, 0xbb00ffff, 0xdd0000ff, 0xffffffff};
/* Point sampled 1 component checkerboard texture */
float texbgd[] = {TX_MAGFILTER, TX_POINT, TX_NULL};
uint32_t check[] =  {
        0x550000ff, 0x00ff5500, 0xff000055, 0x0055ff00,
};

    /*{0xff800000, 0x80ff0000};*/

/* Subdivision parameters */
float scrparams[] = {0., 0., 10};
/* Define texture and vertex coordinates */
float t0[] = {0., 0.}, v0[] = {-2., -4., 0.};
float t1[] = {.4, 0.}, v1[] = {2., -4., 0.};
float t2[] = {1., 0.}, v2[] = {2., 4., 0.};
float t3[] = {.7, 0.}, v3[] = {-2., 4., 0.};

main() {
    long device;
    short data, sub = 0;
    if (getgdesc(GD_TEXTURE) == 0) {
        fprintf(stderr, "Texture mapping not available on this machine\n");
        return 1;
    }
    keepaspect(1, 1);
    winopen("heat");
    RGBmode();
    doublebuffer();
    gconfig();
    subpixel(TRUE);
    lsetdepth(0x0, 0x7fffff);
    blendfunction(BF_SA, BF_MSA); /* Enable blending */
    mmode(MVIEWING);
    perspective(600, 1, 1., 16.);
    /* Define checkerboard */
    texdef2d(1, 1, 4, 4, check, 0, texbgd);
    /* Define heat */
    texdef2d(2, 4, 7, 1, heat, 0, texheat);
    tevdef(1, 0, tevprops);
    tevbind(TV_ENV0, 1);
    translate(0., 0., -6.);
    qdevice(ESCKEY);
    /* Determine if machine does perspective correction */
    if (getgdesc(GD_TEXTURE_PERSP) != 1)
        sub = 1;
        while (TRUE) {
        if (qtest()) {
            device = qread(&data);
            switch (device) {
            case ESCKEY:
                texbind(TX_TEXTURE_0, 0); /* Turn off texturing */
                exit(0);
                break;
            case REDRAW:
                reshapeviewport();
                break;
            }
        }
        cpack(0xff550022);
        clear();
        zclear();
        texbind(TX_TEXTURE_0, 1); /* Bind checkerboard */
        cpack(0xff0000ff);        /* Background rectangle color */
        bgnpolygon();             /* Draw textured rectangle */
        t2f(v0);
        v3f(v0); /* Notice vertex */
        t2f(v1);
        v3f(v1); /* coordinates are used */
        t2f(v2);
        v3f(v2); /* as texture coordinates */
        t2f(v3);
        v3f(v3);
        endpolygon();
        pushmatrix();
        rotate(getvaluator(MOUSEX)*5, 'y');
        rotate(getvaluator(MOUSEY)*5, 'x');
        
        texbind(TX_TEXTURE_0, 2); /* Bind heat */
        cpack(0xffffffff);        /* Heated rectangle base color */
        bgnpolygon();             /* Draw textured rectangle */
        t2f(t0);
        v3f(v0);
        t2f(t1);
        v3f(v1);
        t2f(t2);
        v3f(v2);
        t2f(t3);
        v3f(v3);
        endpolygon();
        popmatrix();
        swapbuffers();
    }
}