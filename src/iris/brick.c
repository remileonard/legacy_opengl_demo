#include "porting/iris2ogl.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

float texprops[] = {TX_MINFILTER, TX_POINT,TX_MAGFILTER, TX_POINT,
TX_WRAP, TX_REPEAT, TX_NULL};
/* Texture color is brick-red */
float tevprops[] = {TV_COLOR, .75, .13, .06, 1., TV_BLEND, TV_NULL};

/* Subdivision parameters */
float scrparams[] = {0., 0., 10.};
unsigned long bricks[] = /*Define texture image */
    {
        0x00ffffff, 0xffffffff,
        0x00ffffff, 0xffffffff,
        0x00ffffff, 0xffffffff,
        0x00000000, 0x00000000,
        0xffffffff, 0x00ffffff,
        0xffffffff, 0x00ffffff,
        0xffffffff, 0x00ffffff,
        0x00000000, 0x00000000
    };
/* Define texture and vertex coordinates */
float t0[2] = {0., 0.}, v0[3] = {-2., -4.,0.};
float t1[2] = {16., 0.}, v1[3] = {2., -4.,0.};
float t2[2] = {16., 32.}, v2[3] = {2., 4.,0.};
float t3[2] = {0., 32.}, v3[3] = {-2., 4.,0.};

unsigned char hills[128 * 128]; /* Texture image buffer */
void readtex(char *fname, unsigned long *buf, int size)
{
    long ifd;

    if ((ifd = open(fname, O_RDONLY)) == -1) {
        fprintf(stderr, "flight: can't open texture file %s\n", fname);
        exit(1);
    }

    int readt = read(ifd, buf, size);
    if (readt != size) {
        fprintf(stderr, "flight: error reading texture file %s\n", fname);
        close(ifd);
        exit(1);
    }
    close(ifd);
}
void main() {
    short val;
    int dev, texflag;
    if (getgdesc(GD_TEXTURE) == 0) {
        fprintf(stderr, "texture mapping not availble on this machine\n");
        return ;
    }
    keepaspect(1, 1);
    winopen("brick");
    subpixel(TRUE);
    RGBmode();
    doublebuffer();
    gconfig();
    qdevice(ESCKEY);
    qdevice(REDRAW);
    qdevice(LEFTMOUSE);
    qenter(LEFTMOUSE, 0);
    mmode(MVIEWING);
    perspective(600, 1.0, 0.1, 1000.0);
    //texdef2d(1, 1, 128, 128, hills, 0, texprops);
    texdef2d(1, 1, 8, 8, bricks, 0, texprops);
    tevdef(1, 0, tevprops);
    texbind(TX_TEXTURE_0, 1);
    tevbind(TV_ENV0, 1);
    texflag = getgdesc(GD_TEXTURE_PERSP);
    translate(0., 0., -6.); /* Move poly away from viewer */
    while (!(qtest() && qread(&val) == ESCKEY && val == 0)) {
        while (TRUE) {
            while (qtest()) {
                dev = qread(&val);
                switch (dev) {
                case ESCKEY:
                    exit(0);
                    break;
                case REDRAW:
                    reshapeviewport();
                    break;
                } 
                /* end main switch */
            } /* end qtest */
            rotate(5,'y');
            rotate(6,'z');

            cpack(0xffcc0000);
            clear();
            zclear();
            pushmatrix();
            cpack(0xff0000cc);
            bgnpolygon();
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
        texbind(TX_TEXTURE_0, 0); /* Turn off texturing */
    }
}