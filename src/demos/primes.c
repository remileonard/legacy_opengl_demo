#ifdef WIN32
    #include <windows.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define IDM_APPLICATION_EXIT (101)
#define M_PI 3.14159265358979323846
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)

#define PRIME_LIMIT 10000
#define PRIME_VOXEL_SIZE 8.0f
#define PRIME_SPIRAL_STEP 1000.0f

#define PRIME_ROTATION 90.0f
static clock_t last_frame_time = 0;
// Structure pour stocker les positions des cubes
typedef struct {
    float x, y, z;
    float r, g, b;
    float sx, sy, sz;
} CubePosition;

static CubePosition* placedCubes = NULL;
static int cubeCount = 0;
static int cubeCapacity = 0;
static int initialized = 0;

static float initProgress = 0.0f;
static int isInitializing = 0;

void (*idlefunc)(void) = NULL;

static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void display(void);
static void initGL(void);

float eyesX = 0.0f;
float eyesY = 5.0f;
float eyesZ = 15.0f;
float lookX = 0.0f;
float lookY = 0.0f;
float lookZ = 0.0f;
    
static int lastMouseX = 400;
static int lastMouseY = 300;
static int firstMouse = 1;
static float cameraYaw = 0.0f;   // Rotation horizontale
static float cameraPitch = -45.0f; // Rotation verticale
static float cameraDistance = 30.0f; // Distance de la caméra par rapport au point regardé
static float mouseSensitivity = 0.5f;

static void primes(void);

static int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

static int is_position_valid(float x, float y, float z, float minDist) {
    float minDistSq = minDist * minDist;
    for (int i = 0; i < cubeCount; i++) {
        float dx = x - placedCubes[i].x;
        float dy = y - placedCubes[i].y;
        float dz = z - placedCubes[i].z;
        float distSq = dx*dx + dy*dy + dz*dz;
        
        if (distSq < minDistSq) {
            return 0;
        }
    }
    return 1;
}

// Fonction pour ajouter une position au tableau avec sa couleur
static void add_cube_position(float x, float y, float z, float r, float g, float b) {
    if (cubeCount >= cubeCapacity) {
        cubeCapacity = (cubeCapacity == 0) ? 1000 : cubeCapacity * 2;
        placedCubes = (CubePosition*)realloc(placedCubes, cubeCapacity * sizeof(CubePosition));
        if (!placedCubes) {
            fprintf(stderr, "Erreur d'allocation mémoire\n");
            exit(1);
        }
    }
    placedCubes[cubeCount].x = x;
    placedCubes[cubeCount].y = y;
    placedCubes[cubeCount].z = z;
    placedCubes[cubeCount].r = r;
    placedCubes[cubeCount].g = g;
    placedCubes[cubeCount].b = b;
    cubeCount++;
}
static void calculate_prime_color(int index, int totalCount, int isPrime, float *r, float *g, float *b) {
    // Progression linéaire de 0 à 1
    float progress = (float)index / (float)totalCount;
    
    // Teinte linéaire sur tout le spectre (0° à 360°)
    float hue = progress * 360.0f;
    
    // Saturation et luminosité FIXES pour des couleurs pures et vives
    float saturation = 0.95f;  // Très saturé (peu de blanc)
    float value = 0.90f;       // Lumineux mais pas trop (évite le blanc)
    
    // Conversion HSV → RGB
    float c = value * saturation;
    float x = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));
    float m = value - c;
    
    float r_temp, g_temp, b_temp;
    
    if (hue >= 0.0f && hue < 60.0f) {
        r_temp = c; g_temp = x; b_temp = 0.0f;  // Rouge → Jaune
    } else if (hue >= 60.0f && hue < 120.0f) {
        r_temp = x; g_temp = c; b_temp = 0.0f;  // Jaune → Vert
    } else if (hue >= 120.0f && hue < 180.0f) {
        r_temp = 0.0f; g_temp = c; b_temp = x;  // Vert → Cyan
    } else if (hue >= 180.0f && hue < 240.0f) {
        r_temp = 0.0f; g_temp = x; b_temp = c;  // Cyan → Bleu
    } else if (hue >= 240.0f && hue < 300.0f) {
        r_temp = x; g_temp = 0.0f; b_temp = c;  // Bleu → Magenta
    } else {
        r_temp = c; g_temp = 0.0f; b_temp = x;  // Magenta → Rouge
    }
    
    *r = r_temp + m;
    *g = g_temp + m;
    *b = b_temp + m;
    
    // Distinction nombres premiers vs composés
    if (isPrime) {
        // Nombres premiers : un peu plus lumineux
        float boost = 1.08f;
        *r = fminf(*r * boost, 1.0f);
        *g = fminf(*g * boost, 1.0f);
        *b = fminf(*b * boost, 1.0f);
    } else {
        // Nombres composés : légèrement plus sombres
        float darken = 0.75f;
        *r *= darken;
        *g *= darken;
        *b *= darken;
    }
}
static void draw_progress_bar() {
    // Sauvegarder les matrices actuelles
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Désactiver l'éclairage et le depth test pour l'UI 2D
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    // Fond noir semi-transparent
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(800, 0);
    glVertex2f(800, 600);
    glVertex2f(0, 600);
    glEnd();
    
    // Cadre de la barre de progression
    float barWidth = 600.0f;
    float barHeight = 40.0f;
    float barX = (800 - barWidth) / 2.0f;
    float barY = 280.0f;
    
    // Contour blanc
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(barX, barY);
    glVertex2f(barX + barWidth, barY);
    glVertex2f(barX + barWidth, barY + barHeight);
    glVertex2f(barX, barY + barHeight);
    glEnd();
    
    // Remplissage de la progression (dégradé arc-en-ciel)
    if (initProgress > 0.0f) {
        float fillWidth = barWidth * initProgress;
        
        // Calculer la couleur arc-en-ciel basée sur la progression
        float hue = initProgress * 6.0f;
        int h_i = (int)hue;
        float f = hue - h_i;
        float q = 1.0f - f;
        float r, g, b;
        
        switch(h_i % 6) {
            case 0: r = 1.0f; g = f;    b = 0.0f; break;
            case 1: r = q;    g = 1.0f; b = 0.0f; break;
            case 2: r = 0.0f; g = 1.0f; b = f;    break;
            case 3: r = 0.0f; g = q;    b = 1.0f; break;
            case 4: r = f;    g = 0.0f; b = 1.0f; break;
            case 5: r = 1.0f; g = 0.0f; b = q;    break;
            default: r = 1.0f; g = 0.0f; b = 0.0f; break;
        }
        
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
        glVertex2f(barX + 2, barY + 2);
        glVertex2f(barX + fillWidth - 2, barY + 2);
        glVertex2f(barX + fillWidth - 2, barY + barHeight - 2);
        glVertex2f(barX + 2, barY + barHeight - 2);
        glEnd();
    }
    
    // Texte "Initializing Prime Spiral..."
    glColor3f(1.0f, 1.0f, 1.0f);
    char text[100];
    sprintf(text, "Initializing Prime Spiral... %.1f%%", initProgress * 100.0f);
    
    glRasterPos2f(barX, barY + barHeight + 20);
    for (char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    
    // Restaurer les matrices
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Fonction pour initialiser toutes les positions des cubes avec progression
static void initialize_prime_spiral(void) {
    int i, j;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float ax = 0.0f;
    float ay = 90.0f;
    float az = 0.0f;
    float step = 0.002f;
    float minDistance = PRIME_VOXEL_SIZE * 1.1f;
    
    cubeCount = 0;
    int cubeIndex = 0;
    isInitializing = 1;
    
    for (i = 0; i < PRIME_LIMIT; i++) {
        // Mettre à jour la progression
        initProgress = (float)i / (float)PRIME_LIMIT;
        
        // Redessiner tous les 100 itérations pour montrer la progression
        if (i % 100 == 0) {
            glutPostRedisplay();
            glutMainLoopEvent();
        }
        
        int iterations = (int)(log((double)i + 1.0) * PRIME_SPIRAL_STEP) + 1;
        int currentIsPrime = is_prime(i);
        float ssx = 0.0f, ssy = 0.0f, ssz = 0.0f;

        
        x = x + iterations*step * cosf(ax * M_PI / 180.0f) * sinf(ay * M_PI / 180.0f);
        y = y + iterations*step * sinf(ax * M_PI / 180.0f) * sinf(ay * M_PI / 180.0f);
        z = z + iterations*step * cosf(ay * M_PI / 180.0f);
            
        if (is_position_valid(x, y, z, minDistance)) {
            float r, g, b;
            calculate_prime_color(i, PRIME_LIMIT, currentIsPrime, &r, &g, &b);
            add_cube_position(x, y, z, r, g, b);
            placedCubes[cubeIndex].sx = ssx;
            placedCubes[cubeIndex].sy = ssy;
            placedCubes[cubeIndex].sz = ssz;
            cubeIndex++;
        }
        
        
        if (currentIsPrime) {
            ay += PRIME_ROTATION;
            if (ay >= 360.0f) {
                az += PRIME_ROTATION;
                ay = 0.0f;
            }
            if (az >= 360.0f) {
                az = 0.0f;
                ax += PRIME_ROTATION;
            }
            if (ax >= 360.0f) {
                ax = 0.0f;
                ay += PRIME_ROTATION;
            }
        }
    }
    
    // Point central à regarder (centre de la spirale)
    if (cubeCount > 0) {
        lookX = placedCubes[cubeCount - 1].x / 2.0f;
        lookY = placedCubes[cubeCount - 1].y / 2.0f;
        lookZ = placedCubes[cubeCount - 1].z / 2.0f;
        cameraDistance = sqrtf(lookX*lookX + lookY*lookY + lookZ*lookZ) * 1.5f;
    }
    
    initProgress = 1.0f;
    isInitializing = 0;
    
    printf("Spirale initialisée avec %d cubes\n", cubeCount);
    initialized = 1;
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
    gluPerspective(60.0, (h > 0) ? (float)w / (float)h : 1.0f, 0.1, 60000.0);
    glMatrixMode(GL_MODELVIEW);
}

static void idle(void) {
    // on se contente de redessiner en continu
    clock_t current_time = clock();
    double elapsed_ms = (double)(current_time - last_frame_time) * 1000.0 / CLOCKS_PER_SEC;
    
    if (!initialized && !isInitializing) {
        initialize_prime_spiral();
        return;
    }
    if (initialized && !isInitializing) {
        idlefunc = primes;
    }
    if (elapsed_ms >= FRAME_TIME_MS) {
        last_frame_time = current_time;
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
static void mouse(int x, int y) {
    if (firstMouse) {
        lastMouseX = x;
        lastMouseY = y;
        firstMouse = 0;
        return;
    }
    
    // Calculer le déplacement depuis la dernière position
    float deltaX = (float)(x - lastMouseX);
    float deltaY = (float)(y - lastMouseY);
    
    lastMouseX = x;
    lastMouseY = y;
    
    // Appliquer la sensibilité
    cameraYaw += deltaX * mouseSensitivity;
    cameraPitch -= deltaY * mouseSensitivity;
    
    // Limiter le pitch pour éviter le retournement
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
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

static void primes(void) {
    
    glClearColor(0.0, 0.0, 0.4, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glLoadIdentity();
    
    cameraYaw += 0.1f; // Rotation automatique lente
    cameraPitch += 0.01f;
    cameraDistance += sinf((float)clock() / CLOCKS_PER_SEC * 0.5f) * 0.05f;
    // Calculer la position de la caméra
    eyesX = lookX + cameraDistance * cosf(cameraYaw * M_PI / 180.0f) * cosf(cameraPitch * M_PI / 180.0f);
    eyesY = lookY + cameraDistance * sinf(cameraPitch * M_PI / 180.0f);
    eyesZ = lookZ + cameraDistance * sinf(cameraYaw * M_PI / 180.0f) * cosf(cameraPitch * M_PI / 180.0f);
    
    if (!initialized && cubeCount > 2) {
        eyesX = placedCubes[cubeCount-1].x+10;
        eyesY = placedCubes[cubeCount-1].y+10;
        eyesZ = placedCubes[cubeCount-1].z+10;
        lookX = placedCubes[cubeCount].x;
        lookY = placedCubes[cubeCount].y;
        lookZ = placedCubes[cubeCount].z;
    }
    gluLookAt(
        eyesX, eyesY, eyesZ,
        lookX, lookY, lookZ,
        0.0, 1.0, 0.0
    );
    glEnable(GL_LIGHTING);
    
    for (int i = 0; i < cubeCount; i++) {
        glPushMatrix();
        glTranslatef(placedCubes[i].x, placedCubes[i].y, placedCubes[i].z);
        // Définir la couleur comme matériau émissif + diffus
        GLfloat color[] = { placedCubes[i].r, placedCubes[i].g, placedCubes[i].b, 1.0f };
        GLfloat emission[] = { 
            placedCubes[i].r * 0.3f,  // 30% de lumière émise
            placedCubes[i].g * 0.3f, 
            placedCubes[i].b * 0.3f, 
            1.0f 
        };
        
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);  // Couleur auto-éclairante
        
        glColor3f(placedCubes[i].r, placedCubes[i].g, placedCubes[i].b);
        glScalef(PRIME_VOXEL_SIZE, PRIME_VOXEL_SIZE, PRIME_VOXEL_SIZE);
        
        glutSolidDodecahedron();
        glPopMatrix();
    }
    if (!initialized) {
        draw_progress_bar();
    }
    glutSwapBuffers();
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
    gluPerspective(60.0, 1.0, 0.1, 60000.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    GLfloat lightPos[] = { 0.0f, 100.0f, 0.0f, 1.0f };  // Lumière au-dessus
    GLfloat lightAmb[] = { 0.2f, 0.2f, 0.2f, 1.0f };    // Beaucoup d'ambiant = couleurs préservées
    GLfloat lightDiff[] = { 0.1f, 0.1f, 0.1f, 1.0f };   // Diffuse modérée pour effet 3D subtil
    GLfloat lightSpec[] = { 0.0f, 0.0f, 0.0f, 1.0f };   // Pas de spéculaire = pas de reflets blancs
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    
    // Paramètres matériaux pour préserver les couleurs
    GLfloat matSpec[] = { 0.0f, 0.0f, 0.0f, 1.0f };  // Pas de spéculaire
    GLfloat matShininess[] = { 0.0f };               // Pas de brillance
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

    idlefunc = NULL;
}
int main(int argc, char **argv) {
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Prime Numbers Demo");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);

    glutAttachMenu(GLUT_RIGHT_BUTTON);

    initGL();
    idlefunc = primes;
    glutDisplayFunc(display);
    glutMotionFunc(mouse);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutSpecialUpFunc(specialUp);

    glutMainLoop();
    return 0;
}