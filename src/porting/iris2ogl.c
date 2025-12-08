/*
 * iris2ogl.c - IRIS GL to OpenGL/GLUT compatibility layer implementation
 */

#include "iris2ogl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>  // For Sleep()
#else
#include <unistd.h>   // For usleep()
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// === Global state ===
float iris_colormap[256][3];
static int current_matrix_mode = GL_MODELVIEW;
static int iris_active_light_count = 0;
static Object next_object_id = 1;
static Object current_object = 0;
static Boolean in_object_definition = FALSE;

// Font management
static fmfonthandle current_font = NULL;
static float current_raster_x = 0.0f;
static float current_raster_y = 0.0f;
static float current_raster_z = 0.0f;

// Pattern management
#define MAX_PATTERNS 100
static GLubyte stipple_patterns[MAX_PATTERNS][128];
static int pattern_defined[MAX_PATTERNS] = {0};

// Event queue
#define EVENT_QUEUE_SIZE 256
typedef struct {
    Device device;
    int16_t value;
} Event;

static Event event_queue[EVENT_QUEUE_SIZE];
static int event_queue_head = 0;
static int event_queue_tail = 0;
static Device queued_devices[256] = {0};
static Boolean mouse_state[3] = {FALSE, FALSE, FALSE};
static Boolean key_state[256] = {FALSE};

static int current_drawmode = NORMALDRAW;
static Screencoord current_viewport[4] = {0, 0, 640, 480};

static GLboolean dm_depth_test   = GL_TRUE;
static GLboolean dm_blend        = GL_FALSE;
static GLboolean dm_color_mask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
static int overlay_planes = 0;  // >0 si overlay actif
static int underlay_planes = 0; // >0 si underlay actif

static float iris_clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};


static int current_pattern = 0;

// === Color Management ===
static void queue_event(Device dev, int16_t val);

void set_iris_colormap(int index, float r, float g, float b) {
    if (index >= 0 && index < 256) {
        iris_colormap[index][0] = r;
        iris_colormap[index][1] = g;
        iris_colormap[index][2] = b;
    }
}
void iris_init_colormap(void) {
    // Initialize with grayscale ramp
    for (int i = 0; i < 256; i++) {
        float val = i / 255.0f;
        iris_colormap[i][0] = val;
        iris_colormap[i][1] = val;
        iris_colormap[i][2] = val;
    }
    
    // Set standard IRIS GL colors
    iris_colormap[BLACK][0] = 0.0f;   iris_colormap[BLACK][1] = 0.0f;   iris_colormap[BLACK][2] = 0.0f;
    iris_colormap[RED][0] = 1.0f;     iris_colormap[RED][1] = 0.0f;     iris_colormap[RED][2] = 0.0f;
    iris_colormap[GREEN][0] = 0.0f;   iris_colormap[GREEN][1] = 1.0f;   iris_colormap[GREEN][2] = 0.0f;
    iris_colormap[YELLOW][0] = 1.0f;  iris_colormap[YELLOW][1] = 1.0f;  iris_colormap[YELLOW][2] = 0.0f;
    iris_colormap[BLUE][0] = 0.0f;    iris_colormap[BLUE][1] = 0.0f;    iris_colormap[BLUE][2] = 1.0f;
    iris_colormap[MAGENTA][0] = 1.0f; iris_colormap[MAGENTA][1] = 0.0f; iris_colormap[MAGENTA][2] = 1.0f;
    iris_colormap[CYAN][0] = 0.0f;    iris_colormap[CYAN][1] = 1.0f;    iris_colormap[CYAN][2] = 1.0f;
    iris_colormap[WHITE][0] = 1.0f;   iris_colormap[WHITE][1] = 1.0f;   iris_colormap[WHITE][2] = 1.0f;
}

void iris_set_color_index(int index) {
    if (index >= 0 && index < 256) {
        float r = iris_colormap[index][0];
        float g = iris_colormap[index][1];
        float b = iris_colormap[index][2];

        // couleur de dessin
        glColor3f(r, g, b);

        // couleur de fond IRIS pour clear()
        iris_clear_color[0] = r;
        iris_clear_color[1] = g;
        iris_clear_color[2] = b;
        iris_clear_color[3] = 1.0f;
    }
}

void cpack(uint32_t color) {
    // Convert packed RGB (0xRRGGBB) to float components
    float b = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float r = (color & 0xFF) / 255.0f;
    glColor3f(r, g, b);
    iris_clear_color[0] = r;
    iris_clear_color[1] = g;
    iris_clear_color[2] = b;
    iris_clear_color[3] = 1.0f;
}

void mapcolor(Colorindex index, RGBvalue r, RGBvalue g, RGBvalue b) {
    if (index < 256) {
        iris_colormap[index][0] = r / 255.0f;
        iris_colormap[index][1] = g / 255.0f;
        iris_colormap[index][2] = b / 255.0f;
    }
}
void clear() {
    // Sauvegarder l'état du scissor test
    GLboolean scissor_enabled = glIsEnabled(GL_SCISSOR_TEST);
    GLint previous_scissor[4];
    if (scissor_enabled) {
        glGetIntegerv(GL_SCISSOR_BOX, previous_scissor);
    }

    // Récupérer le viewport courant
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    // Activer le scissor test et le configurer pour le viewport
    glEnable(GL_SCISSOR_TEST);
    glScissor(vp[0], vp[1], vp[2], vp[3]);

    // Définir la couleur de clear
    glClearColor(iris_clear_color[0],
                 iris_clear_color[1],
                 iris_clear_color[2],
                 iris_clear_color[3]);

    // Effectuer le clear
    if (current_pattern == 0) {
        glClear(GL_COLOR_BUFFER_BIT);
    } else {
        // Si un pattern est actif, dessiner un quad couvrant le viewport
        GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
        GLboolean lighting  = glIsEnabled(GL_LIGHTING);
        GLint matrixMode;
        glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
        GLint x = vp[0];
        GLint y = vp[1];
        GLint w = vp[2];
        GLint h = vp[3];

        // Désactiver ce qui pourrait gêner
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        // Passer en projection 2D avec les coordonnées du viewport
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0.0, (GLdouble)w, 0.0, (GLdouble)h);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        // Définir la couleur de fond
        glColor3f(iris_clear_color[0],
                iris_clear_color[1],
                iris_clear_color[2]);

        // Dessiner un quad couvrant tout le viewport courant
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0.0f,      0.0f     );
            glVertex2f((float)w,  0.0f     );
            glVertex2f((float)w,  (float)h );
            glVertex2f(0.0f,      (float)h );
        glEnd();

        // Restauration des matrices
        glPopMatrix(); // MODELVIEW
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(matrixMode);
    }
    

    // Restaurer l'état précédent du scissor test
    if (scissor_enabled) {
        glScissor(previous_scissor[0], previous_scissor[1],
                  previous_scissor[2], previous_scissor[3]);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}
#ifdef CLEAR_WITH_QUAD
/* 
je le garde ici car pour cycles il permet d'avoir l'effet souhaité. Je regarderais comment
l'implémenter avec le clear actuel
*/
void clear() {
    // Sauvegarde d'un minimum d'état
    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean lighting  = glIsEnabled(GL_LIGHTING);
    GLint matrixMode;
    glGetIntegerv(GL_MATRIX_MODE, &matrixMode);

    // Récupérer le viewport courant
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    GLint x = vp[0];
    GLint y = vp[1];
    GLint w = vp[2];
    GLint h = vp[3];

    // Désactiver ce qui pourrait gêner
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Passer en projection 2D avec les coordonnées du viewport
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)w, 0.0, (GLdouble)h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Définir la couleur de fond
    glColor3f(iris_clear_color[0],
              iris_clear_color[1],
              iris_clear_color[2]);

    // Dessiner un quad couvrant tout le viewport courant
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0.0f,      0.0f     );
        glVertex2f((float)w,  0.0f     );
        glVertex2f((float)w,  (float)h );
        glVertex2f(0.0f,      (float)h );
    glEnd();

    // Restauration des matrices
    glPopMatrix(); // MODELVIEW
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(matrixMode);

    // Restauration des états
    if (depthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (lighting)  glEnable(GL_LIGHTING);   else glDisable(GL_LIGHTING);
}
#endif
// === Buffer Swapping ===

void swapbuffers(void) {
    glutSwapBuffers();
}

// === Geometric Primitives ===

void rectf(Coord x1, Coord y1, Coord x2, Coord y2) {
    glRectf(x1, y1, x2, y2);
}

void rectfs(Scoord x1, Scoord y1, Scoord x2, Scoord y2) {
    glRects(x1, y1, x2, y2);
}

void rect(Icoord x1, Icoord y1, Icoord x2, Icoord y2) {
    glRecti(x1, y1, x2, y2);
}

void circf(Coord x, Coord y, Coord radius) {
    int segments = 32;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
    }
    glEnd();
}

void circ(Coord x, Coord y, Coord radius) {
    int segments = 32;
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex2f(x + cos(angle) * radius, y + sin(angle) * radius);
    }
    glEnd();
}

void sboxf(Coord x1, Coord y1, Coord x2, Coord y2) {
    glRectf(x1, y1, x2, y2);
}

void sboxfi(Icoord x1, Icoord y1, Icoord x2, Icoord y2) {
    glRecti(x1, y1, x2, y2);
}

void polf2(int32_t n, Coord parray[][2]) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++) {
        glVertex2f(parray[i][0], parray[i][1]);
    }
    glEnd();
}

void polf2i(int32_t n, Icoord parray[][2]) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++) {
        glVertex2i(parray[i][0], parray[i][1]);
    }
    glEnd();
}

void polf2s(int32_t n, Scoord parray[][2]) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++) {
        glVertex2s(parray[i][0], parray[i][1]);
    }
    glEnd();
}

// === Matrix and Transformations ===
void getmatrix(Matrix m)
{
    if (current_matrix_mode == GL_PROJECTION) {
        glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat*)m);
    } else {
        glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)m);
    }
}

void loadmatrix(Matrix m)
{
    if (current_matrix_mode == GL_PROJECTION) {
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf((GLfloat*)m);
    } else {
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf((GLfloat*)m);
    }
}
void rot(float angle, char axis) {
    switch (axis) {
        case 'x':
        case 'X':
            glRotatef(angle, 1.0f, 0.0f, 0.0f);
            break;
        case 'y':
        case 'Y':
            glRotatef(angle, 0.0f, 1.0f, 0.0f);
            break;
        case 'z':
        case 'Z':
            glRotatef(angle, 0.0f, 0.0f, 1.0f);
            break;
        default:
            fprintf(stderr, "rot: invalid axis '%c'\n", axis);
            break;
    }
}

void mmode(int mode) {
    switch (mode) {
        case MSINGLE:
        case MVIEWING:
            glMatrixMode(GL_MODELVIEW);
            current_matrix_mode = GL_MODELVIEW;
            break;
        case MPROJECTION:
            glMatrixMode(GL_PROJECTION);
            current_matrix_mode = GL_PROJECTION;
            break;
        default:
            fprintf(stderr, "mmode: unknown mode %d\n", mode);
            break;
    }
}
long getmmode(void)
{
    return current_matrix_mode;
}

void perspective(Angle fov, float aspect, Coord near_val, Coord far_val) {
    // IRIS GL uses 1/10 degree units
    // IRIS GL perspective automatically sets projection mode
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float fov_degrees = fov / 10.0f;
    gluPerspective(fov_degrees, aspect, near_val, far_val);
    glMatrixMode(GL_MODELVIEW);
}

void ortho(Coord left, Coord right, Coord bottom, Coord top, Coord near_val, Coord far_val) {
    // IRIS GL ortho automatically sets projection mode
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, near_val, far_val);
    glMatrixMode(GL_MODELVIEW);
}

void lookat(Coord vx, Coord vy, Coord vz, Coord px, Coord py, Coord pz, Angle twist) {
    // twist is in 1/10 degree units in IRIS GL
    // For now, we ignore twist (typically 0)
    gluLookAt(vx, vy, vz, px, py, pz, 0.0, 1.0, 0.0);
}

// === Display Lists ===

Object genobj(void) {
    GLuint list = glGenLists(1);
    return (Object)list;
}

void makeobj(Object obj) {
    current_object = obj;
    glNewList((GLuint)obj, GL_COMPILE);
    in_object_definition = TRUE;
}

void closeobj(void) {
    glEndList();
    in_object_definition = FALSE;
    current_object = 0;
}

void callobj(Object obj) {
    glCallList((GLuint)obj);
}

void delobj(Object obj) {
    glDeleteLists((GLuint)obj, 1);
}

// === Rendering State ===

void shademodel(int mode) {
    glShadeModel(mode);
}

void zbuffer(Boolean enable) {
    if (enable) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void linewidth(int width) {
    glLineWidth((GLfloat)width);
}

// === Patterns ===



void defpattern(int id, int size, Pattern16 pattern) {
    if (id >= 0 && id < MAX_PATTERNS && size == 16) {
        // Convert 16x16 pattern to OpenGL stipple pattern (32x32)
        memset(stipple_patterns[id], 0, 128);
        for (int y = 0; y < 16; y++) {
            uint16_t row = pattern[y];
            // Replicate each row twice for 32x32
            for (int rep = 0; rep < 2; rep++) {
                int idx = (y * 2 + rep) * 4;
                // Replicate each bit twice horizontally
                for (int x = 0; x < 16; x++) {
                    if (row & (1 << (15 - x))) {
                        int bit_pos = x * 2;
                        stipple_patterns[id][idx + bit_pos / 8] |= (3 << (6 - (bit_pos % 8)));
                    }
                }
            }
        }
        pattern_defined[id] = 1;
    }
}

void setpattern(int id) {
    if (id == 0) {
        glDisable(GL_POLYGON_STIPPLE);
        current_pattern = 0;
    } else if (id > 0 && id < MAX_PATTERNS && pattern_defined[id]) {
        glEnable(GL_POLYGON_STIPPLE);
        glPolygonStipple(stipple_patterns[id]);
        current_pattern = id;
    }
}

// === Lighting ===
static LightDef lights[MAX_LIGHTS];
static MaterialDef materials[MAX_MATERIALS];
static int current_material = 0;
static int num_active_lights = 0;

static LightModelDef light_models[MAX_MATERIALS]; // ou un petit nombre fixe
static int current_light_model = 0;


void resetmaterials(void) {
    // Reset all materials to undefined
    for (int i = 0; i < MAX_MATERIALS; i++) {
        materials[i].defined = FALSE;
    }
}
void lmdef(int deftype, int index, int np, float props[]) {
    if (!props) return;
    int i = 0;
    // Note: np est ignoré dans IRIS GL - le tableau se termine par LMNULL
    
    switch (deftype) {
        case DEFLIGHT: {
            if (index < 0 || index >= MAX_LIGHTS) return;
            
            LightDef *light = &lights[index];
            
            // Initialize with defaults if not already defined
            if (!light->defined) {
                light->ambient[0] = light->ambient[1] = light->ambient[2] = 0.0f;
                light->ambient[3] = 1.0f;
                light->diffuse[0] = light->diffuse[1] = light->diffuse[2] = 1.0f;
                light->diffuse[3] = 1.0f;
                light->specular[0] = light->specular[1] = light->specular[2] = 1.0f;
                light->specular[3] = 1.0f;
                light->position[0] = light->position[1] = 0.0f;
                light->position[2] = 1.0f;
                light->position[3] = 0.0f; // Directional by default
                light->spot_direction[0] = 0.0f;
                light->spot_direction[1] = 0.0f;
                light->spot_direction[2] = -1.0f;
                light->spot_exponent = 0.0f;
                light->spot_cutoff = 180.0f;
                light->attenuation[0] = 1.0f; // constant
                light->attenuation[1] = 0.0f; // linear
                light->attenuation[2] = 0.0f; // quadratic
                light->defined = TRUE;
            }
            
            // Parse properties array - continue jusqu'à LMNULL
            int i = 0;
            while (1) {
                int property = (int)props[i++];
                
                if (property == LMNULL) {
                    break;
                }
                
                switch (property) {
                    case LCOLOR:
                    case DIFFUSE:
                        light->diffuse[0] = props[i++];
                        light->diffuse[1] = props[i++];
                        light->diffuse[2] = props[i++];
                        light->diffuse[3] = 1.0f;
                        break;
                        
                    case AMBIENT:
                        light->ambient[0] = props[i++];
                        light->ambient[1] = props[i++];
                        light->ambient[2] = props[i++];
                        light->ambient[3] = 1.0f;
                        break;
                        
                    case SPECULAR:
                        light->specular[0] = props[i++];
                        light->specular[1] = props[i++];
                        light->specular[2] = props[i++];
                        light->specular[3] = 1.0f;
                        break;
                        
                    case POSITION:
                        light->position[0] = props[i++];
                        light->position[1] = props[i++];
                        light->position[2] = props[i++];
                        light->position[3] = props[i++]; // w component
                        break;
                        
                    case SPOTDIRECTION:
                        light->spot_direction[0] = props[i++];
                        light->spot_direction[1] = props[i++];
                        light->spot_direction[2] = props[i++];
                        break;
                        
                    case SPOTLIGHT:
                        light->spot_exponent = props[i++];
                        light->spot_cutoff = props[i++];
                        break;
                        
                    default:
                        // Propriété inconnue - sauter (ne devrait pas arriver)
                        fprintf(stderr, "lmdef DEFLIGHT: unknown property %d\n", property);
                        break;
                }
            }
            
            break;
        }
        
        case DEFMATERIAL: {
            if (index < 0 || index >= MAX_MATERIALS) {
                printf("ERROR lmdef: Material index %d out of range!\n", index);
                return;
            }
            
            MaterialDef *mat = &materials[index];
            
            // Initialize with defaults
            if (!mat->defined) {
                mat->ambient[0] = 0.2f; mat->ambient[1] = 0.2f;
                mat->ambient[2] = 0.2f; mat->ambient[3] = 1.0f;
                mat->diffuse[0] = 0.8f; mat->diffuse[1] = 0.8f;
                mat->diffuse[2] = 0.8f; mat->diffuse[3] = 1.0f;
                mat->specular[0] = 0.0f; mat->specular[1] = 0.0f;
                mat->specular[2] = 0.0f; mat->specular[3] = 1.0f;
                mat->emission[0] = 0.0f; mat->emission[1] = 0.0f;
                mat->emission[2] = 0.0f; mat->emission[3] = 1.0f;
                mat->shininess = 0.0f;
                mat->alpha = 1.0f;
                mat->defined = TRUE;
            }
            
            // Parse properties - continue jusqu'à LMNULL
            int i = 0;
            while (1) {
                int property = (int)props[i++];
                
                if (property == LMNULL) {
                    break;
                }
                
                switch (property) {
                    case AMBIENT:
                        mat->ambient[0] = props[i++];
                        mat->ambient[1] = props[i++];
                        mat->ambient[2] = props[i++];
                        mat->ambient[3] = 1.0f;
                        break;
                        
                    case DIFFUSE:
                        mat->diffuse[0] = props[i++];
                        mat->diffuse[1] = props[i++];
                        mat->diffuse[2] = props[i++];
                        mat->diffuse[3] = 1.0f;
                        break;
                        
                    case SPECULAR:
                        mat->specular[0] = props[i++];
                        mat->specular[1] = props[i++];
                        mat->specular[2] = props[i++];
                        mat->specular[3] = 1.0f;
                        break;
                        
                    case EMISSION:
                        mat->emission[0] = props[i++];
                        mat->emission[1] = props[i++];
                        mat->emission[2] = props[i++];
                        mat->emission[3] = 1.0f;
                        break;
                        
                    case SHININESS:
                        mat->shininess = props[i++];
                        if (mat->shininess > 128.0f) mat->shininess = 128.0f;
                        if (mat->shininess < 0.0f) mat->shininess = 0.0f;
                        break;
                    case ALPHA:
                        mat->alpha = props[i++];
                        if (mat->alpha > 1.0f) mat->alpha = 1.0f;
                        if (mat->alpha < 0.0f) mat->alpha = 0.0f;
                        
                        // Mettre à jour tous les composants alpha
                        mat->ambient[3] = mat->alpha;
                        mat->diffuse[3] = mat->alpha;
                        mat->specular[3] = mat->alpha;
                        mat->emission[3] = mat->alpha;
                        break;    
                    default:
                        fprintf(stderr, "lmdef DEFMATERIAL: unknown property %d\n", property);
                        break;
                }
            }
            
            break;
        }
        
        case DEFLMODEL: {
            LightModelDef *lm = &light_models[index];
            // valeurs par défaut
            lm->ambient[0] = lm->ambient[1] = lm->ambient[2] = 0.2f;
            lm->ambient[3] = 1.0f;
            lm->local_viewer = FALSE;

            while (i < np) {
                int token = (int)props[i++];
                switch (token) {
                case AMBIENT:
                    lm->ambient[0] = props[i++];
                    lm->ambient[1] = props[i++];
                    lm->ambient[2] = props[i++];
                    lm->ambient[3] = 1.0f;
                    break;
                case LOCALVIEWER:
                    lm->local_viewer = (props[i++] != 0.0f);
                    break;
                case ATTENUATION:
                    lm->attenuation[0] = props[i++];   // constant
                    lm->attenuation[1] = props[i++];   // linear
                    break;
                case LMNULL:
                    i = np;
                    break;
                default:
                    // sauter les valeurs inconnues si besoin
                    break;
                }
            }
            lm->defined = TRUE;
            break;
        }
        
        default:
            fprintf(stderr, "lmdef: unknown deftype %d\n", deftype);
            break;
    }
}

void lmbind(int target, int index) {
    
    switch (target) {
        case LIGHT0:
        case LIGHT1:
        case LIGHT2:
        case LIGHT3:
        case LIGHT4:
        case LIGHT5:
        case LIGHT6:
        case LIGHT7: {
            GLenum light_id = GL_LIGHT0 + (target - LIGHT0);

            // On regarde si cette light était déjà activée côté GL
            GLboolean was_enabled = glIsEnabled(light_id);

            if (index == 0) {
                // Unbind/disable this light
                if (was_enabled) {
                    glDisable(light_id);
                    if (iris_active_light_count > 0) {
                        iris_active_light_count--;
                    }
                }

                // Si plus aucune light active, couper l'éclairage global
                if (iris_active_light_count == 0) {
                    glDisable(GL_LIGHTING);
                }
            } else if (index > 0 && index < MAX_LIGHTS && lights[index].defined) {
                LightDef *light = &lights[index];

                // Si elle n'était pas encore active, on incrémente le compteur
                if (!was_enabled) {
                    iris_active_light_count++;
                }

                // Si au moins une light active, s'assurer que GL_LIGHTING est ON
                if (iris_active_light_count > 0) {
                    glEnable(GL_LIGHTING);
                }
                glEnable(light_id);

                glLightfv(light_id, GL_POSITION, light->position);
                glLightfv(light_id, GL_AMBIENT,  light->ambient);
                glLightfv(light_id, GL_DIFFUSE,  light->diffuse);
                glLightfv(light_id, GL_SPECULAR, light->specular);
                glLightf (light_id, GL_SPOT_CUTOFF, 180.0f);
            }
            break;
        }
        
        case MATERIAL: {
            if (index == 0) {
                printf("  -> Using COLOR_MATERIAL mode\n");
                
                glEnable(GL_COLOR_MATERIAL);
                glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

                GLfloat default_specular[] = {0.0f, 0.0f, 0.0f, 1.0f};
                GLfloat default_emission[] = {0.0f, 0.0f, 0.0f, 1.0f};
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, default_specular);
                glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, default_emission);
                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

                current_material = 0;
            } else if (index > 0 && index < MAX_MATERIALS && materials[index].defined) {
                // Bind material
                printf("  -> Material %d is DEFINED, applying\n", index);
                    
                glDisable(GL_COLOR_MATERIAL);
                MaterialDef *mat = &materials[index];
                current_material = index;
                
                printf("     ambient=(%f, %f, %f)\n", mat->ambient[0], mat->ambient[1], mat->ambient[2]);
                printf("     diffuse=(%f, %f, %f)\n", mat->diffuse[0], mat->diffuse[1], mat->diffuse[2]);
                    
                glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat->ambient);
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat->diffuse);
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat->specular);
                glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat->emission);
                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mat->shininess);

                if (mat->alpha < 1.0f) {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                } else {
                    glDisable(GL_BLEND);
                }
            }
            break;
        }
        
        case LMODEL: {
            if (index == 0) {
                // Reset to default light model
                GLfloat default_ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
                glLightModelfv(GL_LIGHT_MODEL_AMBIENT, default_ambient);
                glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);

                // Reset light attenuation to defaults
                for (int li = 0; li < MAX_LIGHTS; ++li) {
                    GLenum lid = GL_LIGHT0 + li;
                    if (glIsEnabled(lid)) {
                        glLightf(lid, GL_CONSTANT_ATTENUATION, 1.0f);
                        glLightf(lid, GL_LINEAR_ATTENUATION,   0.0f);
                        glLightf(lid, GL_QUADRATIC_ATTENUATION, 0.0f);
                    }
                }
                current_light_model = 0;
                glDisable(GL_LIGHTING);
            } 
            if (index > 0 && index < MAX_MATERIALS && light_models[index].defined) {
                LightModelDef *lm = &light_models[index];
                current_light_model = index;
                glEnable(GL_LIGHTING);
                glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lm->ambient);
                glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, lm->local_viewer ? GL_TRUE : GL_FALSE);

                for (int li = 0; li < MAX_LIGHTS; ++li) {
                    GLenum lid = GL_LIGHT0 + li;
                    if (glIsEnabled(lid)) {
                        glLightf(lid, GL_CONSTANT_ATTENUATION, lm->attenuation[0]);
                        glLightf(lid, GL_LINEAR_ATTENUATION,   lm->attenuation[1]);
                        glLightf(lid, GL_QUADRATIC_ATTENUATION, 0.0f);
                    }
                }
            }
            break;
        }
        default:
            fprintf(stderr, "lmbind: unknown target %d\n", target);
            break;
    }
    fflush(stdout);
}

void lmcolor(int mode) {
    // Color material mode - stub
    if (mode == 1) { // LMC_COLOR
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    } else {
        glDisable(GL_COLOR_MATERIAL);
    }
}

// === Font Management ===

// Simple font rendering using GLUT bitmap fonts
static void* glut_fonts[] = {
    GLUT_BITMAP_8_BY_13,
    GLUT_BITMAP_9_BY_15,
    GLUT_BITMAP_TIMES_ROMAN_10,
    GLUT_BITMAP_TIMES_ROMAN_24,
    GLUT_BITMAP_HELVETICA_10,
    GLUT_BITMAP_HELVETICA_12,
    GLUT_BITMAP_HELVETICA_18
};

#define NUM_GLUT_FONTS 7
#define DEFAULT_FONT 0

void fminit(void) {
    // Initialize font system - nothing needed for GLUT fonts
    current_font = glut_fonts[DEFAULT_FONT];
}

fmfonthandle fmfindfont(const char *fontname) {
    // Map font names to GLUT fonts
    if (strstr(fontname, "times") || strstr(fontname, "Times")) {
        if (strstr(fontname, "24")) return glut_fonts[3];
        return glut_fonts[2];
    }
    if (strstr(fontname, "helvetica") || strstr(fontname, "Helvetica")) {
        if (strstr(fontname, "18")) return glut_fonts[6];
        if (strstr(fontname, "12")) return glut_fonts[5];
        return glut_fonts[4];
    }
    return glut_fonts[DEFAULT_FONT];
}

void fmsetfont(fmfonthandle font) {
    if (font != NULL) {
        current_font = font;
    }
}

void cmov(Coord x, Coord y, Coord z) {
    // Appliquer les transformations pour obtenir la vraie position raster
    GLdouble model[16], proj[16];
    GLint viewport[4];
    GLdouble winX, winY, winZ;
    
    // Récupérer les matrices et viewport actuels
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    if (gluProject(x, y, z, model, proj, viewport, &winX, &winY, &winZ) == GL_TRUE) {
        current_raster_x = (float)winX;
        current_raster_y = (float)winY;
        current_raster_z = (float)winZ;
    } else {
        current_raster_x = 0.0f;
        current_raster_y = 0.0f;
        current_raster_z = 0.0f;
    }

    glRasterPos3f(x, y, z);
}

void cmov2(Coord x, Coord y) {
    cmov(x, y, 0.1f);
}
void cmov2i(Icoord x, Icoord y)
{
    // Position du texte en coordonnées entières (fenêtre)
    glRasterPos2i(x, y);

    GLdouble model[16], proj[16];
    GLint viewport[4];
    GLdouble winX, winY, winZ;
    
    // Récupérer les matrices et viewport actuels
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    if (gluProject(x, y, 0, model, proj, viewport, &winX, &winY, &winZ) == GL_TRUE) {
        current_raster_x = (float)winX;
        current_raster_y = (float)winY;
        current_raster_z = (float)winZ;
    } else {
        current_raster_x = 0.0f;
        current_raster_y = 0.0f;
        current_raster_z = 0.0f;
    }
}
void debug_opengl_state(void) {
    printf("=== DEBUG OPENGL STATE ===\n");
    
    // Vérifier la matrice de projection
    GLfloat proj[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    printf("Projection matrix: [%.2f %.2f %.2f %.2f]\n", proj[0], proj[5], proj[10], proj[15]);
    
    // Vérifier le viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    printf("Viewport: %d %d %d %d\n", viewport[0], viewport[1], viewport[2], viewport[3]);
    
    // Vérifier les états OpenGL
    printf("Depth test: %s\n", glIsEnabled(GL_DEPTH_TEST) ? "ON" : "OFF");
    printf("Culling: %s\n", glIsEnabled(GL_CULL_FACE) ? "ON" : "OFF");
    printf("Lighting: %s\n", glIsEnabled(GL_LIGHTING) ? "ON" : "OFF");
    
    // Vérifier la couleur actuelle
    GLfloat color[4];
    glGetFloatv(GL_CURRENT_COLOR, color);
    printf("Current color: %.2f %.2f %.2f %.2f\n", color[0], color[1], color[2], color[3]);
    
    // Vérifier les erreurs OpenGL
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        printf("OpenGL ERROR: %d\n", error);
    }
}
void fmprstr(const char *str) {
    if (current_font == NULL) {
        current_font = glut_fonts[DEFAULT_FONT];
    }
    
    if (!str) return;
    
    // Vérifier si current_font est un ScaledFont
    ScaledFont* sf = NULL;
    if (is_scaled_font(current_font, &sf)) {
        float scale = sf->scale;
        void* actual_font = sf->base_font;
        int is_stroke = sf->is_stroke;
        
        if (is_stroke) {
            glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
            glEnable(GL_LINE_SMOOTH);
            
            int window_width = glutGet(GLUT_WINDOW_WIDTH);
            int window_height = glutGet(GLUT_WINDOW_HEIGHT);
            
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            glOrtho(0, window_width, 0, window_height, -1, 1);
            
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();
            
            glTranslatef(current_raster_x, current_raster_y, current_raster_z);
            
            float stroke_scale = scale * 0.0032f; // Les polices GLUT stroke sont à l'échelle ~119 unités
            glScalef(stroke_scale, stroke_scale, 1.0f);
            float advance = 0.0f;
            for (const char *c = str; *c != '\0'; c++) {
                glutStrokeCharacter(actual_font, *c);
                advance += glutStrokeWidth(actual_font, *c) * stroke_scale;
            }
            current_raster_x += advance;
            // Restaurer les matrices
            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
            glDisable(GL_LINE_SMOOTH);
            glPopAttrib();
        } else {
            for (const char *c = str; *c != '\0'; c++) {
                glutBitmapCharacter(actual_font, *c);
            }
        }
    } else {
        // Font normal sans scaling
        for (const char *c = str; *c != '\0'; c++) {
            glutBitmapCharacter(current_font, *c);
        }
    }
}

// === Window Management ===

static int main_window = 0;
static int window_width = 800;
static int window_height = 600;
static int window_x = 100;
static int window_y = 100;

// Forward declarations for GLUT callbacks
static void iris_display_func(void);
static void iris_idle_func(void);
void iris_keyboard_func(unsigned char key, int x, int y);
void iris_keyboard_up_func(unsigned char key, int x, int y);
void iris_special_func(int key, int x, int y);
void iris_special_up_func(int key, int x, int y);
void iris_mouse_func(int button, int state, int x, int y);
void iris_motion_func(int x, int y);

void winopen(const char *title) {
    // Initialize GLUT if not already done
    static int glut_initialized = 0;
    if (!glut_initialized) {
        int argc = 1;
        char *argv[1] = {(char *)"iris_gl_emulator"};
        glutInit(&argc, argv);
        glut_initialized = 1;
    }
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(window_x, window_y);
    main_window = glutCreateWindow(title);
    
    //   GLUT callbacks
    glutDisplayFunc(iris_display_func);
    glutIdleFunc(iris_idle_func);  // Keep processing events
    glutKeyboardFunc(iris_keyboard_func);
    glutKeyboardUpFunc(iris_keyboard_up_func);
    glutSpecialFunc(iris_special_func);
    glutSpecialUpFunc(iris_special_up_func);
    glutMouseFunc(iris_mouse_func);
    glutMotionFunc(iris_motion_func);
    glutPassiveMotionFunc(iris_motion_func);
    
    // Initialize OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (queued_devices[REDRAW]) {
        queue_event(REDRAW, 1);
    }
}

void winclose(int win) {
    glutDestroyWindow(win);
}

int winget(void)
{
    // Comportement proche de IRIS GL : fenêtre courante si existante
    int win = glutGetWindow();
    if (win != 0) {
        return win;
    }
    // Sinon, retourner la fenêtre principale si on la connaît
    return main_window;
}

void winposition(int x, int y, int width, int height) {
    window_x = x;
    window_y = y;
    window_width = width;
    window_height = height;
    if (main_window != 0) {
        glutPositionWindow(x, y);
        glutReshapeWindow(width, height);
    }
}
void wintitle(const char *title)
{
    if (!title) return;
    if (main_window != 0) {
        glutSetWindow(main_window);
        glutSetWindowTitle(title);
    }
}
void getsize(int *width, int *height) {
    if (main_window != 0) {
        *width = glutGet(GLUT_WINDOW_WIDTH);
        *height = glutGet(GLUT_WINDOW_HEIGHT);
    } else {
        *width = window_width;
        *height = window_height;
    }
}

void getorigin(int *x, int *y) {
    if (main_window != 0) {
        *x = glutGet(GLUT_WINDOW_X);
        *y = glutGet(GLUT_WINDOW_Y);
    } else {
        *x = window_x;
        *y = window_y;
    }
}

void reshapeviewport(void) {
    int width, height;
    getsize(&width, &height);
    glViewport(0, 0, width, height);
}

void keepaspect(int x, int y) {
    // GLUT doesn't support aspect ratio locking directly
    // This is a no-op in this implementation
}

void prefposition(int x1, int x2, int y1, int y2) {
    window_x = x1;
    window_y = y1;
    window_width = x2 - x1;
    window_height = y2 - y1;
}

void prefsize(int width, int height) {
    window_width = width;
    window_height = height;
}

void maxsize(int width, int height) {
    // GLUT doesn't support max size constraints
}

void minsize(int width, int height) {
    // GLUT doesn't support min size constraints
}

void stepunit(int x, int y) {
    // Window resize step - not supported in GLUT
}

void foreground(void) {
    // Bring window to foreground - limited in GLUT
    if (main_window != 0) {
        glutSetWindow(main_window);
    }
}

void gconfig(void) {
    // Graphics configuration - most settings done in glutInitDisplayMode
}

void RGBmode(void) {
    // Set RGB mode - done by default in glutInitDisplayMode
}

void cmode(void) {
    // Set color index mode - we simulate it with RGB + palette
    // Nothing special to do, iris_set_color_index() handles it
}

void doublebuffer(void) {
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
}

// Note: set_win_coords() is defined in draw.c (game-specific implementation)

void frontbuffer(Boolean enable) {
    /*if (enable) {
        glDrawBuffer(GL_FRONT);
    }*/
    glDrawBuffer(GL_BACK);
}

void backbuffer(Boolean enable) {
    /*if (enable) {
        glDrawBuffer(GL_BACK);
    }*/
    glDrawBuffer(GL_BACK);
}

// === Device/Event Management ===

static void queue_event(Device dev, int16_t val) {
    int next = (event_queue_tail + 1) % EVENT_QUEUE_SIZE;
    if (next != event_queue_head) {
        event_queue[event_queue_tail].device = dev;
        event_queue[event_queue_tail].value = val;
        event_queue_tail = next;
    }
}
void qenter(Device dev, int16_t val) {
    // IRIS GL ne nécessite pas que le device soit "qdevice"‑é pour qenter,
    // mais on respecte la même file pour cohérence.
    queue_event(dev, val);
}
static unsigned long last_redraw_time = 0;

#ifdef _WIN32
static unsigned long get_time_ms(void) {
    return GetTickCount();
}
#else
#include <sys/time.h>
static unsigned long get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
#endif
void qdevice(Device dev) {
    if (dev >= 0 && dev < 256) {
        queued_devices[dev] = 1;
    }
}

void unqdevice(Device dev) {
    if (dev >= 0 && dev < 256) {
        queued_devices[dev] = 0;
    }
}

Boolean qtest(void) {
    /* 1) Si la file contient déjà des événements, ne touche pas à GLUT */
    if (event_queue_head != event_queue_tail) {
        return TRUE;
    }

    /* 3) Sinon, on laisse GLUT traiter au plus une vague d'événements */
    glutMainLoopEvent();

    /* 4) Re-tester la file IRIS */
    if (event_queue_head != event_queue_tail) {
        return TRUE;
    } else {
        return FALSE;
    }
}


int32_t qread(int16_t *val) {

    for (;;) {
        // Si on a quelque chose dans la file, on le renvoie tout de suite
        if (event_queue_head != event_queue_tail) {
            Event evt = event_queue[event_queue_head];
            event_queue_head = (event_queue_head + 1) % EVENT_QUEUE_SIZE;
            *val = evt.value;
            return evt.device;
        }

        // Sinon, on laisse GLUT produire des événements UNE fois
        glutMainLoopEvent();

        // Gestion fenêtre fermée: pas d'event → retourne 0
        if (glutGetWindow() == 0) {
            *val = 0;
            return 0;
        }

        // Petit délai pour éviter 100% CPU si pas d'événements
        #ifdef _WIN32
        Sleep(1);
        #else
        usleep(1000);
        #endif
    }
}

Boolean getbutton(Device dev) {
    switch (dev) {
        case LEFTMOUSE:
            return mouse_state[0];
        case MIDDLEMOUSE:
            return mouse_state[1];
        case RIGHTMOUSE:
            return mouse_state[2];
        default:
            if (dev >= 0 && dev < 256) {
                return key_state[dev];
            }
            return FALSE;
    }
}

// GLUT callback helpers to populate event queue
void iris_keyboard_func(unsigned char key, int x, int y) {
    Device dev = 0;
    
    // Map special keys to their device codes
    switch (key) {
        case 27: dev = ESCKEY; break;
        case 13: dev = RETKEY; break;
        case ' ': dev = SPACEKEY; break;
        case 'h': case 'H': dev = HKEY; break;
        case 'a': case 'A': dev = AKEY; break;
        default:
            dev = 0;  // Not a special key
            break;
    }
    
    // If it's a mapped special key and queued, send it
    if (dev != 0 && queued_devices[dev]) {
        queue_event(dev, 1);
        key_state[dev] = TRUE;
    }
    
    // Also send to KEYBD device if it's queued (for all keys)
    if (queued_devices[KEYBD]) {
        queue_event(KEYBD, (int16_t)key);
    }
}

void iris_keyboard_up_func(unsigned char key, int x, int y) {
    Device dev = 0;
    
    // Map special keys to their device codes
    switch (key) {
        case 27: dev = ESCKEY; break;
        case 13: dev = RETKEY; break;
        case ' ': dev = SPACEKEY; break;
        case 'h': case 'H': dev = HKEY; break;
        case 'a': case 'A': dev = AKEY; break;
        default:
            dev = 0;  // Not a special key
            break;
    }
    
    // If it's a mapped special key and queued, send release event
    if (dev != 0 && queued_devices[dev]) {
        queue_event(dev, 0);
        key_state[dev] = FALSE;
    }
    
    // Note: KEYBD typically only sends key press, not release
}

void iris_special_func(int key, int x, int y) {
    Device dev = 0;
    
    switch (key) {
        case GLUT_KEY_LEFT: dev = LEFTARROWKEY; break;
        case GLUT_KEY_RIGHT: dev = RIGHTARROWKEY; break;
        case GLUT_KEY_UP: dev = UPARROWKEY; break;
        case GLUT_KEY_DOWN: dev = DOWNARROWKEY; break;
        default: return;
    }
    
    if (queued_devices[dev]) {
        queue_event(dev, 1);
        key_state[dev] = TRUE;
    }
}

void iris_special_up_func(int key, int x, int y) {
    Device dev = 0;
    
    switch (key) {
        case GLUT_KEY_LEFT: dev = LEFTARROWKEY; break;
        case GLUT_KEY_RIGHT: dev = RIGHTARROWKEY; break;
        case GLUT_KEY_UP: dev = UPARROWKEY; break;
        case GLUT_KEY_DOWN: dev = DOWNARROWKEY; break;
        default: return;
    }
    
    if (queued_devices[dev]) {
        queue_event(dev, 0);
        key_state[dev] = FALSE;
    }
}

void iris_mouse_func(int button, int state, int x, int y) {
    Device dev = 0;
    Boolean pressed = (state == GLUT_DOWN);
    switch (button) {
        case GLUT_LEFT_BUTTON:
            dev = LEFTMOUSE;
            mouse_state[0] = pressed;
            break;
        case GLUT_MIDDLE_BUTTON:
            dev = MIDDLEMOUSE;
            mouse_state[1] = pressed;
            break;
        case GLUT_RIGHT_BUTTON:
            dev = RIGHTMOUSE;
            mouse_state[2] = pressed;
            break;
        default:
            return;
    }
    
    if (queued_devices[dev]) {
        queue_event(dev, pressed ? 1 : 0);
    }
}

// Track mouse position for MOUSEX/MOUSEY
static int current_mouse_x = 0;
static int current_mouse_y = 0;

int iris_get_mouse_x(void) {
    return current_mouse_x;
}

int iris_get_mouse_y(void) {
    return current_mouse_y;
}

void iris_motion_func(int x, int y) {
    current_mouse_x = x;
    current_mouse_y = window_height - y;
    
    if (queued_devices[MOUSEX]) {
        queue_event(MOUSEX, current_mouse_x);
    }
    if (queued_devices[MOUSEY]) {
        queue_event(MOUSEY, current_mouse_y);
    }
}

// === Spaceball simulated state (valuators SBTX..SBPERIOD) ===
static float iris_sb_tx = 0.0f;
static float iris_sb_ty = 0.0f;
static float iris_sb_tz = 0.0f;
static float iris_sb_rx = 0.0f;
static float iris_sb_ry = 0.0f;
static float iris_sb_rz = 0.0f;
static int   iris_sb_period_ms = 16; // ~60 Hz

float iris_get_spaceball_tx(void) { return iris_sb_tx; }
float iris_get_spaceball_ty(void) { return iris_sb_ty; }
float iris_get_spaceball_tz(void) { return iris_sb_tz; }
float iris_get_spaceball_rx(void) { return iris_sb_rx; }
float iris_get_spaceball_ry(void) { return iris_sb_ry; }
float iris_get_spaceball_rz(void) { return iris_sb_rz; }
int   iris_get_spaceball_period_ms(void) { return iris_sb_period_ms; }

// Utilitaire: à appeler depuis le portage (par ex. dans un callback clavier)
void iris_spaceball_update(float dtx, float dty, float dtz,
                           float drx, float dry, float drz)
{
    iris_sb_tx += dtx;
    iris_sb_ty += dty;
    iris_sb_tz += dtz;
    iris_sb_rx += drx;
    iris_sb_ry += dry;
    iris_sb_rz += drz;

    // Si un jour tu veux générer aussi des événements qread() pour SBTX...
    if (queued_devices[SBTX])    queue_event(SBTX,    (int16_t)(iris_sb_tx * 1000.0f));
    if (queued_devices[SBTY])    queue_event(SBTY,    (int16_t)(iris_sb_ty * 1000.0f));
    if (queued_devices[SBTZ])    queue_event(SBTZ,    (int16_t)(iris_sb_tz * 1000.0f));
    if (queued_devices[SBRX])    queue_event(SBRX,    (int16_t)(iris_sb_rx * 1000.0f));
    if (queued_devices[SBRY])    queue_event(SBRY,    (int16_t)(iris_sb_ry * 1000.0f));
    if (queued_devices[SBRZ])    queue_event(SBRZ,    (int16_t)(iris_sb_rz * 1000.0f));
    if (queued_devices[SBPERIOD]) queue_event(SBPERIOD, (int16_t)iris_sb_period_ms);
}


static void iris_display_func(void) {
    // Generate REDRAW event if it's being listened to
    if (queued_devices[REDRAW]) {
        queue_event(REDRAW, 1);
    }
    // Don't actually draw here - let the application handle it
    // The application will call swapbuffers() when ready
}

static void iris_idle_func(void) {
    // Idle callback - keeps GLUT processing events
    // This allows qread() to receive events even when blocked
    #ifdef _WIN32
    Sleep(1);
    #else
    usleep(1000);
    #endif
}

void iris_reshape_func(int width, int height) {
    if (queued_devices[REDRAW]) {
        queue_event(REDRAW, 1);
    }
}

void iris_entry_func(int state) {
    if (queued_devices[INPUTCHANGE]) {
        queue_event(INPUTCHANGE, state == GLUT_ENTERED ? 1 : 0);
    }
}

// Helper to setup all GLUT callbacks
void iris_setup_glut_callbacks(void) {
    glutKeyboardFunc(iris_keyboard_func);
    glutKeyboardUpFunc(iris_keyboard_up_func);
    glutSpecialFunc(iris_special_func);
    glutSpecialUpFunc(iris_special_up_func);
    glutMouseFunc(iris_mouse_func);
    glutReshapeFunc(iris_reshape_func);
    glutEntryFunc(iris_entry_func);
}

// === UNIX Compatibility ===

char* iris_cuserid(char *buf) {
    static char username[256] = "Player";
    
#ifdef _WIN32
    // Windows: use GetUserName
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size)) {
        if (buf) {
            strncpy(buf, username, 255);
            buf[255] = '\0';
            return buf;
        }
        return username;
    }
#else
    // POSIX: use getenv("USER") or getlogin()
    const char *user = getenv("USER");
    if (!user) user = getenv("LOGNAME");
    if (!user) user = "Player";
    
    strncpy(username, user, 255);
    username[255] = '\0';
    
    if (buf) {
        strncpy(buf, username, 255);
        buf[255] = '\0';
        return buf;
    }
#endif
    
    return username;
}

void zfunction(int func)
{
    GLenum mode = GL_LESS; // valeur par défaut IRIS ≈ OpenGL

    switch (func) {
    case ZF_NEVER:    mode = GL_NEVER;    break;
    case ZF_LESS:     mode = GL_LESS;     break;
    case ZF_EQUAL:    mode = GL_EQUAL;    break;
    case ZF_LEQUAL:   mode = GL_LEQUAL;   break;
    case ZF_GREATER:  mode = GL_GREATER;  break;
    case ZF_NOTEQUAL: mode = GL_NOTEQUAL; break;
    case ZF_GEQUAL:   mode = GL_GEQUAL;   break;  // <-- mapping direct
    case ZF_ALWAYS:   mode = GL_ALWAYS;   break;
    default:
        // valeur inconnue -> garder le mode courant
        return;
    }

    glDepthFunc(mode);
}

void czclear(unsigned long cval, unsigned long zval)
{

    uint32_t color = (uint32_t)cval;
    float b = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float r = (color & 0xFF) / 255.0f;
    
    iris_clear_color[0] = r;
    iris_clear_color[1] = g;
    iris_clear_color[2] = b;
    iris_clear_color[3] = 1.0f;

    // Profondeur : en IRIS GL zval est une valeur entière entre GD_ZMIN et GD_ZMAX.
    // Dans ton port, tu as fixé la plage logique à [0.0, 1.0], on mappe donc :
    if (zval == (unsigned long)getgdesc(GD_ZMAX)) {
        glClearDepth(1.0);
    } else if (zval == (unsigned long)getgdesc(GD_ZMIN)) {
        glClearDepth(0.0);
    } else {
        // mapping linéaire si jamais quelqu’un passe une autre valeur
        double zmin = (double)getgdesc(GD_ZMIN);
        double zmax = (double)getgdesc(GD_ZMAX);
        double norm = (zval - zmin) / (zmax - zmin);
        if (norm < 0.0) norm = 0.0;
        if (norm > 1.0) norm = 1.0;
        glClearDepth(norm);
    }
    glClear(GL_DEPTH_BUFFER_BIT);
    clear();
}

void smoothline(Boolean on)
{
    if (on) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        // Optionnel: pour une meilleure qualité
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    } else {
        glDisable(GL_LINE_SMOOTH);
        // Ne pas forcément couper le blend globalement si ton app l’utilise ailleurs
        // glDisable(GL_BLEND);
    }
}

void subpixel(Boolean on)
{
    // IRIS GL: active/désactive le subpixel rendering.
    // OpenGL moderne travaille déjà en flottants, donc on ne fait rien ici.
    (void)on;
}

char *gversion(char *machinetype)
{
    // Chaîne au format libre, juste pour satisfaire les vieux codes IRIS GL
    static char ver[] = "IRIS2OGL 1.0 (OpenGL/GLUT compatibility layer)";
    return ver;
}
static unsigned long iris48_seed = 1UL;
void srand48(long seedval)
{
    // version simple : réutilise srand pour rester cohérent
    iris48_seed = (unsigned long)seedval;
    srand((unsigned int)(seedval ^ (seedval >> 16)));
}

double drand48(void)
{
    // Retourne un double dans [0.0, 1.0)
    // basé sur rand() (RAND_MAX typiquement 32767 ou plus)
    int r = rand();
    return (double)r / ((double)RAND_MAX + 1.0);
}

static PupMenu pup_menus[MAX_PUP_MENUS];
static int     last_menu_choice = -1;

// Callback global GLUT -> route vers notre PupMenu
static void iris_pup_menu_callback(int value)
{
    last_menu_choice = value;
    int glut_id = glutGetMenu();
    for (int i = 0; i < MAX_PUP_MENUS; ++i) {
        if (pup_menus[i].used && pup_menus[i].glut_menu_id == glut_id) {
            PupMenu *pm = &pup_menus[i];
            int idx = value - 1;
            if (idx < 0 || idx >= pm->item_count) return;

            PupItem *it = &pm->items[idx];

            if (it->has_F && it->callback_int) {
                it->callback_int(it->value);   // ex: toggle_alpha(n)
            } else if (it->has_f && it->callback_void) {
                it->callback_void();           // ex: ui_exit()
            } else if (pm->has_F && pm->callback_int) {
                pm->callback_int(it->value);
            } else if (pm->has_f && pm->callback_void) {
                pm->callback_void();
            }
            return;
        }
    }
}

// Création d'un nouveau menu IRISGL
Menu newpup(void)
{
    for (int i = 0; i < MAX_PUP_MENUS; ++i) {
        if (!pup_menus[i].used) {
            PupMenu *pm = &pup_menus[i];
            memset(pm, 0, sizeof(PupMenu));
            pm->used = 1;
            pm->glut_menu_id = glutCreateMenu(iris_pup_menu_callback);
            return (Menu)i;
        }
    }
    return -1;
}

// Parsing très simple des suffixes IRISGL dans 'label':
//   "%t"   -> titre (ignoré ici)
//   "%m"   -> sous-menu, le Menu est passé en varargs
//   "%F"   -> ce menu a une fonction callback(int)
//   "%f"   -> ce menu a une fonction callback(void)
//   "%xNN" -> valeur numérique associée à l'item
#include <stdarg.h>

Menu addtopup(Menu m, const char *label, ...)
{
    if (m < 0 || m >= MAX_PUP_MENUS || !label) return m;
    PupMenu *pm = &pup_menus[m];
    if (!pm->used) return m;

    va_list ap;
    va_start(ap, label);

    // Copie du label original pour extraire la partie affichable (avant les %)
    char text[128];
    strncpy(text, label, sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';

    // Recherche du premier '%' qui marque le début des codes
    char *percent = strchr(text, '%');
    if (percent) {
        *percent = '\0';  // garde seulement la partie "visible"
    }

    int is_submenu = 0;
    int value_x = 0;
    PupFunc1 cb_int = NULL;
    PupFunc0 cb_void = NULL;
    int has_F = 0;
    int has_f = 0;

    // Parse les codes dans la chaîne originale (label, pas text)
    const char *p = label;
    while ((p = strchr(p, '%')) != NULL) {
        p++; // passer le '%'
        if (*p == '%') {
            // "%%" → un '%' littéral dans le texte, rien de spécial
            p++;
            continue;
        }
        switch (*p) {
        case 't':
            // titre -> ignoré
            p++;
            break;
        case 'm': {
            // sous-menu: le Menu est passé en varargs
            Menu sub = va_arg(ap, Menu);
            glutSetMenu(pm->glut_menu_id);
            glutAddSubMenu(text, pup_menus[sub].glut_menu_id);
            is_submenu = 1;
            p++;
            break;
        }
        case 'F':
            // fonction f(int)
            cb_int = va_arg(ap, PupFunc1);
            has_F = 1;
            p++;
            break;
        case 'f':
            // fonction f(void)
            cb_void = va_arg(ap, PupFunc0);
            has_f = 1;
            p++;
            break;
        case 'x': {
            // %xNN -> lit un entier dans la suite de la chaîne
            int v = 0;
            p++;
            sscanf(p, "%d", &v);
            value_x = v;
            // avancer p jusqu'à la fin du nombre
            while (*p && (*p == '-' || (*p >= '0' && *p <= '9')))
                p++;
            break;
        }
        default:
            p++;
            break;
        }
    }

    // Si c'était un sous-menu, on a déjà fait glutAddSubMenu, on sort
    if (is_submenu) {
        // En IRISGL, addtopup retourne le même handle
        va_end(ap);
        return m;
    }

    // Enregistrer les callbacks au niveau du menu
    if (has_F) {
        pm->callback_int = cb_int;
        pm->has_F = 1;
    }
    if (has_f) {
        pm->callback_void = cb_void;
        pm->has_f = 1;
    }

    // Créer un nouvel item
    if (pm->item_count >= MAX_PUP_ITEMS) {
        va_end(ap);
        return m;
    }
    PupItem *it = &pm->items[pm->item_count];
    memset(it, 0, sizeof(PupItem));
    strncpy(it->label, text, sizeof(it->label) - 1);
    it->label[sizeof(it->label) - 1] = '\0';
    it->value = value_x;
    it->flags = 0;
    if (has_F) {
        it->callback_int = cb_int;
        it->has_F = 1;
    }
    if (has_f) {
        it->callback_void = cb_void;
        it->has_f = 1;
    }

    // Ajout dans GLUT
    glutSetMenu(pm->glut_menu_id);
    // On utilise l'indice de l'item (1..n) comme 'value' passé au callback global
    glutAddMenuEntry(it->label, pm->item_count + 1);

    pm->item_count++;

    va_end(ap);
    return m;
}

void setpup(Menu m, int item, int flags)
{
    if (m < 0 || m >= MAX_PUP_MENUS) return;
    PupMenu *pm = &pup_menus[m];
    if (!pm->used) return;
    if (item < 1 || item > pm->item_count) return;

    pm->items[item - 1].flags = flags;
    // GLUT ne permet pas de modifier le label après coup de façon portable,
    // donc on garde les flags en logique interne (Flip ne dépend que de l’état).
}

int dopup(Menu m)
{
    if (m < 0 || m >= MAX_PUP_MENUS) return -1;
    PupMenu *pm = &pup_menus[m];
    if (!pm->used || pm->item_count == 0) return -1;

    glutSetWindow(main_window);
    glutSetMenu(pm->glut_menu_id);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    last_menu_choice = -1;

    // Boucle "bloquante" IRISGL-style: on pompe les events jusqu’à un choix
    while (last_menu_choice < 0) {
        glutMainLoopEvent(); // freeglut
        // tu peux insérer un petit Sleep(1) / usleep ici si besoin
    }

    int choice = last_menu_choice;
    last_menu_choice = -1;
    glutDetachMenu(GLUT_RIGHT_BUTTON);
    return choice;   // index d’item (1..n). Flip ne l’utilise pas directement.
}

void freepup(Menu m)
{
    if (m < 0 || m >= MAX_PUP_MENUS) return;
    PupMenu *pm = &pup_menus[m];
    if (!pm->used) return;

    if (pm->glut_menu_id != 0) {
        glutDestroyMenu(pm->glut_menu_id);
        pm->glut_menu_id = 0;
    }
    memset(pm, 0, sizeof(PupMenu));
}

static double iris_depth_to_norm(unsigned long zval)
{
    double zmin = (double)getgdesc(GD_ZMIN);
    double zmax = (double)getgdesc(GD_ZMAX);

    if (zmax == zmin) return 1.0;
    double norm = (zval - zmin) / (zmax - zmin);
    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;
    return norm;
}

void setdepth(unsigned long znear, unsigned long zfar)
{
    // IRIS GL: définit la plage de profondeur pour le depth test
    double n = iris_depth_to_norm(znear);
    double f = iris_depth_to_norm(zfar);
    glDepthRange(n, f);
}

void lsetdepth(unsigned long znear, unsigned long zfar)
{
    // Beaucoup d’exemples IRIS utilisent lsetdepth pour l’inversion
    // de la plage, combinée à zfunction. Ici on le mappe comme setdepth.
    double n = iris_depth_to_norm(znear);
    double f = iris_depth_to_norm(zfar);
    glDepthRange(n, f);
}

void gexit(void)
{
    // Tentative de nettoyage GLUT :
    // - détruire la fenêtre si encore présente
    int win = glutGetWindow();
    if (win != 0) {
        glutDestroyWindow(win);
    }

    // Si le code IRISGL appelle gexit(), il attend généralement
    // une terminaison du programme.
    // On laisse un exit explicite côté portage.
    exit(0);
}

// === Additional IRIS GL implementations ===

// Internal state variables


// 2D drawing functions - 3D versions
void move(Coord x, Coord y, Coord z) {
    glBegin(GL_LINES);
    glVertex3f(x, y, z);
}

void draw(Coord x, Coord y, Coord z) {
    glVertex3f(x, y, z);
    glEnd();
}

// 2D drawing functions - float versions
void move2(Coord x, Coord y) {
    glBegin(GL_LINES);
    glVertex2f(x, y);
}

void draw2(Coord x, Coord y) {
    glVertex2f(x, y);
    glEnd();
}

// 2D drawing functions - integer versions
void move2i(Icoord x, Icoord y) {
    glBegin(GL_LINES);
    glVertex2i(x, y);
}

void draw2i(Icoord x, Icoord y) {
    glVertex2i(x, y);
    glEnd();
}

// 2D drawing functions - short versions
void move2s(Scoord x, Scoord y) {
    glBegin(GL_LINES);
    glVertex2s(x, y);
}

void draw2s(Scoord x, Scoord y) {
    glVertex2s(x, y);
    glEnd();
}

// Character move (raster position) - short version
void cmov2s(Scoord x, Scoord y) {
    glRasterPos2s(x, y);
    current_raster_x = (float)x;
    current_raster_y = (float)y;
}

// Point drawing
void pnt2(Coord x, Coord y) {
    glBegin(GL_POINTS);
    glVertex2f(x, y);
    glEnd();
}

void pnt2s(Scoord x, Scoord y) {
    glBegin(GL_POINTS);
    glVertex2s(x, y);
    glEnd();
}

// 2D polygon
void poly2(int32_t n, Coord parray[][2]) {
    glBegin(GL_POLYGON);
    for (int32_t i = 0; i < n; i++) {
        glVertex2f(parray[i][0], parray[i][1]);
    }
    glEnd();
}

// Rectangle functions
void recti(Icoord x1, Icoord y1, Icoord x2, Icoord y2) {
    glBegin(GL_LINE_LOOP);
    glVertex2i(x1, y1);
    glVertex2i(x2, y1);
    glVertex2i(x2, y2);
    glVertex2i(x1, y2);
    glEnd();
}

void rectfi(Icoord x1, Icoord y1, Icoord x2, Icoord y2) {
    glRecti(x1, y1, x2, y2);
}

void rects(Scoord x1, Scoord y1, Scoord x2, Scoord y2) {
    glBegin(GL_LINE_LOOP);
    glVertex2s(x1, y1);
    glVertex2s(x2, y1);
    glVertex2s(x2, y2);
    glVertex2s(x1, y2);
    glEnd();
}

// Filled circle - short coordinates
void circfs(Scoord x, Scoord y, Scoord radius) {
    int segments = 32;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2s(x, y);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex2f(x + radius * cos(angle), y + radius * sin(angle));
    }
    glEnd();
}

// Text functions
int strwidth(const char* str) {
    if (!str) return 0;
    int width = 0;
    void* font = GLUT_BITMAP_9_BY_15;
    for (const char* p = str; *p; p++) {
        width += glutBitmapWidth(font, *p);
    }
    return width;
}

int getheight(void) {
    return 15; // Height of GLUT_BITMAP_9_BY_15
}

int getdescender(void) {
    return 3; // Approximate descender height
}

void getcpos(Scoord* x, Scoord* y) {
    if (x) *x = (Scoord)current_raster_x;
    if (y) *y = (Scoord)current_raster_y;
}

// Viewport stack
#define MAX_VIEWPORT_STACK 256
static Screencoord viewport_stack[MAX_VIEWPORT_STACK][4];
static int viewport_stack_depth = 0;
static Screencoord current_scrmask[4] = {0, 0, 0, 0};
static int scrmask_enabled = 0;

void pushviewport(void) {
    if (viewport_stack_depth < MAX_VIEWPORT_STACK) {
        for (int i = 0; i < 4; i++) {
            viewport_stack[viewport_stack_depth][i] = current_viewport[i];
        }
        viewport_stack_depth++;
    }
}

void popviewport(void) {
    if (viewport_stack_depth > 0) {
        viewport_stack_depth--;
        for (int i = 0; i < 4; i++) {
            current_viewport[i] = viewport_stack[viewport_stack_depth][i];
        }
        glViewport(current_viewport[0], current_viewport[2], 
                   current_viewport[1] - current_viewport[0], 
                   current_viewport[3] - current_viewport[2]);
    }
}

void getviewport(Screencoord* left, Screencoord* right, Screencoord* bottom, Screencoord* top) {
    if (left) *left = current_viewport[0];
    if (right) *right = current_viewport[1];
    if (bottom) *bottom = current_viewport[2];
    if (top) *top = current_viewport[3];
}

void scrmask(Screencoord left, Screencoord right,
             Screencoord bottom, Screencoord top)
{
    current_scrmask[0] = left;
    current_scrmask[1] = right;
    current_scrmask[2] = bottom;
    current_scrmask[3] = top;

    // Largeur / hauteur en coord. fenêtre
    Screencoord w = right  - left;
    Screencoord h = top    - bottom;
    if (w <= 0 || h <= 0) {
        glDisable(GL_SCISSOR_TEST);
        scrmask_enabled = 0;
        return;
    }

    int win_w, win_h;
    getsize(&win_w, &win_h);

    // Cas "plein écran" (comme IRIS) → désactive le masque
    if (left == 0 && bottom == 0 &&
        right == win_w && top == win_h) {
        glDisable(GL_SCISSOR_TEST);
        scrmask_enabled = 0;
        return;
    }

    // Ici on considère left/right/bottom/top déjà en coords fenêtre
    GLint sc_x = left;
    GLint sc_y = bottom;
    GLsizei sc_w = w;
    GLsizei sc_h = h;

    // Clamp au cas où ça dépasse la fenêtre (valeurs négatives etc.)
    if (sc_x < 0) {
        sc_w += sc_x;      // réduit la largeur
        sc_x  = 0;
    }
    if (sc_y < 0) {
        sc_h += sc_y;
        sc_y  = 0;
    }
    if (sc_x + sc_w > win_w) sc_w = win_w - sc_x;
    if (sc_y + sc_h > win_h) sc_h = win_h - sc_y;

    if (sc_w <= 0 || sc_h <= 0) {
        glDisable(GL_SCISSOR_TEST);
        scrmask_enabled = 0;
        return;
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(sc_x, sc_y, sc_w, sc_h);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, win_w, 0, win_h);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1, 1, 0); // jaune
    glBegin(GL_LINE_LOOP);
        glVertex2i(sc_x,         sc_y);
        glVertex2i(sc_x+sc_w,    sc_y);
        glVertex2i(sc_x+sc_w,    sc_y+sc_h);
        glVertex2i(sc_x,         sc_y+sc_h);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    scrmask_enabled = 1;
}


// Drawing mode function
void drawmode(int mode) {
    /*current_drawmode = mode;

    switch (mode) {
    case NORMALDRAW:
    case UNDERDRAW:
        // Rendu "normal" : Z actif
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        break;

    case OVERDRAW:
    case PUPDRAW:
        // Overlays / PUP : par-dessus, sans interaction Z
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        break;

    default:
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        break;
    }
    */
}
// Viewport function
void viewport(Screencoord left, Screencoord right, Screencoord bottom, Screencoord top) {
    current_viewport[0] = left;
    current_viewport[1] = right;
    current_viewport[2] = bottom;
    current_viewport[3] = top;
    glViewport(left, bottom, right - left, top - bottom);
}

// Line style functions
#define MAX_LINE_STYLES 16
static unsigned short line_styles[MAX_LINE_STYLES];
static int line_styles_defined[MAX_LINE_STYLES] = {0};

void setlinestyle(int style) {
    if (style == 0) {
        glDisable(GL_LINE_STIPPLE);
    } else if (style > 0 && style < MAX_LINE_STYLES && line_styles_defined[style]) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, line_styles[style]);
    }
}

void deflinestyle(int style, unsigned short pattern) {
    if (style > 0 && style < MAX_LINE_STYLES) {
        line_styles[style] = pattern;
        line_styles_defined[style] = 1;
    }
}

void linesmooth(unsigned long mode) {
    if (mode) {
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    } else {
        glDisable(GL_LINE_SMOOTH);
    }
}

// Cursor functions (stubs - GLUT doesn't support custom cursors easily)
void cursoff(void) {
    glutSetCursor(GLUT_CURSOR_NONE);
}

void curson(void) {
    glutSetCursor(GLUT_CURSOR_INHERIT);
}

void defcursor(int index, unsigned short cursor[16]) {
    // Stub - GLUT doesn't support defining custom cursors
    (void)index;
    (void)cursor;
}

void curorigin(int index, Scoord x, Scoord y) {
    // Stub - cursor origin not supported in GLUT
    (void)index;
    (void)x;
    (void)y;
}

void setcursor(int index, Colorindex color, Colorindex writemask) {
    // Stub - limited cursor support in GLUT
    (void)index;
    (void)color;
    (void)writemask;
}

// Window management functions (most are stubs)
void noborder(void) {
    // Stub - GLUT doesn't support borderless windows easily
}

void icontitle(const char* title) {
    glutSetIconTitle(title);
}

void mssize(int samples, int coverage, int z) {
    // Stub - multisampling configuration, not directly supported
    (void)samples;
    (void)coverage;
    (void)z;
}

void underlay(int planes) {
    // IRIS GL : demande des plans underlay.
    // Ici on mémorise juste l'intention : >0 => on considère
    // qu'on veut un plan "sous" la 3D/HUD.
    underlay_planes = planes;
}

void overlay(int planes) {
    // IRIS GL : demande des plans overlay.
    // On mémorise : >0 => on utilisera un mode "par-dessus tout".
    overlay_planes = planes;
}

void swapinterval(int interval) {
    // Stub - VSync control not in GLUT API
    (void)interval;
}

long getgconfig(int what) {
    // Return graphics configuration
    switch (what) {
        case GD_XPMAX: return 1024;
        case GD_YPMAX: return 768;
        case GD_BITS_NORM_DBL_CMODE: return 8;
        case GD_MULTISAMPLE: return 0;
        default: return 0;
    }
}

void greset(void) {
    // Reset graphics state - reinitialize
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

// Event queue functions
void qreset(void) {
    event_queue_head = 0;
    event_queue_tail = 0;
}

void setvaluator(Device dev, int16_t init, int16_t vmin, int16_t vmax) {
    // Stub - valuator initialization
    (void)dev;
    (void)init;
    (void)vmin;
    (void)vmax;
}

void setbell(int volume) {
    // Stub - bell volume control
    (void)volume;
}

void ringbell(void) {
    // Ring the terminal bell
#ifdef _WIN32
    Beep(750, 300);
#else
    printf("\a");
    fflush(stdout);
#endif
}

// Math functions
void gl_sincos(int angle_decidegrees, float* sinval, float* cosval) {
    // IRIS GL uses decidegrees (tenths of degrees)
    float radians = (angle_decidegrees / 10.0f) * (M_PI / 180.0f);
    if (sinval) *sinval = sinf(radians);
    if (cosval) *cosval = cosf(radians);
}

float fasin(float x) {
    return asinf(x);
}

float fcos(float x) {
    return cosf(x);
}

float fsqrt(float x) {
    return sqrtf(x);
}

float fexp(float x) {
    return expf(x);
}

// Fog functions
void fogvertex(int mode, float density[]) {
    // IRIS GL fog vertex mode
    glFogi(GL_FOG_MODE, mode == 0 ? GL_LINEAR : GL_EXP);
    if (density && mode != 0) {
        glFogf(GL_FOG_DENSITY, density[0]);
    }
}

// Texture functions (stubs - texturing needs more complete implementation)
void texdef2d(int texid, int nc, int width, int height, void* image, int np, float props[]) {
    // Stub - texture definition
    (void)texid;
    (void)nc;
    (void)width;
    (void)height;
    (void)image;
    (void)np;
    (void)props;
}

void tevdef(int texid, int nc, float props[]) {
    // Stub - texture environment definition
    (void)texid;
    (void)nc;
    (void)props;
}

void texbind(int target, int texid) {
    // Stub - texture binding
    if (texid > 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texid);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
}

void tevbind(int target, int texid) {
    // Stub - texture environment binding
    (void)target;
    (void)texid;
}

// Color map functions
void gflush(void) {
    glFlush();
}

void getmcolor(Colorindex index, RGBvalue* r, RGBvalue* g, RGBvalue* b) {
    if (index < 256) {
        if (r) *r = (RGBvalue)(iris_colormap[index][0] * 255);
        if (g) *g = (RGBvalue)(iris_colormap[index][1] * 255);
        if (b) *b = (RGBvalue)(iris_colormap[index][2] * 255);
    }
}

void glcompat(int mode, int value) {
    // Stub - GL compatibility mode setting
    (void)mode;
    (void)value;
}