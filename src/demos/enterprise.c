
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

#define MAZE_HEIGHT (16)
#define MAZE_WIDTH (16)
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)

enum entity_type {
    ENTITY_TYPE_PLAYER,
    ENTITY_PHASER,
    ENTITY_TORPEDO,
    ENTITY_TYPE_ENEMY,
    ENTITY_BASE,
};
typedef struct entity_t {
    float x;
    float y;
    float h; // heading
    float s; // speed
    float m; // speed multiplier
    enum entity_type type;
    float life;
    float energy;
    float shield;
    struct entity_t *parent;
} entity_t;

struct entity_list_t {
    entity_t *entities;
    struct entity_list_t *next;
};
typedef struct {
    float x;
    float y;
} star_t;

#define STAR_COUNT 400
#define STAR_EXTENT 20.0f
static star_t stars[STAR_COUNT];
static float previous_player_heading = 0.0f;
static clock_t last_frame_time = 0;
static int window_width = 800;
static int window_height = 600;
static float cube_angle = 0.0f;
static entity_t player = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, ENTITY_TYPE_PLAYER, -1, -1, -1, NULL};
static entity_t enemy = {0.5f, -0.4f, 0.0f, 0.0f, 1.0f, ENTITY_TYPE_ENEMY, -1, -1, -1, NULL};

static struct entity_list_t *entity_list_head = NULL;

void (*idlefunc)(void) = NULL;

static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void display(void);
static void initGL(void);

static void init_stars(void) {
    for (int i = 0; i < STAR_COUNT; ++i) {
        stars[i].x = ((float)rand() / RAND_MAX - 0.5f) * 40.0f;
        stars[i].y = ((float)rand() / RAND_MAX - 0.5f) * 40.0f;
    }
}
static void wrap_star(star_t *star) {
    if (star->x > STAR_EXTENT) star->x -= STAR_EXTENT * 2.0f;
    else if (star->x < -STAR_EXTENT) star->x += STAR_EXTENT * 2.0f;
    if (star->y > STAR_EXTENT) star->y -= STAR_EXTENT * 2.0f;
    else if (star->y < -STAR_EXTENT) star->y += STAR_EXTENT * 2.0f;
}
static void update_starfield(float delta_time) {
    (void)delta_time;
    float delta_heading = player.h - previous_player_heading;
    while (delta_heading > 180.0f) delta_heading -= 360.0f;
    while (delta_heading < -180.0f) delta_heading += 360.0f;
    float lateral_shift = delta_heading * 0.05f;
    for (int i = 0; i < STAR_COUNT; ++i) {
        stars[i].x += lateral_shift;
        wrap_star(&stars[i]);
    }
    previous_player_heading = player.h;
}
static void draw_starfield(void) {
    glPushMatrix();
    glTranslatef(-player.x * 0.2f, -player.y * 0.2f, -5.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    
    for (int i = 0; i < STAR_COUNT; ++i) {
        glPointSize(i%5);
        glBegin(GL_POINTS);
        glVertex3f(stars[i].x, stars[i].y, 0.0f);
        glEnd();
    }
    
    glPopMatrix();
    glPointSize(1.0f);
}
static void remove_entity_from_list(entity_t *to_remove) {
    if (entity_list_head == NULL || to_remove == NULL) {
        return;
    }

    if (entity_list_head->entities == to_remove) {
        entity_list_head = entity_list_head->next;
    } else {
        struct entity_list_t *current = entity_list_head;
        while (current->next != NULL) {
            if (current->next->entities == to_remove) {
                break;
            }
            current = current->next;
        }
        if (current->next->entities == to_remove) {
            current->next = current->next->next;
        }
    }

    free(to_remove);
}
static float distance(entity_t *a, entity_t *b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    return sqrtf(dx * dx + dy * dy);
}
static void shoot(entity_t *entity, entity_t *target) {
    if (entity->energy > 0.0f) {
        entity->energy -= 1.0f; // recharge energy if negative
        return;
    }
    entity->energy = 200.0f; // shooting costs energy
    float dx = target->x - entity->x;
    float dy = target->y - entity->y;
    float angle = atan2f(dy, dx) * (180.0f / 3.14159265f);

    struct entity_list_t *new_node = (struct entity_list_t *)malloc(sizeof(struct entity_list_t));
    new_node->entities = (entity_t *)malloc(sizeof(entity_t));
    new_node->entities->x = entity->x;
    new_node->entities->y = entity->y;
    new_node->entities->h = angle+270.0f;
    new_node->entities->s = 0.1f;
    new_node->entities->m = 1.0f;
    new_node->entities->type = ENTITY_PHASER;
    new_node->entities->life = 40.0f; // lasts for 2 seconds
    new_node->entities->parent = entity;
    new_node->next = entity_list_head;
    entity_list_head = new_node;
}
static void draw_bitmap_text_wrapped(
    int viewport_width,
    int viewport_height,
    float origin_x,
    float origin_y,
    float max_width,
    const char *text
) {
    const float line_height = (float)glutBitmapHeight(GLUT_BITMAP_HELVETICA_18) / (float)viewport_height;
    float cursor_x = origin_x;
    float cursor_y = origin_y;

    glRasterPos2f(cursor_x, cursor_y);

    for (const unsigned char *c = (const unsigned char *)text; *c; ++c) {
        if (*c == '\n') {
            cursor_x = origin_x;
            cursor_y -= line_height;
            glRasterPos2f(cursor_x, cursor_y);
            continue;
        }

        float glyph_width = (float)glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, *c) / (float)viewport_width;

        if (cursor_x + glyph_width > origin_x + max_width) {
            cursor_x = origin_x;
            cursor_y -= line_height;
            glRasterPos2f(cursor_x, cursor_y);
        }

        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        cursor_x += glyph_width;
    }
}
static void update_player_position(float delta_time, entity_t *entity) {
    // Met à jour la position du joueur en fonction de sa vitesse et de son orientation
    float distance = entity->s * entity->m * delta_time;
    float radians = (entity->h + 90) * (3.14159265f / 180.0f);
    entity->x += cosf(radians) * distance;
    entity->y += sinf(radians) * distance;
    
    if (entity->life>0) {
        entity->life -= delta_time * 5.0f;
        if (entity->life < 0) {
            remove_entity_from_list(entity);
        };
    }
}
static void draw_rect_outline(int x, int y, int width, int height) {
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x, y + height);
    glEnd();
}

static void draw_layout_borders(int top_left_width,
                                int top_right_width,
                                int top_height,
                                int bottom_height,
                                int top_y) {
    glViewport(0, 0, window_width, window_height);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, window_width, 0.0, window_height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0.0f, 1.0f, 0.0f);
    glLineWidth(2.0f);

    draw_rect_outline(0, top_y, top_left_width, top_height);
    draw_rect_outline(top_left_width, top_y, top_right_width, top_height);
    draw_rect_outline(0, 0, window_width, bottom_height);
}
static void render_top_left_2d(int viewport_width, int viewport_height) {
    (void)viewport_width;
    (void)viewport_height;

    char buffer[256];
    snprintf(
        buffer,
        sizeof(buffer),
        "Player Position: (%.2f, %.2f) Heading: %.2f",
        player.x,
        player.y,
        player.h
    );
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float bar_width = 0.8f;
    bar_width =bar_width - (0.8 *(-player.life > 0.0f) ? (-player.life / 100.0f) : 0.0f);
    glPushMatrix();
    glTranslatef(0.1f, 0.1f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.8f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(bar_width, 0.0f);
    glVertex2f(bar_width, 0.1f);
    glVertex2f(0.0f, 0.1f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 0.25f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.8f, 0.0f);
    glVertex2f(0.8f, 0.1f);
    glVertex2f(0.0f, 0.1f);
    glEnd();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 0.40f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.8f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(0.8f, 0.0f);
    glVertex2f(0.8f, 0.1f);
    glVertex2f(0.0f, 0.1f);
    glEnd();
    glPopMatrix();

    glColor3f(1.0f, 1.0f, 1.0f);
    snprintf(
        buffer,
        sizeof(buffer),
        "Player Position: (%.2f, %.2f) Heading: %.2f",
        player.x,
        player.y,
        player.h
    );
    draw_bitmap_text_wrapped(
        viewport_width,
        viewport_height,
        0.05f,
        0.9f,
        0.85f,   // largeur autorisée dans [0,1]
        buffer
    );
    snprintf(
        buffer,
        sizeof(buffer),
        "Enemy Position: (%.2f, %.2f) Heading: %.2f",
        enemy.x,
        enemy.y,
        enemy.h
    );
    draw_bitmap_text_wrapped(
        viewport_width,
        viewport_height,
        0.05f,
        0.75f,
        0.85f,   // largeur autorisée dans [0,1]
        buffer
    );
}
static void draw_spaceship_top_view(void) {
    // Corps principal - triangle vue de dessus
    glBegin(GL_TRIANGLES);
    glColor3f(0.6f, 0.6f, 0.7f);
    glVertex2f(0.0f, 0.05f);        // Pointe avant
    glVertex2f(-0.03f, -0.05f);     // Arrière gauche
    glVertex2f(0.03f, -0.05f);      // Arrière droit
    glEnd();
    
    // Cockpit - petit triangle au centre avant
    glBegin(GL_TRIANGLES);
    glColor3f(0.2f, 0.5f, 0.8f);
    glVertex2f(0.0f, 0.03f);
    glVertex2f(-0.01f, 0.0f);
    glVertex2f(0.01f, 0.0f);
    glEnd();
    
    // Moteur gauche
    glBegin(GL_TRIANGLES);
    glColor3f(0.5f, 0.5f, 0.6f);
    glVertex2f(-0.025f, -0.03f);
    glVertex2f(-0.035f, -0.06f);
    glVertex2f(-0.015f, -0.06f);
    glEnd();
    
    // Moteur droit
    glBegin(GL_TRIANGLES);
    glColor3f(0.5f, 0.5f, 0.6f);
    glVertex2f(0.025f, -0.03f);
    glVertex2f(0.035f, -0.06f);
    glVertex2f(0.015f, -0.06f);
    glEnd();
    
}
static void draw_player_ship_top_view(void) {
    // Corps principal - forme de flèche allongée
    glBegin(GL_TRIANGLES);
    glColor3f(0.8f, 0.3f, 0.3f);
    glVertex2f(0.0f, 0.07f);        // Pointe avant longue
    glVertex2f(-0.02f, -0.01f);     // Milieu gauche
    glVertex2f(0.02f, -0.01f);      // Milieu droit
    glEnd();
    
    // Section arrière du corps
    glBegin(GL_QUADS);
    glColor3f(0.7f, 0.2f, 0.2f);
    glVertex2f(-0.02f, -0.01f);
    glVertex2f(0.02f, -0.01f);
    glVertex2f(0.015f, -0.04f);
    glVertex2f(-0.015f, -0.04f);
    glEnd();
    
    // Cockpit - capsule au centre
    glBegin(GL_QUADS);
    glColor3f(0.3f, 0.6f, 0.9f);
    glVertex2f(-0.008f, 0.02f);
    glVertex2f(0.008f, 0.02f);
    glVertex2f(0.008f, 0.0f);
    glVertex2f(-0.008f, 0.0f);
    glEnd();
    
    // Réacteur gauche
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.5f, 0.6f);
    glVertex2f(-0.025f, -0.04f);
    glVertex2f(-0.015f, -0.04f);
    glVertex2f(-0.015f, -0.05f);
    glVertex2f(-0.025f, -0.05f);
    glEnd();
    
    
    // Réacteur droit
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.5f, 0.6f);
    glVertex2f(0.015f, -0.04f);
    glVertex2f(0.025f, -0.04f);
    glVertex2f(0.025f, -0.05f);
    glVertex2f(0.015f, -0.05f);
    glEnd();
}
static void render_top_right_2d(int viewport_width, int viewport_height) {
    (void)viewport_width;
    (void)viewport_height;

    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    
    glLoadIdentity();

    glTranslatef(-player.x, -player.y, 0);
    
    struct entity_list_t *node = entity_list_head;
    while (node != NULL) {
        struct entity_list_t *current = node;
        glPushMatrix();
        glTranslatef(current->entities->x, current->entities->y, 0.0f);
        glRotatef(current->entities->h, 0.0f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f(-0.02f, -0.02f);
        glVertex2f(0.02f, -0.02f);
        glVertex2f(0.02f, 0.02f);
        glVertex2f(-0.02f, 0.02f);
        glEnd();
        glPopMatrix();
        node = node->next;
    }
    glPushMatrix();
    glTranslatef(enemy.x, enemy.y, 0.0f);
    glRotatef(enemy.h, 0.0f, 0.0f, 1.0f);
    draw_spaceship_top_view();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(player.x, player.y, 0.0f);
    glRotatef(player.h, 0.0f, 0.0f, 1.0f);
    draw_player_ship_top_view();
    glPopMatrix();
}
static void draw_spaceship(void) {
    // Corps principal - forme de losange avec épaisseur
    float body_height = 0.03f;
    
    // Face supérieure du corps
    glBegin(GL_TRIANGLES);
    glColor3f(0.6f, 0.6f, 0.7f);
    glVertex3f(0.0f, 0.1f, 0.0f);           // Pointe avant
    glVertex3f(-0.05f, -0.05f, 0.0f);       // Arrière gauche
    glVertex3f(0.0f, 0.0f, body_height);    // Centre haut
    
    glVertex3f(0.0f, 0.1f, 0.0f);
    glVertex3f(0.0f, 0.0f, body_height);
    glVertex3f(0.05f, -0.05f, 0.0f);        // Arrière droit
    
    glVertex3f(-0.05f, -0.05f, 0.0f);
    glVertex3f(0.05f, -0.05f, 0.0f);
    glVertex3f(0.0f, 0.0f, body_height);
    glEnd();
    
    // Face inférieure du corps
    glBegin(GL_TRIANGLES);
    glColor3f(0.4f, 0.4f, 0.5f);
    glVertex3f(0.0f, 0.1f, 0.0f);
    glVertex3f(0.0f, 0.0f, -body_height);
    glVertex3f(-0.05f, -0.05f, 0.0f);
    
    glVertex3f(0.0f, 0.1f, 0.0f);
    glVertex3f(0.05f, -0.05f, 0.0f);
    glVertex3f(0.0f, 0.0f, -body_height);
    
    glVertex3f(-0.05f, -0.05f, 0.0f);
    glVertex3f(0.0f, 0.0f, -body_height);
    glVertex3f(0.05f, -0.05f, 0.0f);
    glEnd();
    
    // Côtés du corps
    glBegin(GL_TRIANGLES);
    glColor3f(0.5f, 0.5f, 0.6f);
    // Avant gauche
    glVertex3f(0.0f, 0.1f, 0.0f);
    glVertex3f(0.0f, 0.0f, body_height);
    glVertex3f(0.0f, 0.0f, -body_height);
    
    // Arrière
    glVertex3f(-0.05f, -0.05f, 0.0f);
    glVertex3f(0.0f, 0.0f, -body_height);
    glVertex3f(0.0f, 0.0f, body_height);
    
    glVertex3f(0.05f, -0.05f, 0.0f);
    glVertex3f(0.0f, 0.0f, body_height);
    glVertex3f(0.0f, 0.0f, -body_height);
    glEnd();
    
    
    // Cockpit - pyramide plus prononcée
    float cockpit_height = 0.04f;
    glBegin(GL_TRIANGLES);
    glColor3f(0.2f, 0.5f, 0.8f);
    // Sommet vers l'avant
    glVertex3f(0.01f, 0.06f, cockpit_height);
    glVertex3f(-0.015f, 0.02f, 0.0f);
    glVertex3f(0.015f, 0.02f, 0.0f);
    
    glVertex3f(0.01f, 0.06f, cockpit_height);
    glVertex3f(-0.015f, 0.02f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    
    glVertex3f(0.01f, 0.06f, cockpit_height);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.015f, 0.02f, 0.0f);
    
    // Base
    glColor3f(0.15f, 0.4f, 0.7f);
    glVertex3f(-0.015f, 0.02f, 0.0f);
    glVertex3f(0.015f, 0.02f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    
    // Moteurs - cylindres épais
    float motor_radius = 0.012f;
    float motor_length = 0.04f;
    
    // Moteur gauche
    glPushMatrix();
    glTranslatef(-0.03f, -0.06f, 0.0f);
    glColor3f(1.0f, 0.3f, 0.0f);
    glBegin(GL_TRIANGLES);
    // Arrière (flamme)
    glVertex3f(-motor_radius, -motor_length, 0.0f);
    glVertex3f(motor_radius, -motor_length, 0.0f);
    glVertex3f(0.0f, -motor_length, motor_radius);
    
    glVertex3f(-motor_radius, -motor_length, 0.0f);
    glVertex3f(0.0f, -motor_length, motor_radius);
    glVertex3f(0.0f, -motor_length, -motor_radius);
    
    glVertex3f(motor_radius, -motor_length, 0.0f);
    glVertex3f(0.0f, -motor_length, -motor_radius);
    glVertex3f(0.0f, -motor_length, motor_radius);
    
    // Corps
    glColor3f(0.6f, 0.6f, 0.7f);
    glVertex3f(-motor_radius, 0.0f, 0.0f);
    glVertex3f(-motor_radius, -motor_length, 0.0f);
    glVertex3f(0.0f, 0.0f, motor_radius);
    
    glVertex3f(motor_radius, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, motor_radius);
    glVertex3f(motor_radius, -motor_length, 0.0f);
    glEnd();
    glPopMatrix();
    
    // Moteur droit
    glPushMatrix();
    glTranslatef(0.03f, -0.06f, 0.0f);
    glColor3f(1.0f, 0.3f, 0.0f);
    glBegin(GL_TRIANGLES);
    // Arrière (flamme)
    glVertex3f(-motor_radius, -motor_length, 0.0f);
    glVertex3f(motor_radius, -motor_length, 0.0f);
    glVertex3f(0.0f, -motor_length, motor_radius);
    
    glVertex3f(-motor_radius, -motor_length, 0.0f);
    glVertex3f(0.0f, -motor_length, motor_radius);
    glVertex3f(0.0f, -motor_length, -motor_radius);
    
    glVertex3f(motor_radius, -motor_length, 0.0f);
    glVertex3f(0.0f, -motor_length, -motor_radius);
    glVertex3f(0.0f, -motor_length, motor_radius);
    
    // Corps
    glColor3f(0.6f, 0.6f, 0.7f);
    glVertex3f(-motor_radius, 0.0f, 0.0f);
    glVertex3f(-motor_radius, -motor_length, 0.0f);
    glVertex3f(0.0f, 0.0f, motor_radius);
    
    glVertex3f(motor_radius, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, motor_radius);
    glVertex3f(motor_radius, -motor_length, 0.0f);
    glEnd();
    glPopMatrix();
}
static void render_bottom_3d(int viewport_width, int viewport_height) {
    if (viewport_height <= 0) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(
        45.0,
        (viewport_height > 0) ? (double)viewport_width / (double)viewport_height : 1.0,
        0.01,
        100.0
    );
    glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat[]){0.0f, 0.0f, 1.0f, 0.0f});
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    draw_starfield();
    // XY -> plan horizontal
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(-player.h, 0.0f, 0.0f, 1.0f);
    glTranslatef(-player.x, -player.y, 0.0f);

    glPushMatrix();
    glTranslatef(enemy.x, enemy.y, 0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glRotatef(enemy.h, 0.0f, 0.0f, 1.0f);
    draw_spaceship();
    glPopMatrix();
    
    struct entity_list_t *node = entity_list_head;
    while (node != NULL) {
        struct entity_list_t *current = node;
        glPushMatrix();
        glTranslatef(current->entities->x, current->entities->y, 0.0f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glScalef(0.002f, 0.002f, 0.001f);
        glutSolidDodecahedron();    
        glPopMatrix();
        node = node->next;
    }

    // Crosshair overlay (écran)
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);
    
    glBegin(GL_LINES);
    glVertex2f(0.0f, 0.2f);
    glVertex2f(0.0f, 0.04f);
    glEnd();
    
    
    glBegin(GL_LINES);
    glVertex2f(0.02f, 0.0f);
    glVertex2f(0.1f, 0.0f);
    glEnd();
    
    glBegin(GL_LINES);
    glVertex2f(-0.02f, 0.0f);
    glVertex2f(-0.1f, 0.0f);
    glEnd();
    
    glBegin(GL_LINES);
    glVertex2f(0.0f, -0.2f);
    glVertex2f(0.0f, -0.04f);
    glEnd();
    
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_LIGHTING);
}
static void game_loop(double delta_time) {
    update_player_position(delta_time, &player);
    update_starfield((float)delta_time);
    enemy.h += 0.1f;
    if (enemy.h >= 360.0f) enemy.h -= 360.0f;
    enemy.s = 0.05f;
    update_player_position(delta_time, &enemy);
    if (distance(&enemy, &player) < 0.5f) {
        shoot(&enemy, &player);
    }
    struct entity_list_t *node = entity_list_head;
    while (node != NULL) {
        update_player_position(delta_time, node->entities);
        if (node->entities->parent != &player && distance(node->entities, &player) < 0.05f && node->entities->type == ENTITY_PHASER) {
            player.life -= 1.0f;
            remove_entity_from_list(node->entities);
        }
        if (node->entities->parent != &enemy && distance(node->entities, &enemy) < 0.05f && node->entities->type == ENTITY_PHASER) {
            enemy.life -= 1.0f;
            remove_entity_from_list(node->entities);
        }
        node = node->next;
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
    window_width = (w > 0) ? w : 1;
    window_height = (h > 0) ? h : 1;
    glViewport(0, 0, window_width, window_height);
}

static void idle(void) {
    // on se contente de redessiner en continu
    clock_t current_time = clock();
    double elapsed_ms = (double)(current_time - last_frame_time) * 1000.0 / CLOCKS_PER_SEC;
    
    if (elapsed_ms >= FRAME_TIME_MS) {
        last_frame_time = current_time;
        game_loop(elapsed_ms / 1000.0);
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
    case 32: // SPACE
        struct entity_list_t *new_node = (struct entity_list_t *)malloc(sizeof(struct entity_list_t));
        new_node->entities = (entity_t *)malloc(sizeof(entity_t));
        new_node->entities->x = player.x;
        new_node->entities->y = player.y;
        new_node->entities->h = player.h;
        new_node->entities->s = 0.2f;
        new_node->entities->m = 1.0f;
        new_node->entities->type = ENTITY_PHASER;
        new_node->entities->life = 10;
        new_node->entities->parent = &player;
        new_node->next = entity_list_head;
        entity_list_head = new_node;
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
    case GLUT_KEY_LEFT:
        player.h += 5.0f;
        break;
    case GLUT_KEY_RIGHT:
        player.h -= 5.0f;
        break;
    case GLUT_KEY_UP:
        player.s = 0.1f;
        break;
    default:
        break;
    }
}

static void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    case GLUT_KEY_UP:
        player.s = 0.0f;
        break;
    default:
        break;
    }
}

static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int top_height = window_height / 2;
    int bottom_height = window_height - top_height;
    int top_left_width = window_width / 3;
    int top_right_width = window_width - top_left_width;
    int top_y = bottom_height;

    glViewport(0, top_y, top_left_width, top_height);
    render_top_left_2d(top_left_width, top_height);

    glViewport(top_left_width, top_y, top_right_width, top_height);
    render_top_right_2d(top_right_width, top_height);

    glViewport(0, 0, window_width, bottom_height);
    render_bottom_3d(window_width, bottom_height);

    draw_layout_borders(top_left_width, top_right_width, top_height, bottom_height, top_y);

    glutSwapBuffers();
}
static void initGL(void) {
    last_frame_time = clock();
    glEnable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    init_stars();
    idlefunc = NULL;
}
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    // Double buffer + RGB + z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Legacy OpenGL Demo");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);
    

    // bouton droit de la souris = menu
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