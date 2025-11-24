
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
#define CALC_NORMAL(v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z) \
        { \
            float ux = v2x - v1x, uy = v2y - v1y, uz = v2z - v1z; \
            float vx = v3x - v1x, vy = v3y - v1y, vz = v3z - v1z; \
            float nx = uy * vz - uz * vy; \
            float ny = uz * vx - ux * vz; \
            float nz = ux * vy - uy * vx; \
            float len = sqrtf(nx*nx + ny*ny + nz*nz); \
            if (len > 0.0001f) { nx /= len; ny /= len; nz /= len; } \
            glNormal3f(nx, ny, nz); \
        }

enum entity_type {
    ENTITY_TYPE_PLAYER,
    ENTITY_PHASER,
    ENTITY_TORPEDO,
    ENTITY_TYPE_ENEMY,
    ENTITY_BASE,
};
enum game_state {
    GAME_STATE_TITLE,
    GAME_STATE_PLAYING,
    GAME_STATE_GAME_OVER
};
static enum game_state current_game_state = GAME_STATE_TITLE;

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
    void (*automat)(struct entity_t *self, double delta_time);
    int marked_for_removal;
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
static int nb_enemies = 5;
static int sector = 0;
static int score = 0;
static int altDown = 0;
static int shiftDown = 0;
int keyStates[256] = {0};        // État des touches normales
int specialKeyStates[256] = {0}; // État des touches spéciales (flèches)

static struct entity_list_t *entity_list_head = NULL;

void (*idlefunc)(void) = NULL;

static void init_stars(void);
static void wrap_star(star_t *star);
static void update_starfield(float delta_time);
static void draw_starfield(void);
static void remove_entity_from_list(entity_t *to_remove);
static float distance(entity_t *a, entity_t *b);
static void shoot(entity_t *entity, entity_t *target);
static void draw_bitmap_text_wrapped(int viewport_width,int viewport_height,float origin_x,float origin_y,float max_width,const char *text);
static void update_player_position(float delta_time, entity_t *entity);
static void draw_rect_outline(int x, int y, int width, int height);
static void draw_layout_borders(int top_left_width,int top_right_width,int top_height,int bottom_height,int top_y);
static void render_top_left_2d(int viewport_width, int viewport_height);
static void draw_spaceship_top_view(void);
static void draw_player_ship_top_view(void);
static void render_top_right_2d(int viewport_width, int viewport_height);
static void draw_spaceship(void);
static void render_bottom_3d(int viewport_width, int viewport_height);
static void game_loop(double delta_time);
static void reshape(int w, int h);
static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void render_gameplay(void);
static void display(void);
static void initGL(void);
static void enemi_automaton(entity_t *self, double delta_time);
static void player_automaton(entity_t *self, double delta_time);
static void phaser_automaton(entity_t *self, double delta_time);
static void init_enemies(void);
static void draw_cracked_screen_overlay(float damage_level);
static void reset_game(void);
static entity_t player = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, ENTITY_TYPE_PLAYER, -1, -1, -1, NULL, NULL, 0};


static void player_automaton(entity_t *self, double delta_time) {
    update_player_position(delta_time, &player);
    if (player.s > 0.0f) {
        player.s -= (float)(delta_time * 0.05);
    }
    if (player.shield > 0.0f) {
        player.shield -= (float)(delta_time * 30.0);
    }
    if (player.energy > 0.0f) {
        player.energy -= (float)(delta_time * 200.0);
        if (player.energy < 0.0f) {
            player.energy = 0.0f;
        }
    }
}
static void phaser_automaton(entity_t *self, double delta_time) {
    update_player_position(delta_time, self);
    if (self->parent != &player && distance(self, &player) < 0.05f && self->type == ENTITY_PHASER) {
        player.life -= 5.0f;
        if (player.shield > 0.0f) {
            player.life -= 5.0f;
        }
        player.shield = 30.0f;
        self->marked_for_removal = 1;
    } else if (self->parent == &player) {
        struct entity_list_t *enemy_node = entity_list_head;
        while (enemy_node != NULL) {
            if (enemy_node->entities->type == ENTITY_TYPE_ENEMY && distance(self, enemy_node->entities) < 0.05f && self->type == ENTITY_PHASER) {
                enemy_node->entities->life -= 1.0f;
                self->marked_for_removal = 1;
                enemy_node->entities->marked_for_removal = 1;
                nb_enemies--;
                score += 100;
            }
            enemy_node = enemy_node->next;
        }
    }
}
static void render_game_over_screen(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Fond avec étoiles
    glViewport(0, 0, window_width, window_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)window_width / (double)window_height, 0.01, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    draw_starfield();
    
    // Retour en 2D pour le texte
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    
    // Titre Game Over
    glColor3f(1.0f, 0.0f, 0.0f);
    draw_bitmap_text_wrapped(
        window_width,
        window_height,
        0.35f,
        0.7f,
        0.3f,
        "GAME OVER"
    );
    
    // Score final
    char score_buffer[64];
    snprintf(score_buffer, sizeof(score_buffer), "Final Score: %d", score);
    glColor3f(0.0f, 1.0f, 0.0f);
    draw_bitmap_text_wrapped(
        window_width,
        window_height,
        0.3f,
        0.55f,
        0.4f,
        score_buffer
    );
    
    // Secteur atteint
    char sector_buffer[64];
    snprintf(sector_buffer, sizeof(sector_buffer), "Sectors Cleared: %d", sector);
    glColor3f(0.0f, 1.0f, 0.0f);
    draw_bitmap_text_wrapped(
        window_width,
        window_height,
        0.25f,
        0.45f,
        0.5f,
        sector_buffer
    );
    
    // Message pour rejouer
    glColor3f(1.0f, 1.0f, 0.0f);
    draw_bitmap_text_wrapped(
        window_width,
        window_height,
        0.25f,
        0.25f,
        0.5f,
        "Press SPACE to play again\nPress ESC to quit"
    );
    
    glutSwapBuffers();
}
static void cleanup_marked_entities(void) {
    struct entity_list_t *current = entity_list_head;
    struct entity_list_t *prev = NULL;
    
    while (current != NULL) {
        struct entity_list_t *next = current->next;
        
        if (current->entities->marked_for_removal) {
            // Détacher du chaînage
            if (prev == NULL) {
                entity_list_head = next;
            } else {
                prev->next = next;
            }
            
            // Libérer la mémoire
            free(current->entities);
            free(current);
        } else {
            prev = current;
        }
        
        current = next;
    }
}
static void enemi_automaton(entity_t *self, double delta_time) {
    if (self->energy > 0.0f) {
        self->energy -= delta_time * 50.0f;
        if (self->energy < 0.0f) {
            self->energy = 0.0f;
        }
    }
    float dx = player.x - self->x;
    float dy = player.y - self->y;
    float angle_to_player = atan2f(dy, dx) * (180.0f / 3.14159265f) - 90.0f;

    
    while (angle_to_player > 180.0f) angle_to_player -= 360.0f;
    while (angle_to_player < -180.0f) angle_to_player += 360.0f;
    while (self->h > 180.0f) self->h -= 360.0f;
    while (self->h < -180.0f) self->h += 360.0f;
    

    float angle_diff = angle_to_player - self->h;
    while (angle_diff > 180.0f) angle_diff -= 360.0f;
    while (angle_diff < -180.0f) angle_diff += 360.0f;
    

    if (distance(self, &player) > 0.5f) {
        self->s = 0.05f; // constant speed
    } else {
        self->s = 0.0f; // stop when close
    }
    self->h += (angle_diff > 0.0f ? 1.0f : -1.0f);
    if (distance(self, &player) < 0.5f && abs(angle_diff) < 2.0f) {
        shoot(self, &player);
    }
    update_player_position((float)delta_time, self);
}

static void draw_cracked_screen_overlay(float damage_level) {
    // Sauvegarde de l'état
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Intensité basée sur les dégâts (0.0 = pas de fissures, 1.0 = très fissuré)
    float alpha = fminf(damage_level, 1.0f) * 0.7f;
    
    glLineWidth(2.0f);
    glColor4f(0.8f, 0.8f, 0.9f, alpha);
    
    // Fissure principale diagonale
    glBegin(GL_LINE_STRIP);
    glVertex2f(-1.0f, 1.0f);
    glVertex2f(-0.4f, 0.5f);
    glVertex2f(-0.2f, 0.4f);
    glVertex2f(0.1f, 0.2f);
    glVertex2f(0.3f, -0.1f);
    glVertex2f(0.5f, -0.4f);
    glVertex2f(1.0f, -1.0f);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(0.4f, -0.5f);
    glVertex2f(0.2f, -0.4f);
    glVertex2f(-0.1f, -0.2f);
    glVertex2f(-0.3f, 0.1f);
    glVertex2f(-0.5f, 0.4f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();
    
    // Fissures secondaires (branches)
    if (damage_level > 0.3f) {
        glLineWidth(1.5f);
        glColor4f(0.7f, 0.7f, 0.8f, alpha * 0.8f);
        
        // Branche 1
        glBegin(GL_LINE_STRIP);
        glVertex2f(-0.4f, 0.5f);
        glVertex2f(-0.6f, 0.3f);
        glVertex2f(-0.8f, 0.1f);
        glEnd();
        
        // Branche 2
        glBegin(GL_LINE_STRIP);
        glVertex2f(-0.2f, 0.4f);
        glVertex2f(-0.3f, 0.2f);
        glVertex2f(-0.5f, 0.0f);
        glEnd();
        
        // Branche 3
        glBegin(GL_LINE_STRIP);
        glVertex2f(0.1f, 0.2f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(-0.1f, -0.2f);
        glEnd();
        
        // Branche 4
        glBegin(GL_LINE_STRIP);
        glVertex2f(0.3f, -0.1f);
        glVertex2f(0.4f, 0.1f);
        glVertex2f(0.6f, 0.2f);
        glEnd();
    }
    
    // Fissures tertiaires (plus de détails)
    if (damage_level > 0.6f) {
        glLineWidth(1.0f);
        glColor4f(0.6f, 0.6f, 0.7f, alpha * 0.6f);
        
        // Mini-fissures
        glBegin(GL_LINES);
        glVertex2f(-0.6f, 0.3f); glVertex2f(-0.7f, 0.4f);
        glVertex2f(-0.5f, 0.0f); glVertex2f(-0.6f, -0.1f);
        glVertex2f(0.0f, 0.0f); glVertex2f(0.05f, -0.15f);
        glVertex2f(0.4f, 0.1f); glVertex2f(0.5f, 0.25f);
        glVertex2f(0.5f, -0.4f); glVertex2f(0.6f, -0.5f);
        glEnd();
        
        // Effet de fragmentation
        glBegin(GL_LINE_LOOP);
        glVertex2f(-0.3f, 0.3f);
        glVertex2f(-0.25f, 0.25f);
        glVertex2f(-0.2f, 0.3f);
        glVertex2f(-0.25f, 0.35f);
        glEnd();
        
        glBegin(GL_LINE_LOOP);
        glVertex2f(0.2f, -0.05f);
        glVertex2f(0.25f, -0.1f);
        glVertex2f(0.3f, -0.05f);
        glVertex2f(0.25f, 0.0f);
        glEnd();
    }
    
    // Restauration de l'état
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
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
    glDisable(GL_LIGHTING);
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
        if (to_remove == entity_list_head->entities) {
            entity_list_head = entity_list_head->next;
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
        return;
    }
    entity->energy = 200.0f; // shooting costs energy
    
    struct entity_list_t *new_node = (struct entity_list_t *)malloc(sizeof(struct entity_list_t));
    new_node->entities = (entity_t *)malloc(sizeof(entity_t));
    new_node->entities->x = entity->x;
    new_node->entities->y = entity->y;
    new_node->entities->h = entity->h;
    new_node->entities->s = 0.2f;
    new_node->entities->m = 1.0f;
    new_node->entities->type = ENTITY_PHASER;
    new_node->entities->life = 40.0f; // lasts for 2 seconds
    new_node->entities->parent = entity;
    new_node->entities->automat = NULL;
    new_node->entities->marked_for_removal = 0;
    new_node->entities->automat = phaser_automaton;
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
        "SECTOR %d - SCORE: %d",
        sector,
        score
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

    bar_width = 0.8f;
    bar_width = bar_width - (bar_width * (player.shield > 0.0f ? (player.shield / 100.0f) : 0.0f));
    glPushMatrix();
    glTranslatef(0.1f, 0.25f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.8f, 0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(bar_width, 0.0f);
    glVertex2f(bar_width, 0.1f);
    glVertex2f(0.0f, 0.1f);
    glEnd();
    glPopMatrix();

    bar_width = 0.8f;
    bar_width = bar_width - (bar_width * (player.energy > 0.0f ? (player.energy / 200.0f) : 0.0f));
    glPushMatrix();
    glTranslatef(0.1f, 0.40f, 0.0f);
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.8f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(bar_width, 0.0f);
    glVertex2f(bar_width, 0.1f);
    glVertex2f(0.0f, 0.1f);
    glEnd();
    glPopMatrix();

    glColor3f(1.0f, 1.0f, 1.0f);
    snprintf(
        buffer,
        sizeof(buffer),
        "SECTOR %d",
        sector
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
        "SCORE: %d",
        score
    );
    draw_bitmap_text_wrapped(
        viewport_width,
        viewport_height,
        0.05f,
        0.8f,
        0.85f,   // largeur autorisée dans [0,1]
        buffer
    );
}
static void draw_spaceship_top_view(void) {
    // Corps central (cou allongé) - vue de dessus
    glBegin(GL_QUADS);
    glColor3f(0.3f, 0.4f, 0.3f);
    glVertex2f(-0.008f, 0.05f);      // Haut gauche du cou
    glVertex2f(0.008f, 0.05f);       // Haut droit du cou
    glVertex2f(0.008f, -0.08f);      // Bas droit du cou
    glVertex2f(-0.008f, -0.08f);     // Bas gauche du cou
    glEnd();
    
    // Tête/Cockpit (bec pointu)
    glBegin(GL_TRIANGLES);
    glColor3f(0.4f, 0.5f, 0.4f);
    glVertex2f(0.0f, 0.1f);          // Pointe avant
    glVertex2f(-0.015f, 0.05f);      // Gauche
    glVertex2f(0.015f, 0.05f);       // Droite
    glEnd();
    
    // Aile gauche (forme de rapace déployée)
    glBegin(GL_POLYGON);
    glColor3f(0.35f, 0.45f, 0.35f);
    glVertex2f(-0.008f, 0.0f);       // Attache au corps
    glVertex2f(-0.02f, 0.01f);       // Bord avant interne
    glVertex2f(-0.055f, -0.03f);     // Pointe avant de l'aile
    glVertex2f(-0.08f, -0.06f);      // Extrémité de l'aile
    glVertex2f(-0.05f, -0.08f);      // Bord arrière externe
    glVertex2f(-0.03f, -0.09f);      // Pointe arrière
    glVertex2f(-0.015f, -0.08f);     // Retour vers le corps
    glEnd();
    
    // Aile droite (symétrique)
    glBegin(GL_POLYGON);
    glColor3f(0.35f, 0.45f, 0.35f);
    glVertex2f(0.008f, 0.0f);        // Attache au corps
    glVertex2f(0.015f, -0.08f);      // Retour vers le corps
    glVertex2f(0.03f, -0.09f);       // Pointe arrière
    glVertex2f(0.05f, -0.08f);       // Bord arrière externe
    glVertex2f(0.08f, -0.06f);       // Extrémité de l'aile
    glVertex2f(0.055f, -0.03f);      // Pointe avant de l'aile
    glVertex2f(0.02f, 0.01f);        // Bord avant interne
    glEnd();
    
    // Détails des ailes - bords accentués
    glColor3f(0.25f, 0.35f, 0.25f);
    glLineWidth(1.5f);
    
    // Contour aile gauche
    glBegin(GL_LINE_STRIP);
    glVertex2f(-0.008f, 0.0f);
    glVertex2f(-0.02f, 0.01f);
    glVertex2f(-0.055f, -0.03f);
    glVertex2f(-0.08f, -0.06f);
    glVertex2f(-0.05f, -0.08f);
    glVertex2f(-0.03f, -0.09f);
    glEnd();
    
    // Contour aile droite
    glBegin(GL_LINE_STRIP);
    glVertex2f(0.008f, 0.0f);
    glVertex2f(0.02f, 0.01f);
    glVertex2f(0.055f, -0.03f);
    glVertex2f(0.08f, -0.06f);
    glVertex2f(0.05f, -0.08f);
    glVertex2f(0.03f, -0.09f);
    glEnd();
    
    glLineWidth(1.0f);
    
    // Moteurs rouges à l'arrière des ailes
    glColor3f(1.0f, 0.2f, 0.0f);
    
    // Moteur gauche
    glBegin(GL_POLYGON);
    glVertex2f(-0.062f, -0.065f);
    glVertex2f(-0.068f, -0.065f);
    glVertex2f(-0.068f, -0.075f);
    glVertex2f(-0.062f, -0.075f);
    glEnd();
    
    // Moteur droit
    glBegin(GL_POLYGON);
    glVertex2f(0.062f, -0.065f);
    glVertex2f(0.068f, -0.065f);
    glVertex2f(0.068f, -0.075f);
    glVertex2f(0.062f, -0.075f);
    glEnd();
    
    // Canon disrupteur (petit point rouge sous le cockpit)
    glColor3f(0.8f, 0.1f, 0.1f);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    glVertex2f(0.0f, 0.07f);
    glEnd();
    glPointSize(1.0f);
}
static void draw_player_ship_top_view(void) {
    // Soucoupe principale (saucer section) - forme elliptique
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.7f, 0.7f, 0.75f);
    glVertex2f(0.0f, 0.0f); // Centre
    
    // Dessiner un cercle aplati (ellipse)
    int segments = 20;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * 3.14159265f;
        float x = cosf(angle) * 0.035f; // Largeur
        float y = sinf(angle) * 0.035f; // Hauteur (plus court)
        glVertex2f(x, y);
    }
    glEnd();
    
    // Bridge/pont (petit dôme au centre de la soucoupe)
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.5f, 0.5f, 0.6f);
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * 3.14159265f;
        float x = cosf(angle) * 0.008f;
        float y = sinf(angle) * 0.006f;
        glVertex2f(x, y);
    }
    glEnd();
    
    // Cou de connexion (neck) - fin et allongé vers l'arrière
    glBegin(GL_QUADS);
    glColor3f(0.6f, 0.6f, 0.65f);
    glVertex2f(-0.004f, -0.025f);  // Haut gauche
    glVertex2f(0.004f, -0.025f);   // Haut droit
    glVertex2f(0.004f, -0.055f);   // Bas droit
    glVertex2f(-0.004f, -0.055f);  // Bas gauche
    glEnd();
    
    // Section d'ingénierie (corps secondaire) - cylindre allongé
    glBegin(GL_QUADS);
    glColor3f(0.65f, 0.65f, 0.7f);
    glVertex2f(-0.012f, -0.050f);
    glVertex2f(0.012f, -0.050f);
    glVertex2f(0.012f, -0.11f);
    glVertex2f(-0.012f, -0.11f);
    glEnd();
    
    
    // Nacelle de distorsion gauche (warp nacelle)
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.5f, 0.6f);
    glVertex2f(-0.042f, -0.05f);   // Avant gauche
    glVertex2f(-0.032f, -0.05f);   // Avant droit
    glVertex2f(-0.032f, -0.13f);   // Arrière droit
    glVertex2f(-0.042f, -0.13f);   // Arrière gauche
    glEnd();
    

    
    // Nacelle de distorsion droite
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.5f, 0.6f);
    glVertex2f(0.038f, -0.05f);
    glVertex2f(0.048f, -0.05f);
    glVertex2f(0.048f, -0.13f);
    glVertex2f(0.038f, -0.13f);
    glEnd();
    
    
    // Détails - lignes de séparation sur la soucoupe
    glColor3f(0.5f, 0.5f, 0.55f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    // Ligne horizontale
    glVertex2f(-0.03f, 0.0f);
    glVertex2f(0.03f, 0.0f);
    // Ligne verticale
    glVertex2f(0.0f, 0.02f);
    glVertex2f(0.0f, -0.02f);
    glEnd();
    
    glLineWidth(1.0f);
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
        if (current->entities->type == ENTITY_PHASER) {
            
            glBegin(GL_QUADS);
            glColor3f(0.0f, 1.0f, 0.0f);
            glVertex2f(-0.01f, -0.02f);
            glVertex2f(0.01f, -0.02f);
            glVertex2f(0.01f, 0.02f);
            glVertex2f(-0.01f, 0.02f);
            glEnd();
            
        } else if (current->entities->type == ENTITY_TYPE_ENEMY) {
            glScalef(0.8f, 0.8f, 0.8f);
            draw_spaceship();   
        }
        glPopMatrix();
        node = node->next;
    }
    glPushMatrix();
    
    glTranslatef(player.x, player.y, 0.0f);
    glRotatef(player.h, 0.0f, 0.0f, 1.0f);
    glPushMatrix();
    draw_player_ship_top_view();
    glPopMatrix();
    glPopMatrix();
}
static void draw_spaceship(void) {
    // =================================================================
    // PARAMÈTRES DE GÉOMÉTRIE DU BIRD OF PREY
    // =================================================================
    
    // Corps central (cou)
    const float neck_width = 0.02f;
    const float neck_front_y = 0.05f;
    const float neck_back_y = -0.05f;
    const float neck_thickness = 0.01f;
    
    // Cockpit (tête en bec)
    const float head_tip_y = 0.1f;
    const float head_base_y = neck_front_y;
    const float head_width = 0.02f;
    const float head_thickness = 0.01f;
    
    // Ailes (geometry principale)
    const float wing_root_x = neck_width;           // Attache au corps
    const float wing_root_y = neck_width;
    const float wing_tip_x = 0.08f;                 // Extrémité externe
    const float wing_tip_y = 0.01f;
    const float wing_tip_z_top = -0.03f;
    const float wing_tip_z_bottom = -0.04f;
    const float wing_mid_x = 0.08f;                 // Point milieu arrière
    const float wing_mid_y = -0.01f;
    const float wing_back_x = 0.03f;                // Pointe arrière
    const float wing_back_y = -0.05f;
    const float wing_thickness_root = 0.01f;
    const float wing_thickness_tip = 0.01f;
    
    // Moteurs
    const float engine_x = 0.065f;
    const float engine_y = -0.07f;
    const float engine_z = -0.035f;
    const float engine_radius = 0.008f;
    
    // Canon disrupteur
    const float cannon_y = 0.08f;
    const float cannon_z = -0.015f;
    const float cannon_scale = 0.01f;
    
    // =================================================================
    // RENDU
    // =================================================================
    
    // Corps central (cou allongé)
    glBegin(GL_QUADS);
    glColor3f(0.3f, 0.4f, 0.3f);
    
    // Dessus du cou
    CALC_NORMAL(-neck_width, neck_front_y, neck_thickness, 
                neck_width, neck_front_y, neck_thickness, 
                neck_width, neck_back_y, neck_thickness);
    glVertex3f(-neck_width, neck_front_y, neck_thickness);
    glVertex3f(neck_width, neck_front_y, neck_thickness);
    glVertex3f(neck_width, neck_back_y, neck_thickness);
    glVertex3f(-neck_width, neck_back_y, neck_thickness);
    
    // Dessous du cou
    glColor3f(0.2f, 0.3f, 0.2f);
    CALC_NORMAL(-neck_width, neck_front_y, -neck_thickness, 
                -neck_width, neck_back_y, -neck_thickness, 
                neck_width, neck_back_y, -neck_thickness);
    glVertex3f(-neck_width, neck_front_y, -neck_thickness);
    glVertex3f(-neck_width, neck_back_y, -neck_thickness);
    glVertex3f(neck_width, neck_back_y, -neck_thickness);
    glVertex3f(neck_width, neck_front_y, -neck_thickness);
    
    // Côtés du cou
    glColor3f(0.25f, 0.35f, 0.25f);
    CALC_NORMAL(-neck_width, neck_front_y, neck_thickness, 
                -neck_width, neck_back_y, neck_thickness, 
                -neck_width, neck_back_y, -neck_thickness);
    glVertex3f(-neck_width, neck_front_y, neck_thickness);
    glVertex3f(-neck_width, neck_back_y, neck_thickness);
    glVertex3f(-neck_width, neck_back_y, -neck_thickness);
    glVertex3f(-neck_width, neck_front_y, -neck_thickness);
    
    CALC_NORMAL(neck_width, neck_front_y, neck_thickness, 
                neck_width, neck_front_y, -neck_thickness, 
                neck_width, neck_back_y, -neck_thickness);
    glVertex3f(neck_width, neck_front_y, neck_thickness);
    glVertex3f(neck_width, neck_front_y, -neck_thickness);
    glVertex3f(neck_width, neck_back_y, -neck_thickness);
    glVertex3f(neck_width, neck_back_y, neck_thickness);
    glEnd();
    
    // Tête/Cockpit (forme de bec rapace)
    glBegin(GL_TRIANGLES);
    glColor3f(0.4f, 0.5f, 0.4f);
    
    // Dessus de la tête - pointe vers l'avant
    CALC_NORMAL(0.0f, head_tip_y, 0.0f, 
                -head_width, head_base_y, head_thickness, 
                head_width, head_base_y, head_thickness);
    glVertex3f(0.0f, head_tip_y, 0.0f);
    glVertex3f(-head_width, head_base_y, head_thickness);
    glVertex3f(head_width, head_base_y, head_thickness);
    
    // Côté gauche de la tête
    CALC_NORMAL(0.0f, head_tip_y, 0.0f, 
                -head_width, head_base_y, -head_thickness, 
                -head_width, head_base_y, head_thickness);
    glVertex3f(0.0f, head_tip_y, 0.0f);
    glVertex3f(-head_width, head_base_y, -head_thickness);
    glVertex3f(-head_width, head_base_y, head_thickness);
    
    // Côté droit de la tête
    CALC_NORMAL(0.0f, head_tip_y, 0.0f, 
                head_width, head_base_y, head_thickness, 
                head_width, head_base_y, -head_thickness);
    glVertex3f(0.0f, head_tip_y, 0.0f);
    glVertex3f(head_width, head_base_y, head_thickness);
    glVertex3f(head_width, head_base_y, -head_thickness);
    
    // Dessous de la tête
    glColor3f(0.3f, 0.4f, 0.3f);
    CALC_NORMAL(0.0f, head_tip_y, 0.0f, 
                head_width, head_base_y, -head_thickness, 
                -head_width, head_base_y, -head_thickness);
    glVertex3f(0.0f, head_tip_y, 0.0f);
    glVertex3f(head_width, head_base_y, -head_thickness);
    glVertex3f(-head_width, head_base_y, -head_thickness);
    glEnd();
    
    // =============== AILE GAUCHE ===============
    glBegin(GL_TRIANGLES);
    glColor3f(0.35f, 0.45f, 0.35f);
    
    // Face supérieure de l'aile gauche
    CALC_NORMAL(-wing_root_x, wing_root_y, 0.0f, 
                -wing_tip_x, wing_tip_y, wing_tip_z_top, 
                -wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    glVertex3f(-wing_root_x, wing_root_y, 0.0f);
    glVertex3f(-wing_tip_x, wing_tip_y, wing_tip_z_top);
    glVertex3f(-wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    
    // Pointe arrière de l'aile gauche
    CALC_NORMAL(-wing_root_x, wing_root_y, 0.0f, 
                -wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip, 
                -wing_back_x, wing_back_y, 0.0f);
    glVertex3f(-wing_root_x, wing_root_y, 0.0f);
    glVertex3f(-wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    glVertex3f(-wing_back_x, wing_back_y, 0.0f);
    
    // Bord avant de l'aile gauche
    glColor3f(0.3f, 0.4f, 0.3f);
    CALC_NORMAL(-wing_root_x, wing_root_y, 0.0f, 
                -wing_back_x, wing_back_y, 0.0f, 
                -wing_root_x, head_base_y, -wing_thickness_root);
    glVertex3f(-wing_root_x, wing_root_y, 0.0f);
    glVertex3f(-wing_back_x, wing_back_y, 0.0f);
    glVertex3f(-wing_root_x, head_base_y, -wing_thickness_root);
    
    // Face inférieure de l'aile gauche
    glColor3f(0.25f, 0.35f, 0.25f);
    CALC_NORMAL(-wing_root_x, wing_root_y, -wing_thickness_root, 
                -wing_mid_x, wing_mid_y, wing_tip_z_bottom, 
                -wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    glVertex3f(-wing_root_x, wing_root_y, -wing_thickness_root);
    glVertex3f(-wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    glVertex3f(-wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    glEnd();
    
    // Bords de l'aile gauche (épaisseur)
    glBegin(GL_QUADS);
    glColor3f(0.28f, 0.38f, 0.28f);
    
    // Bord avant extérieur
    CALC_NORMAL(-wing_root_x, wing_root_y, 0.0f, 
                -wing_tip_x, wing_tip_y, wing_tip_z_top, 
                -wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    glVertex3f(-wing_root_x, wing_root_y, 0.0f);
    glVertex3f(-wing_tip_x, wing_tip_y, wing_tip_z_top);
    glVertex3f(-wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    glVertex3f(-wing_root_x, wing_root_y, -wing_thickness_root);
    
    // Bord milieu
    CALC_NORMAL(-wing_tip_x, wing_tip_y, wing_tip_z_top, 
                -wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip, 
                -wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    glVertex3f(-wing_tip_x, wing_tip_y, wing_tip_z_top);
    glVertex3f(-wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    glVertex3f(-wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    glVertex3f(-wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    
    // Bord arrière
    CALC_NORMAL(-wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip, 
                -wing_back_x, wing_back_y, 0.0f, 
                -wing_back_x, wing_back_y, -wing_thickness_root);
    glVertex3f(-wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    glVertex3f(-wing_back_x, wing_back_y, 0.0f);
    glVertex3f(-wing_back_x, wing_back_y, -wing_thickness_root);
    glVertex3f(-wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    
    // Bord interne avant
    CALC_NORMAL(-wing_back_x, wing_back_y, 0.0f, 
                -wing_root_x, wing_root_y, 0.0f, 
                -wing_root_x, wing_root_y, -wing_thickness_root);
    glVertex3f(-wing_back_x, wing_back_y, 0.0f);
    glVertex3f(-wing_root_x, wing_root_y, 0.0f);
    glVertex3f(-wing_root_x, wing_root_y, -wing_thickness_root);
    glVertex3f(-wing_back_x, wing_back_y, -wing_thickness_root);
    glEnd();
    
    // =============== AILE DROITE (symétrique) ===============
    glBegin(GL_TRIANGLES);
    glColor3f(0.35f, 0.45f, 0.35f);
    
    // Face supérieure de l'aile droite
    CALC_NORMAL(wing_root_x, wing_root_y, 0.0f, 
                wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip, 
                wing_tip_x, wing_tip_y, wing_tip_z_top);
    glVertex3f(wing_root_x, wing_root_y, 0.0f);
    glVertex3f(wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    glVertex3f(wing_tip_x, wing_tip_y, wing_tip_z_top);
    
    // Pointe arrière de l'aile droite
    CALC_NORMAL(wing_root_x, wing_root_y, 0.0f, 
                wing_back_x, wing_back_y, 0.0f, 
                wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    glVertex3f(wing_root_x, wing_root_y, 0.0f);
    glVertex3f(wing_back_x, wing_back_y, 0.0f);
    glVertex3f(wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    
    // Bord avant de l'aile droite
    glColor3f(0.3f, 0.4f, 0.3f);
    CALC_NORMAL(wing_root_x, wing_root_y, 0.0f, 
                wing_root_x, head_base_y, -wing_thickness_root, 
                wing_back_x, wing_back_y, 0.0f);
    glVertex3f(wing_root_x, wing_root_y, 0.0f);
    glVertex3f(wing_root_x, head_base_y, -wing_thickness_root);
    glVertex3f(wing_back_x, wing_back_y, 0.0f);
    
    // Face inférieure de l'aile droite
    glColor3f(0.25f, 0.35f, 0.25f);
    CALC_NORMAL(wing_root_x, wing_root_y, -wing_thickness_root, 
                wing_tip_x, wing_tip_y, wing_tip_z_bottom, 
                wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    glVertex3f(wing_root_x, wing_root_y, -wing_thickness_root);
    glVertex3f(wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    glVertex3f(wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    glEnd();
    
    // Bords de l'aile droite (épaisseur)
    glBegin(GL_QUADS);
    glColor3f(0.28f, 0.38f, 0.28f);
    
    // Bord avant extérieur
    CALC_NORMAL(wing_root_x, wing_root_y, 0.0f, 
                wing_root_x, wing_root_y, -wing_thickness_root, 
                wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    glVertex3f(wing_root_x, wing_root_y, 0.0f);
    glVertex3f(wing_root_x, wing_root_y, -wing_thickness_root);
    glVertex3f(wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    glVertex3f(wing_tip_x, wing_tip_y, wing_tip_z_top);
    
    // Bord milieu
    CALC_NORMAL(wing_tip_x, wing_tip_y, wing_tip_z_top, 
                wing_tip_x, wing_tip_y, wing_tip_z_bottom, 
                wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    glVertex3f(wing_tip_x, wing_tip_y, wing_tip_z_top);
    glVertex3f(wing_tip_x, wing_tip_y, wing_tip_z_bottom);
    glVertex3f(wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    glVertex3f(wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    
    // Bord arrière
    CALC_NORMAL(wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip, 
                wing_mid_x, wing_mid_y, wing_tip_z_bottom, 
                wing_back_x, wing_back_y, -wing_thickness_root);
    glVertex3f(wing_mid_x, wing_mid_y, wing_tip_z_top - wing_thickness_tip);
    glVertex3f(wing_mid_x, wing_mid_y, wing_tip_z_bottom);
    glVertex3f(wing_back_x, wing_back_y, -wing_thickness_root);
    glVertex3f(wing_back_x, wing_back_y, 0.0f);
    
    // Bord interne avant
    CALC_NORMAL(wing_back_x, wing_back_y, 0.0f, 
                wing_back_x, wing_back_y, -wing_thickness_root, 
                wing_root_x, wing_root_y, -wing_thickness_root);
    glVertex3f(wing_back_x, wing_back_y, 0.0f);
    glVertex3f(wing_back_x, wing_back_y, -wing_thickness_root);
    glVertex3f(wing_root_x, wing_root_y, -wing_thickness_root);
    glVertex3f(wing_root_x, wing_root_y, 0.0f);
    glEnd();
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
    glEnable(GL_NORMALIZE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    draw_starfield();
    glEnable(GL_LIGHTING);
    // XY -> plan horizontal
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(-player.h, 0.0f, 0.0f, 1.0f);
    glTranslatef(-player.x, -player.y, 0.0f);
    
    struct entity_list_t *node = entity_list_head;
    while (node != NULL) {
        struct entity_list_t *current = node;
        glPushMatrix();
        glTranslatef(current->entities->x, current->entities->y, 0.0f);
        if (current->entities->type == ENTITY_PHASER) {
            glColor3f(0.0f, 1.0f, 0.0f);
            glScalef(0.002f, 0.002f, 0.001f);
            glutSolidDodecahedron();    
        } else if (current->entities->type == ENTITY_TYPE_ENEMY) {
            glColor3f(1.0f, 0.0f, 0.0f);
            glRotatef(current->entities->h, 0.0f, 0.0f, 1.0f);
            glPushMatrix();
            glScalef(1.2f, 1.4f, 0.8f);
            draw_spaceship();
            glPopMatrix();
        }
        glPopMatrix();
        node = node->next;
    }

    // Crosshair overlay (écran)
    glDisable(GL_LIGHTING);
    glLineWidth(2.0f);
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

    if (player.shield > 0.0f) {
        draw_cracked_screen_overlay(player.shield / 30.0f);
    }
    
}
static void game_loop(double delta_time) {

    if (specialKeyStates[GLUT_KEY_LEFT]) {
        player.h += 2.0f;
    }
    if (specialKeyStates[GLUT_KEY_RIGHT]) {
        player.h -= 2.0f;
    }
    if (specialKeyStates[GLUT_KEY_UP]) {
        player.s = 0.1f;
        if (shiftDown) {
            player.s = 0.4f;
        }
    }
    if (keyStates[32]) {
        shoot(&player, NULL);
    }

    if (player.life <= -100.0f) {
        current_game_state = GAME_STATE_GAME_OVER;
        idlefunc = render_game_over_screen;
        return;
    }
    player.automat(&player, (float)delta_time);
    update_starfield((float)delta_time);
    struct entity_list_t *node = entity_list_head;
    while (node != NULL && entity_list_head != NULL) {
        if (node->entities->automat != NULL) {
            node->entities->automat(node->entities, (float)delta_time);
        }
        node = node->next;
    }
    cleanup_marked_entities();
    if (nb_enemies <= 0) {
        nb_enemies = rand() % 5 + 1;
        sector++;
        init_enemies();
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

static void render_title_screen(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Fond avec étoiles
    glViewport(0, 0, window_width, window_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)window_width / (double)window_height, 0.01, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    draw_starfield();
    
    // Retour en 2D pour le texte
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    
    // Titre principal
    glColor3f(0.0f, 1.0f, 0.0f);
    draw_bitmap_text_wrapped(
        window_width,
        window_height,
        0.25f,
        0.7f,
        0.5f,
        "ENTERPRISE"
    );
    
    // Instructions
    glColor3f(0.8f, 0.8f, 0.8f);
    draw_bitmap_text_wrapped(
        window_width,
        window_height,
        0.2f,
        0.5f,
        0.6f,
        "CONTROLS:\n\nArrow Keys - Steer\nUp Arrow - Thrust\nShift+Up - Boost\nSpace - Fire\nESC - Quit"
    );
    
    // Message de démarrage
    glColor3f(1.0f, 1.0f, 0.0f);
    draw_bitmap_text_wrapped(
        window_width,
        window_height,
        0.3f,
        0.2f,
        0.4f,
        "Press SPACE to start"
    );
    
    glutSwapBuffers();
}
static void reset_game(void) {
    // Réinitialiser le joueur
    player.x = 0.0f;
    player.y = 0.0f;
    player.h = 0.0f;
    player.s = 0.0f;
    player.life = -1.0f;
    player.energy = 0.0f;
    player.shield = 0.0f;
    
    // Nettoyer toutes les entités
    while (entity_list_head != NULL) {
        struct entity_list_t *temp = entity_list_head;
        entity_list_head = entity_list_head->next;
        free(temp->entities);
        free(temp);
    }
    
    // Réinitialiser les variables de jeu
    score = 0;
    sector = 0;
    nb_enemies = 5;
    
    // Réinitialiser les étoiles
    init_stars();
    previous_player_heading = 0.0f;
    
    // Créer les nouveaux ennemis
    init_enemies();
}
static void render_gameplay(void) {
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
    player.automat = player_automaton;
    glEnable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    init_stars();
    init_enemies();
    idlefunc = render_title_screen;
    current_game_state = GAME_STATE_TITLE;
}
static void init_enemies(void) {
    for (int i=0; i<nb_enemies; i++) {
        struct entity_list_t *new_node = (struct entity_list_t *)malloc(sizeof(struct entity_list_t));
        new_node->entities = (entity_t *)malloc(sizeof(entity_t));
        new_node->entities->x = (float)(rand() % 100) / 50.0f - 1.0f;
        new_node->entities->y = (float)(rand() % 100) / 50.0f - 1.0f;
        new_node->entities->h = (float)(rand() % 360);
        new_node->entities->s = 0.0f;
        new_node->entities->m = 1.0f;
        new_node->entities->type = ENTITY_TYPE_ENEMY;
        new_node->entities->life = -1.0f;
        
        new_node->entities->parent = NULL;
        new_node->entities->automat = enemi_automaton;
        new_node->entities->marked_for_removal = 0;
        new_node->next = entity_list_head;
        entity_list_head = new_node;
    }
}
void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    switch (key) {
    case 27: // ESC
        glutLeaveMainLoop();
        break;
    case 32: // SPACE
        if (current_game_state == GAME_STATE_TITLE) {
            current_game_state = GAME_STATE_PLAYING;
            idlefunc = render_gameplay;
            reset_game();
        } else if (current_game_state == GAME_STATE_GAME_OVER) {
            current_game_state = GAME_STATE_PLAYING;
            idlefunc = render_gameplay;
            reset_game();
        } else if (current_game_state == GAME_STATE_PLAYING) {
            keyStates[key] = 1;
        }
        break;
    default:
        keyStates[key] = 1;
        break;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    keyStates[key] = 0;  // Marquer la touche comme relâchée
}

void specialKeys(int key, int x, int y) {
    (void)x;
    (void)y;
    int modifiers = glutGetModifiers();
    altDown = modifiers & GLUT_ACTIVE_ALT;
    shiftDown = modifiers & GLUT_ACTIVE_SHIFT;
    specialKeyStates[key] = 1;
}

void specialKeysUp(int key, int x, int y) {
    (void)x;
    (void)y;
    specialKeyStates[key] = 0;
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
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);

    glutMainLoop();
    return 0;
}