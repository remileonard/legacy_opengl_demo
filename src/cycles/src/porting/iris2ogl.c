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

// === Color Management ===

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
        glColor3f(iris_colormap[index][0], iris_colormap[index][1], iris_colormap[index][2]);
    }
}

void cpack(uint32_t color) {
    // Convert packed RGB (0xRRGGBB) to float components
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    glColor3f(r, g, b);
}

void mapcolor(Colorindex index, RGBvalue r, RGBvalue g, RGBvalue b) {
    if (index < 256) {
        iris_colormap[index][0] = r / 255.0f;
        iris_colormap[index][1] = g / 255.0f;
        iris_colormap[index][2] = b / 255.0f;
    }
}

// === Buffer Swapping ===

void swapbuffers(void) {
    glutSwapBuffers();
    // Request a new display update so GLUT keeps generating REDRAW events
    glutPostRedisplay();
}

// === Geometric Primitives ===

void rectf(Coord x1, Coord y1, Coord x2, Coord y2) {
    glRectf(x1, y1, x2, y2);
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

// === Matrix and Transformations ===

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
    } else if (id > 0 && id < MAX_PATTERNS && pattern_defined[id]) {
        glEnable(GL_POLYGON_STIPPLE);
        glPolygonStipple(stipple_patterns[id]);
    }
}

// === Lighting ===

void lmdef(int deftype, int index, int np, float props[]) {
    // Simplified lighting - would need full implementation for real use
    // This is a stub for now
}

void lmbind(int target, int index) {
    // Simplified lighting binding - stub
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
#define DEFAULT_FONT 3  // TIMES_ROMAN_24

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
    current_raster_x = x;
    current_raster_y = y;
    current_raster_z = z;
    glRasterPos3f(x, y, z);
}

void cmov2(Coord x, Coord y) {
    current_raster_x = x;
    current_raster_y = y;
    current_raster_z = 0.0f;
    glRasterPos2f(x, y);
}

void fmprstr(const char *str) {
    if (current_font == NULL) {
        current_font = glut_fonts[DEFAULT_FONT];
    }
    
    for (const char *c = str; *c != '\0'; c++) {
        glutBitmapCharacter(current_font, *c);
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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(window_x, window_y);
    main_window = glutCreateWindow(title);
    
    // Register GLUT callbacks
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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void winclose(int win) {
    glutDestroyWindow(win);
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

void prefposition(int x1, int y1, int x2, int y2) {
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
    if (enable) {
        glDrawBuffer(GL_FRONT);
    }
}

void backbuffer(Boolean enable) {
    if (enable) {
        glDrawBuffer(GL_BACK);
    }
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

void qdevice(Device dev) {
    if (dev >= 0 && dev < 256) {
        queued_devices[dev] = 1;
        
        // Generate an initial event for REDRAW to trigger display
        if (dev == REDRAW) {
            queue_event(REDRAW, 1);
            glutPostRedisplay();  // Force GLUT to call display callback
        }
    }
}

void unqdevice(Device dev) {
    if (dev >= 0 && dev < 256) {
        queued_devices[dev] = 0;
    }
}

Boolean qtest(void) {
    // Process any pending GLUT events to populate the queue
    // This is crucial for window events and display callbacks
    glutMainLoopEvent();
    
    return (event_queue_head != event_queue_tail) ? TRUE : FALSE;
}

int32_t qread(int16_t *val) {
    // If queue is empty, process GLUT events until we get one
    while (event_queue_head == event_queue_tail) {
        // Process pending GLUT events
        // This requires freeglut with glutMainLoopEvent support
        glutMainLoopEvent();
        
        // Check if window was closed
        if (glutGetWindow() == 0) {
            *val = 0;
            return 0;
        }
    }
    
    Event evt = event_queue[event_queue_head];
    event_queue_head = (event_queue_head + 1) % EVENT_QUEUE_SIZE;
    *val = evt.value;
    return evt.device;
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
    current_mouse_y = y;
    
    if (queued_devices[MOUSEX]) {
        queue_event(MOUSEX, x);
    }
    if (queued_devices[MOUSEY]) {
        queue_event(MOUSEY, y);
    }
}

static void iris_display_func(void) {
    // Generate REDRAW event if it's being listened to
    queue_event(REDRAW, 1);
    // Don't actually draw here - let the application handle it
    // The application will call swapbuffers() when ready
}

static void iris_idle_func(void) {
    // Idle callback - keeps GLUT processing events
    // This allows qread() to receive events even when blocked
    
    // Small sleep to avoid consuming 100% CPU
    #ifdef _WIN32
    Sleep(1);
    #else
    usleep(1000);
    #endif
    queue_event(REDRAW, 1);
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
