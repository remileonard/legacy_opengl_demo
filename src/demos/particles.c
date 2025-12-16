#ifdef WIN32
    #include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define IDM_APPLICATION_EXIT (101)

#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)
#define MAX_PARTICLES 500
#define PARTICLE_LIFETIME 5.0f
#define M_PI 3.14159265358979323846
#define CAMERA_DISTANCE 30.0f
static clock_t last_frame_time = 0;

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    float r, g, b;
    float life;
    float max_life;
} Particle;

Particle particles[MAX_PARTICLES];
int particleCount = 0;

void (*idlefunc)(void) = NULL;

static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void display(void);
static void initGL(void);
static void particlesystem_update(void);
static void particlesystem_render(void);

int mx,my;
float cx=0, cy=0, cz=0;

static void menu_callback(int value) {
    switch (value) {
    case IDM_APPLICATION_EXIT:
        glutLeaveMainLoop();
        break;
    default:
        break;
    }
}
static void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (h > 0) ? (float)w / (float)h : 1.0f, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
}

static void idle(void) {
    // on se contente de redessiner en continu
    clock_t current_time = clock();
    double elapsed_ms = (double)(current_time - last_frame_time) * 1000.0 / CLOCKS_PER_SEC;
    
    if (elapsed_ms >= FRAME_TIME_MS) {
        last_frame_time = current_time;
        particlesystem_update();
        glutPostRedisplay();
    }
}

static void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    case 27: // ESC
        glutLeaveMainLoop();
        break;
    default:
        break;
    }
}

static void special(int key, int x, int y) {
    (void)x;
    (void)y;

    int modifiers = glutGetModifiers();
    int altDown = modifiers & GLUT_ACTIVE_ALT;

    switch (key) {
    default:
        break;
    }
}

static void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    default:
        break;
    }
}
static void particlesystem_renderer(void) {
    glClearColor(0.0, 0.0, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /**/
    mx+=1;
    cx = CAMERA_DISTANCE * sinf((float)mx * 0.01f);
    cz = CAMERA_DISTANCE * cosf((float)mx * 0.01f);
    cy = CAMERA_DISTANCE * sinf((float)mx * 0.005f);
    glLoadIdentity();
    GLfloat lightPos[] = { 50.0f, 50.0f, 100.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    gluLookAt(cx, cy, cz,
              0.0f, 0.0f, 0.0f,
              0.0f, 1.0f, 0.0f);
    
    

    glColor3f(0.1f, 0.1f, 0.7f);
    glBegin(GL_QUADS);
    glVertex3f(-10.0f, 0.0f, -10.0f);
    glVertex3f( 10.0f, 0.0f, -10.0f);
    glVertex3f( 10.0f, 0.0f,  10.0f);
    glVertex3f(-10.0f, 0.0f,  10.0f);
    glEnd();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for (int i = 0; i < particleCount; i++) {
        Particle* p = &particles[i];
        glPushMatrix();
        glTranslatef(p->x, p->y, p->z);
        glColor4f(p->r, p->g, p->b, p->life / p->max_life);
        glutSolidSphere(0.2f, 8, 8);
        
        glPopMatrix();
    }
    

    glutSwapBuffers();
}
void particlesystem_explode(float x, float y, float z) {
    float speed = 0.1f;
    int numParticles = 12*6;

    for (int i = 0; i < numParticles && particleCount < MAX_PARTICLES; i++) {
        float theta = ((float)(rand() % 100) / 100.0f) * 2.0f * M_PI;
        float phi = acosf(1.0f - 2.0f * ((float)(rand() % 100) / 100.0f));
        Particle* p = &particles[particleCount++];
        p->x = x;
        p->y = y;
        p->z = z;
        p->vx = speed * sinf(phi) * cosf(theta);
        p->vy = speed * sinf(phi) * sinf(theta);
        p->vz = speed * cosf(phi);
        float intensity = (float)(rand() % 100) / 100.0f;
        p->r = 1.0f;
        p->g = intensity;
        p->b = 0.0f;
        p->life = PARTICLE_LIFETIME*0.1f;
        p->max_life = p->life;
    }

}
void particlesystem_update(void) {

    for (int i = 0; i < particleCount; ) {
        Particle* p = &particles[i];
        p->x += p->vx;
        p->y += p->vy;
        p->z += p->vz;
        p->vy -= 0.002f; // Gravité simple
        if (p->y < 0.0f && p->x > -10.0f && p->x < 10.0f && p->z > -10.0f && p->z < 10.0f) {
            p->y = 0.0f;
            p->vy *= -0.8f; // Rebond
            p->life *= 0.5f; // Perte de vie au rebond
        }
        if (p->vy < 0.00001f && p->y >= 6.0f) {
            float explode_chance = 0.000000000000001f;
            float chek = (float)(rand() % 10000000000000000) / 10000000000000000.0f;
            if (chek < explode_chance) {
                particlesystem_explode(p->x, p->y, p->z);
                particles[i] = particles[particleCount - 1];
                particleCount--;
                continue;
            }
        }
        p->life -= 0.01f;
        if (p->life <= 0 || p->y < -20.0f) {
            particles[i] = particles[particleCount - 1];
            particleCount--;
        } else {
            i++;
        }
    }

    if (particleCount < MAX_PARTICLES) {
        Particle* p = &particles[particleCount++];
        p->x = 0.0f;
        p->y = 0.0f;
        p->z = 0.0f;
        p->vx = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
        p->vy = ((float)(rand() % 100) / 100.0f) * 0.3f;
        p->vz = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
        float intensity = (float)(rand() % 100) / 100.0f;
        p->r = intensity;
        p->g = 0.0f;
        p->b = 1.0f;
        p->life = PARTICLE_LIFETIME;
        p->max_life = p->life;
    }
}
static void display(void) {
    if (idlefunc) {
        idlefunc();
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glutSwapBuffers();
    }
}
static void initGL(void) {
    last_frame_time = clock();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    // Lumière
    GLfloat lightPos[] = { 50.0f, 50.0f, 100.0f, 1.0f };
    GLfloat lightAmb[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat lightDiff[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    idlefunc = NULL;
}
void mouseMotion(int x, int y) {
    mx = x;
    my = y;
}
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    // Double buffer + RGB + z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Particles");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    initGL();
    idlefunc = particlesystem_renderer;
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMotionFunc(mouseMotion);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);

    glutMainLoop();
    return 0;
}