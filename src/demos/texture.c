/* 
   texture.c
   Nate Robins, 1997

   A simple program to show how to do texture mapping.

*/


#include <stdio.h>
#include <stdlib.h>
#if defined (__APPLE__) || defined (MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


GLubyte texture[] = {
    0x80, 0x80, 0x80, 0xff, 0xff, 0xff, 0x80, 0x80, 0x80, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x80, 0x80, 0x80, 0xff, 0xff, 0xff, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0xff, 0xff, 0xff, 0x80, 0x80, 0x80, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x80, 0x80, 0x80, 0xff, 0xff, 0xff, 0x80, 0x80, 0x80,
};

void
reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1.0, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 4, 4, 0, 
        GL_RGB, GL_UNSIGNED_BYTE, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void
display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3ub(255, 255, 255);
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0, 0.0, 1.0);
    glColor3ub(255, 0, 0);
    glTexCoord2f(0.5, 1.0);
    glVertex2f(0.0, 2.0);
    glColor3ub(0, 255, 0);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-2.0, -2.0);
    glColor3ub(0, 0, 255);
    glTexCoord2f(1.0, 0.0);
    glVertex2f(2.0, -2.0);
    glEnd();

    glFlush();
}

void
keyboard(unsigned char key, int x, int y)
{
    if (key == 27) {
        exit(0);
    }
}

int
main(int argc, char** argv)
{
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowPosition(50, 50);
    glutInitWindowSize(320, 320);
    glutInit(&argc, argv);

    glutCreateWindow("texture");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
