/* 
   rotate.c
   Nate Robins, 1997

   An example of rotating a bitmap (w/o OpenGL's help).

*/


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined (__APPLE__) || defined (MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


#define deg2rad(x)  ((x) * 3.14159265 / 180.0)
#undef min
#undef max
#define min(a, b) (((b) < (a)) ? (b) : (a))
#define max(a, b) (((b) > (a)) ? (b) : (a))


int isize;
int width, height;
unsigned char* image;


void
reshape(int w, int h)
{
    int i, j;

    width = w; height = h;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    isize = width > height ? width/2 : height/2;
    if (image) free(image);
    image = (unsigned char*)malloc(sizeof(unsigned char)*isize*isize*3);
	
    /* create a pretty color ramp */
    for (j = 0; j < isize; j++) {
        for (i = 0; i < isize; i++) {
            image[(isize * j + i) * 3 + 0] = 255-(i*j/255*255/isize);
            image[(isize * j + i) * 3 + 1] = i*256/isize;
            image[(isize * j + i) * 3 + 2] = j*256/isize;
        }
    }
}

void
display(void)
{
    int x, y;
    int i, j;
    float min_x, min_y, max_x, max_y;
    float isize2, tmp0, tmp1, tmp2, tmp3;
    float sin_theta, cos_theta;
    static float theta = 0.0;

    glClear(GL_COLOR_BUFFER_BIT);

    isize2 = (float)isize/2;

    sin_theta = sin(deg2rad(theta));
    cos_theta = cos(deg2rad(theta));

    /* rotate the corners of the image to get the bounding box of the
       rotated image */
    
    tmp0 = -isize2 * cos_theta - -isize2 * sin_theta;
    tmp1 =  isize2 * cos_theta - -isize2 * sin_theta;
    tmp2 =  isize2 * cos_theta - isize2 * sin_theta;
    tmp3 = -isize2 * cos_theta - isize2 * sin_theta;
    min_x = min(tmp0, min(tmp1, min(tmp2, tmp3)));
    max_x = max(tmp0, max(tmp1, max(tmp2, tmp3)));

    tmp0 = -isize2 * sin_theta + -isize2 * cos_theta;
    tmp1 =  isize2 * sin_theta + -isize2 * cos_theta;
    tmp2 =  isize2 * sin_theta +  isize2 * cos_theta;
    tmp3 = -isize2 * sin_theta +  isize2 * cos_theta;
    min_y = min(tmp0, min(tmp1, min(tmp2, tmp3)));
    max_y = max(tmp0, max(tmp1, max(tmp2, tmp3)));

    glBegin(GL_POINTS);
    for (j = min_y; j < max_y; j++) {
        for (i = min_x; i < max_x; i++) {

            /* calculate the sample point */
            x = (i * cos_theta - j * sin_theta) + isize2;
            y = (i * sin_theta + j * cos_theta) + isize2;

            /* skip out if we're going to sample outside the bitmap */
            if (x < 0 || x >= isize || y < 0 || y >= isize)
                continue;

            glColor3ubv(&image[(isize * y + x) * 3]);
            glVertex2i(width/2+i, width/2+j);
        }
    }
    glEnd();

    theta += 6.0;
    if (theta > 360.0) theta -= 360.0;

    glutSwapBuffers();
}

void
keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
        exit(0);
}

void
idle(void)
{
    glutPostRedisplay();
}

int
main(int argc, char** argv)
{
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(50, 50);
    glutInitWindowSize(256, 256);
    glutInit(&argc, argv);

    glutCreateWindow("rotate");
    glutKeyboardFunc(keyboard);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
