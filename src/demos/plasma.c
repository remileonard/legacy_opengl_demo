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


static clock_t last_frame_time = 0;

float width = 640.0f;
float height = 480.0f;
static int nbframes = 0;
#define MAX_FRAMES 1000
#define MAX_EFFECT 4
void (*idlefunc)(void) = NULL;
static int current_effect = 0;

void displayPlasma(void);
static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void display(void);
static void initGL(void);

void (*effects[MAX_EFFECT])(void);

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
    width = (float)w;
    height = (float)h;
    glMatrixMode(GL_MODELVIEW);
}

static void idle(void) {
    // on se contente de redessiner en continu
    clock_t current_time = clock();
    double elapsed_ms = (double)(current_time - last_frame_time) * 1000.0 / CLOCKS_PER_SEC;
    
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
    case '1':
    case '2':
    case '3':
    case '4':
        nbframes = 0;
        current_effect = key - '1';
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

static void display(void) {
    nbframes++;
    if (nbframes >= MAX_FRAMES) {
        nbframes = 0;
        current_effect = (current_effect + 1) % MAX_EFFECT;
    }
    if (current_effect < MAX_EFFECT && effects[current_effect]) {
        effects[current_effect]();
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glutSwapBuffers();
    }
}
void displayPlasma(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint * buffer = malloc(width * height * sizeof(GLuint));
    if (!buffer) return;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char r = (unsigned char)(128 + 127 * sinf(x * 0.05f + glutGet(GLUT_ELAPSED_TIME) * 0.002f));
            unsigned char g = (unsigned char)(128 + 127 * sinf(y * 0.05f + glutGet(GLUT_ELAPSED_TIME) * 0.002f));
            unsigned char b = (unsigned char)(128 + 127 * sinf((x + y) * 0.05f + glutGet(GLUT_ELAPSED_TIME) * 0.002f));
            buffer[y * (int)width + x] = (255 << 24) | (r << 16) | (g << 8) | b;
        }
    }
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    free(buffer);
    glutSwapBuffers();
}
void displayMetaBob(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLuint * buffer = malloc(width * height * sizeof(GLuint));
    if (!buffer) return;
    
    memset(buffer, 0, width * height * sizeof(GLuint));
    
    #define NUM_BOBS 30
    static int initialized = 0;
    
    float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    
    // Positions des bobs avec trajectoires circulaires/elliptiques
    for (int b = 0; b < NUM_BOBS; b++) {
        float angle = time * (1.0f + b * 0.1f);
        float radiusX = width * 0.3f * (1.0f + 0.3f * sinf(b * 0.7f));
        float radiusY = height * 0.3f * (1.0f + 0.3f * cosf(b * 0.7f));
        
        float x = width/2 + cosf(angle) * radiusX;
        float y = height/2 + sinf(angle + b) * radiusY;
        
        float bobRadius = 40.0f + 15.0f * sinf(time * 3.0f + b);
        
        // Limiter la zone de dessin pour optimiser
        int minX = (int)fmaxf(0, x - bobRadius - 5);
        int maxX = (int)fminf(width - 1, x + bobRadius + 5);
        int minY = (int)fmaxf(0, y - bobRadius - 5);
        int maxY = (int)fminf(height - 1, y + bobRadius + 5);
        
        for (int py = minY; py <= maxY; py++) {
            for (int px = minX; px <= maxX; px++) {
                float dx = px - x;
                float dy = py - y;
                float dist = sqrtf(dx * dx + dy * dy);
                
                if (dist < bobRadius) {
                    float intensity = 1.0f - (dist / bobRadius);
                    intensity = powf(intensity, 2.0f); // Courbe
                    
                    float hue = (float)b / NUM_BOBS + time * 0.1f;
                    
                    unsigned char r = (unsigned char)(128 + 127 * sinf(hue * 6.28f) * intensity);
                    unsigned char g = (unsigned char)(128 + 127 * sinf(hue * 6.28f + 2.09f) * intensity);
                    unsigned char b_col = (unsigned char)(128 + 127 * sinf(hue * 6.28f + 4.18f) * intensity);
                    
                    int idx = py * (int)width + px;
                    unsigned char old_r = (buffer[idx] >> 16) & 0xFF;
                    unsigned char old_g = (buffer[idx] >> 8) & 0xFF;
                    unsigned char old_b = buffer[idx] & 0xFF;
                    
                    int new_r = r + old_r;
                    int new_g = g + old_g;
                    int new_b = b_col + old_b;
                    
                    buffer[idx] = (255 << 24) | 
                                  ((new_r > 255 ? 255 : new_r) << 16) | 
                                  ((new_g > 255 ? 255 : new_g) << 8) | 
                                  (new_b > 255 ? 255 : new_b);
                }
            }
        }
    }
    
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    free(buffer);
    glutSwapBuffers();
}
void displayUnlimitedBob(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    #define NUM_BUFFERS 6
    static GLuint *buffers[NUM_BUFFERS] = {NULL};
    static int currentBuffer = 0;
    static int initialized = 0;
    static int w = 0;
    static int h = 0;
    
    // Initialiser les buffers
    if (!initialized) {
        w = (int)width;
        h = (int)height;
        for (int i = 0; i < NUM_BUFFERS; i++) {
            buffers[i] = (GLuint *)malloc(w * h * sizeof(GLuint));
            if (buffers[i]) {
                memset(buffers[i], 0, w * h * sizeof(GLuint));
            }
        }
        initialized = 1;
    }
    
    float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    
    // Calculer la nouvelle position du bob avec une trajectoire circulaire
    float angle = time * 2.0f;
    float radiusX = w * 0.35f;
    float radiusY = h * 0.35f;
    
    float bobX = w / 2.0f + cosf(angle) * radiusX;
    float bobY = h / 2.0f + sinf(angle) * radiusY;
    
    // Taille du bob (rayon du cercle)
    float bobRadius = 25.0f;
    
    // Couleur du bob qui change avec le temps
    float hue = time * 0.3f;
    unsigned char base_r = (unsigned char)(128 + 127 * sinf(hue));
    unsigned char base_g = (unsigned char)(128 + 127 * sinf(hue + 2.09f));
    unsigned char base_b = (unsigned char)(128 + 127 * sinf(hue + 4.18f));
    
    // Dessiner le bob dans le buffer actuel
    int minX = (int)fmaxf(0, bobX - bobRadius - 1);
    int maxX = (int)fminf(w - 1, bobX + bobRadius + 1);
    int minY = (int)fmaxf(0, bobY - bobRadius - 1);
    int maxY = (int)fminf(h - 1, bobY + bobRadius + 1);
    
    for (int py = minY; py <= maxY; py++) {
        for (int px = minX; px <= maxX; px++) {
            float dx = px - bobX;
            float dy = py - bobY;
            float dist = sqrtf(dx * dx + dy * dy);
            
            int idx = py * (int)w + px;
            
            // Bordure noire (1 pixel autour du cercle)
            if (dist > bobRadius && dist <= bobRadius + 1.0f) {
                buffers[currentBuffer][idx] = (255 << 24) | (0 << 16) | (0 << 8) | 0;
            }
            // Cercle de couleur
            else if (dist <= bobRadius) {
                buffers[currentBuffer][idx] = (255 << 24) | (base_r << 16) | (base_g << 8) | base_b;
            }
        }
    }
    
    // AFFICHER le buffer actuel (qui contient maintenant le nouveau bob)
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, buffers[currentBuffer]);
    
    // Passer au buffer suivant pour la prochaine frame
    currentBuffer++;
    currentBuffer %= NUM_BUFFERS;
    
    glutSwapBuffers();
}
void displayShadedBob(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    #define NUM_BUFFERS 6
    #define NUM_BOB_PATHS 3  // Nombre de trajectoires différentes
    static GLuint *buffers[NUM_BUFFERS] = {NULL};
    static int currentBuffer = 0;
    static int initialized = 0;
    
    // Initialiser les buffers
    if (!initialized) {
        for (int i = 0; i < NUM_BUFFERS; i++) {
            buffers[i] = (GLuint *)malloc(width * height * sizeof(GLuint));
            if (buffers[i]) {
                memset(buffers[i], 0, width * height * sizeof(GLuint));
            }
        }
        initialized = 1;
    }
    
    float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    
    // Copier le buffer actuel dans le buffer suivant
    int nextBuffer = (currentBuffer + 1) % NUM_BUFFERS;
    memcpy(buffers[nextBuffer], buffers[currentBuffer], width * height * sizeof(GLuint));
    
    // Dessiner plusieurs bobs avec différentes trajectoires
    for (int bobIdx = 0; bobIdx < NUM_BOB_PATHS; bobIdx++) {
        float speed = 1.5f + bobIdx * 0.5f;
        float angle = time * speed + bobIdx * 2.0f;
        
        // Trajectoires elliptiques avec différentes tailles
        float radiusX = width * (0.25f + bobIdx * 0.05f);
        float radiusY = height * (0.25f + bobIdx * 0.05f);
        
        float bobX = width / 2.0f + cosf(angle) * radiusX;
        float bobY = height / 2.0f + sinf(angle) * radiusY;
        
        float bobRadius = 20.0f + bobIdx * 5.0f;
        
        int minX = (int)fmaxf(0, bobX - bobRadius);
        int maxX = (int)fminf(width - 1, bobX + bobRadius);
        int minY = (int)fmaxf(0, bobY - bobRadius);
        int maxY = (int)fminf(height - 1, bobY + bobRadius);
        
        for (int py = minY; py <= maxY; py++) {
            for (int px = minX; px <= maxX; px++) {
                float dx = px - bobX;
                float dy = py - bobY;
                float dist = sqrtf(dx * dx + dy * dy);
                
                if (dist < bobRadius) {
                    float intensity = 1.0f - (dist / bobRadius);
                    intensity = powf(intensity, 2.0f);
                    
                    // Couleur différente pour chaque bob
                    float hue = time * 0.3f + bobIdx * 2.0f;
                    unsigned char r = (unsigned char)(255 * intensity * (0.5f + 0.5f * sinf(hue)));
                    unsigned char g = (unsigned char)(255 * intensity * (0.5f + 0.5f * sinf(hue + 2.09f)));
                    unsigned char b = (unsigned char)(255 * intensity * (0.5f + 0.5f * sinf(hue + 4.18f)));
                    
                    int idx = py * (int)width + px;
                    
                    // Additionner avec le pixel existant pour l'effet de superposition
                    unsigned char old_r = (buffers[nextBuffer][idx] >> 16) & 0xFF;
                    unsigned char old_g = (buffers[nextBuffer][idx] >> 8) & 0xFF;
                    unsigned char old_b = buffers[nextBuffer][idx] & 0xFF;
                    
                    int new_r = r + old_r;
                    int new_g = g + old_g;
                    int new_b = b + old_b;
                    
                    buffers[nextBuffer][idx] = (255 << 24) | 
                                              ((new_r > 255 ? 255 : new_r) << 16) | 
                                              ((new_g > 255 ? 255 : new_g) << 8) | 
                                              (new_b > 255 ? 255 : new_b);
                }
            }
        }
    }
    
    // Appliquer un fade progressif sur tout le buffer
    float fadeAmount = 0.92f;  // Plus petit = traînée plus longue
    for (int i = 0; i < width * height; i++) {
        unsigned char r = (buffers[nextBuffer][i] >> 16) & 0xFF;
        unsigned char g = (buffers[nextBuffer][i] >> 8) & 0xFF;
        unsigned char b = buffers[nextBuffer][i] & 0xFF;
        
        r = (unsigned char)(r * fadeAmount);
        g = (unsigned char)(g * fadeAmount);
        b = (unsigned char)(b * fadeAmount);
        
        buffers[nextBuffer][i] = (255 << 24) | (r << 16) | (g << 8) | b;
    }
    
    // Afficher le buffer actuel
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffers[currentBuffer]);
    
    // Rotation des buffers
    currentBuffer = nextBuffer;
    
    glutSwapBuffers();
}
static void initGL(void) {
    last_frame_time = clock();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    effects[0] = displayPlasma;
    effects[1] = displayMetaBob;
    effects[2] = displayUnlimitedBob;
    effects[3] = displayShadedBob;
    idlefunc = displayUnlimitedBob;
}
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    // Double buffer + RGB + z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Maze Example (FreeGLUT)");

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

    glutMainLoop();
    return 0;
}