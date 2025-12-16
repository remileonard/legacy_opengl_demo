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

#define GRID_WIDTH 200
#define GRID_HEIGHT 150
#define BASE_ITERATIONS 128
#define MAX_ITERATIONS 1024

static clock_t last_frame_time = 0;
static int *heightMap = NULL;

// Paramètres de navigation
static double centerX = -0.5;
static double centerY = 0.0;
static double zoom = 1.0;

// Rotation avec la souris
static float rotationX = 30.0f;
static float rotationY = 45.0f;
static int lastMouseX = 0;
static int lastMouseY = 0;
static int mouseDown = 0;

void (*idlefunc)(void) = NULL;

static int getMaxIterations(void) {
    int iterations = (int)(BASE_ITERATIONS * (1.0 + log10(zoom) * 0.5));
    if (iterations < BASE_ITERATIONS) iterations = BASE_ITERATIONS;
    if (iterations > MAX_ITERATIONS) iterations = MAX_ITERATIONS;
    return iterations;
}

static int mandelbrot(double cx, double cy, int maxIterations) {
    double zx = 0.0, zy = 0.0;
    int iteration = 0;
    
    while (zx * zx + zy * zy < 4.0 && iteration < maxIterations) {
        double temp = zx * zx - zy * zy + cx;
        zy = 2.0 * zx * zy + cy;
        zx = temp;
        iteration++;
    }
    
    return iteration;
}

static void generateHeightMap(void) {
    int py, px;
    double aspectRatio = (double)GRID_WIDTH / (double)GRID_HEIGHT;
    int maxIterations = getMaxIterations();
    
    for ( py = 0; py < GRID_HEIGHT; py++) {
        for ( px = 0; px < GRID_WIDTH; px++) {
            double x = (px - GRID_WIDTH / 2.0) / (0.5 * zoom * GRID_WIDTH) * aspectRatio + centerX;
            double y = (py - GRID_HEIGHT / 2.0) / (0.5 * zoom * GRID_HEIGHT) + centerY;
            
            int iterations = mandelbrot(x, y, maxIterations);
            heightMap[py * GRID_WIDTH + px] = iterations;
        }
    }
}

static void getColor(int iterations, int maxIterations, float *r, float *g, float *b) {
    if (iterations == maxIterations) {
        *r = 0.0f; *g = 0.0f; *b = 0.0f;
    } else {
        double t = (double)iterations / maxIterations;
        *r = (float)(9 * (1 - t) * t * t * t);
        *g = (float)(15 * (1 - t) * (1 - t) * t * t);
        *b = (float)(8.5 * (1 - t) * (1 - t) * (1 - t) * t);
    }
}

static void drawVoxel(float x, float y, float z, float size, float r, float g, float b) {
    glColor3f(r, g, b);
    
    float hs = size / 2.0f;
    
    // Face avant
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x - hs, y - hs, z + hs);
    glVertex3f(x + hs, y - hs, z + hs);
    glVertex3f(x + hs, y + hs, z + hs);
    glVertex3f(x - hs, y + hs, z + hs);
    glEnd();
    
    // Face arrière
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(x - hs, y - hs, z - hs);
    glVertex3f(x - hs, y + hs, z - hs);
    glVertex3f(x + hs, y + hs, z - hs);
    glVertex3f(x + hs, y - hs, z - hs);
    glEnd();
    
    // Face supérieure
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(x - hs, y + hs, z - hs);
    glVertex3f(x - hs, y + hs, z + hs);
    glVertex3f(x + hs, y + hs, z + hs);
    glVertex3f(x + hs, y + hs, z - hs);
    glEnd();
    
    // Face inférieure
    glBegin(GL_QUADS);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(x - hs, y - hs, z - hs);
    glVertex3f(x + hs, y - hs, z - hs);
    glVertex3f(x + hs, y - hs, z + hs);
    glVertex3f(x - hs, y - hs, z + hs);
    glEnd();
    
    // Face droite
    glBegin(GL_QUADS);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x + hs, y - hs, z - hs);
    glVertex3f(x + hs, y + hs, z - hs);
    glVertex3f(x + hs, y + hs, z + hs);
    glVertex3f(x + hs, y - hs, z + hs);
    glEnd();
    
    // Face gauche
    glBegin(GL_QUADS);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(x - hs, y - hs, z - hs);
    glVertex3f(x - hs, y - hs, z + hs);
    glVertex3f(x - hs, y + hs, z + hs);
    glVertex3f(x - hs, y + hs, z - hs);
    glEnd();
}

static void idle(void) {
    clock_t current_time = clock();
    double elapsed_ms = (double)(current_time - last_frame_time) * 1000.0 / CLOCKS_PER_SEC;
    
    if (elapsed_ms >= FRAME_TIME_MS) {
        last_frame_time = current_time;
        glutPostRedisplay();
    }
}

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
    gluPerspective(60.0, (h > 0) ? (float)w / (float)h : 1.0f, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW);
}

static void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseDown = 1;
            lastMouseX = x;
            lastMouseY = y;
        } else {
            mouseDown = 0;
        }
    }
}

static void motion(int x, int y) {
    if (mouseDown) {
        float dx = (float)(x - lastMouseX);
        float dy = (float)(y - lastMouseY);
        
        rotationY += dx * 0.5f;
        rotationX += dy * 0.5f;
        
        // Limiter la rotation X pour éviter le retournement
        if (rotationX > 89.0f) rotationX = 89.0f;
        if (rotationX < -89.0f) rotationX = -89.0f;
        
        lastMouseX = x;
        lastMouseY = y;
        
        glutPostRedisplay();
    }
}

static void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    int needRedraw = 0;

    switch (key) {
    case 27: // ESC
        glutLeaveMainLoop();
        break;
    case '+': // Zoom in
        zoom *= 1.5;
        needRedraw = 1;
        break;
    case '-': // Zoom out
        zoom /= 1.5;
        needRedraw = 1;
        break;
    case 'r': // Reset
        centerX = -0.5;
        centerY = 0.0;
        zoom = 1.0;
        rotationX = 30.0f;
        rotationY = 45.0f;
        needRedraw = 1;
        break;
    default:
        break;
    }
    
    if (needRedraw) {
        printf("Regenerating Mandelbrot... (zoom: %.2f, iterations: %d)\n", 
               zoom, getMaxIterations());
        generateHeightMap();
        glutPostRedisplay();
    }
}

static void special(int key, int x, int y) {
    (void)x;
    (void)y;
    int needRedraw = 0;
    double moveSpeed = 0.1 / zoom;

    switch (key) {
    case GLUT_KEY_UP:
        centerY += moveSpeed;
        needRedraw = 1;
        break;
    case GLUT_KEY_DOWN:
        centerY -= moveSpeed;
        needRedraw = 1;
        break;
    case GLUT_KEY_LEFT:
        centerX -= moveSpeed;
        needRedraw = 1;
        break;
    case GLUT_KEY_RIGHT:
        centerX += moveSpeed;
        needRedraw = 1;
        break;
    default:
        break;
    }
    
    if (needRedraw) {
        generateHeightMap();
        glutPostRedisplay();
    }
}

static void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;
    (void)key;
}

static void display(void) {
    int py, px;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Caméra
    gluLookAt(0.0, 0.0, 100.0,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);
    
    // Appliquer les rotations
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
    
    // Dessiner les voxels
    float voxelSize = 0.8f;
    int maxIterations = getMaxIterations();
    
    for ( py = 0; py < GRID_HEIGHT; py++) {
        for ( px = 0; px < GRID_WIDTH; px++) {
            int iterations = heightMap[py * GRID_WIDTH + px];
            
            // Position centrée
            float x = (px - GRID_WIDTH / 2.0f) * voxelSize;
            float y = (py - GRID_HEIGHT / 2.0f) * voxelSize;
            
            // Hauteur basée sur les itérations
            float height = (float)iterations / maxIterations * 4.0f;
            float z = height / 2.0f;
            
            float r, g, b;
            getColor(iterations, maxIterations, &r, &g, &b);
            
            // Dessiner un cube avec la hauteur appropriée
            if (iterations < maxIterations) {
                glPushMatrix();
                glTranslatef(x, y, z);
                glScalef(voxelSize, voxelSize, height);
                
                // Cube unitaire
                glColor3f(r, g, b);
                //glutSolidCube(1.0);
                glutSolidDodecahedron();
                //glutSolidSphere(1.0, 6, 6);
                glPopMatrix();
            } else {
                glPushMatrix();
                z = 1.0f;
                glTranslatef(x, y, z);
                glScalef(voxelSize, voxelSize, 1.0f);
                
                // Cube unitaire
                glColor3f(r, g, b);
                glutSolidCube(1.0);
                glPopMatrix();
            }
        }
    }
    
    glutSwapBuffers();
}

static void initGL(void) {
    last_frame_time = clock();
    
    heightMap = (int *)malloc(GRID_WIDTH * GRID_HEIGHT * sizeof(int));
    if (!heightMap) {
        fprintf(stderr, "Erreur allocation mémoire\n");
        exit(1);
    }
    
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
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
    
    printf("Generating initial Mandelbrot 3D...\n");
    generateHeightMap();
    printf("Done!\n");
    printf("\nControles:\n");
    printf("  Souris (clic gauche + bouger): rotation\n");
    printf("  Fleches: deplacer\n");
    printf("  +/-: zoom\n");
    printf("  r: reset\n");
    printf("  ESC: quitter\n");
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Mandelbrot 3D - Voxels");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    
    free(heightMap);
    return 0;
}