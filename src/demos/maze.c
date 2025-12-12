/*
 *                     OpenGL Maze Example
 *
 *              by  Stan Melax  melax@bioware.com
 *
 * In this little demo the player navigates through a simple maze
 * using the arrow keys.  The maze is defined by a 2D array where
 * each element in the array indicates solid or empty space.  This
 * program wraps polygon (quad) walls around the solid space and
 * disallows the player to navigate into solid space during the demo.
 * Note that all the walls are limited to being 90 degrees to each
 * other - there are no "angled" features.  The purpose of this
 * sample program is to show a beginning 3D game programmer some
 * things they can do.
 *
 * One other cool thing that this program does is that it constucts
 * a single quad strip to draw all the walls by doing a recursive
 * depth first search on the maze array data.
 *
 * Permission to execute this program, or look at the code is only
 * granted to those who do not like to sue other people :-)
 * Some of the window setup code was stolen from a simple Cosmo example.
 * OpenGL is a trademark of SGI.
 */
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
#define IDM_APPLICATION_TEXTURE (102)
#define IDM_APPLICATION_BANK (103)
#define MAZE_HEIGHT (21)
#define MAZE_WIDTH (21)
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)
#define STARTING_POINT_X (1.5f);
#define STARTING_POINT_Y (1.5f);
#define STARTING_HEADING (90.0f);


int enablebank = 0;
int enabletexture = 0;
int enablelighting = 1;
static clock_t last_frame_time = 0;


// unfortunately due to the way the polygon walls are generated there
// are restrictions on what the wall/maze data can look like.  See below.
char mazedata[MAZE_HEIGHT][MAZE_WIDTH];

float player_x = STARTING_POINT_X;
float player_y = STARTING_POINT_Y;
float player_h = STARTING_HEADING; // player's heading
float player_s = 0.0f;             // forward speed of the player
float player_m = 1.0f;             // speed multiplier of the player
float player_t = 0.0f;             // player's turning (change in heading)
float player_b = 0.0f;             // viewpoint bank (roll)
float player_str = 0.0f;           // lateral speed (strafe)
int walllist = 0;
int mazelist = 0;
int groundlist = 0;
int cellinglist = 0;

int player_finished = 0;
int ghost_finished  = 0;
clock_t start_time  = 0;
double player_time  = 0.0;
double ghost_time   = 0.0;


typedef struct mazeobj {
    float x, y;
    float h, s, m, t, b, str;
    enum { TREASURE, MONSTER, EXIT } type;
    void (*think)(struct mazeobj* self);
    int has_left_wall;
    int left_turn_cooldown;
} mazeobj;

mazeobj ghost;

// normal vectors for each wall direction
float nrml[4][3] = {
    {-1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
};
// default color for each wall direction
float clr[4][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f},
};
typedef struct {
    int x, y, width, height;
} Room;
typedef struct {
    int x, y;
} Cell;
static float texcoordX = 0.0f;

void (*idlefunc)(void) = NULL;

void readtexture(void);
void spinmaze(void);
void entermaze(void);
void navmaze(void);
void mapmaze(void);

void moveplayer();
int forward(float px, float py, float bf);

int wall(int x, int y);
int drawtop();
int drawwalls();
static int drawground(void);
static int drawcelling(void);

static void idle(void);
static void menu_callback(int value);
static void keyboard(unsigned char key, int x, int y);
static void special(int key, int x, int y);
static void specialUp(int key, int x, int y);
static void display(void);
static void initGL(void);

int obj_forward(float px, float py, float bf, mazeobj *obj) {
    int x = ((int)obj->x);
    int y = ((int)obj->y);
    int h = 0; // number of walls hit
    if ((px > x + 1.0f - bf) && wall(x + 1, y)) {
        px = (float)(x) + 1.0f - bf;
        h++;
    }
    if (py > y + 1.0f - bf && wall(x, y + 1)) {
        py = (float)(y) + 1.0f - bf;
        h++;
    }
    if (px < x + bf && wall(x - 1, y)) {
        px = (float)(x) + bf;
        h++;
    }
    if (py < y + bf && wall(x, y - 1)) {
        py = (float)(y) + bf;
        h++;
    }
    obj->x = px;
    obj->y = py;
    return h;
}
int sense_wall(float dir_x, float dir_y, float step, mazeobj *self, float bf, float px, float py) {
    float tx = px + dir_x * step;
    float ty = py + dir_y * step;
    mazeobj probe = *self;
    probe.x = px;
    probe.y = py;
    obj_forward(tx, ty, bf, &probe);
    int hit = (fabsf(probe.x - tx) > 1e-4f ||
                fabsf(probe.y - ty) > 1e-4f);
    return hit;
}
void ghost_think(mazeobj *self) {
    const float step_probe = 0.35f;
    const float bf         = 0.2f;

    /* 1) Construire le vecteur "avant" à partir de h, s, str (comme moveplayer) */
    float heading_rad = self->h * 3.1415926f / 180.0f;
    float sinH = sinf(heading_rad);
    float cosH = cosf(heading_rad);

    float dir_x = self->s * sinH + self->str * cosH;
    float dir_y = self->s * cosH - self->str * sinH;
    if (fabsf(dir_x) < 1e-4f && fabsf(dir_y) < 1e-4f) {
        dir_x = sinH;
        dir_y = cosH;
    }
    {
        float len = sqrtf(dir_x * dir_x + dir_y * dir_y);
        if (len > 1e-4f) {
            dir_x /= len;
            dir_y /= len;
        }
    }

    float vx_fwd_x   = dir_x;
    float vx_fwd_y   = dir_y;
    float vx_left_x  = -vx_fwd_y;
    float vx_left_y  =  vx_fwd_x;
    float vx_right_x =  vx_fwd_y;
    float vx_right_y = -vx_fwd_x;

    float px = self->x;
    float py = self->y;

    int wall_left  = sense_wall(vx_left_x,  vx_left_y,  step_probe, self, bf, px, py);
    int wall_front = sense_wall(vx_fwd_x,   vx_fwd_y,   step_probe, self, bf, px, py);
    int wall_right = sense_wall(vx_right_x, vx_right_y, step_probe, self, bf, px, py);

    /* 2) Phase d'accrochage : au centre d'une salle, chercher un mur */
    if (!self->has_left_wall) {
        if (wall_left) {
            /* On vient de trouver un mur à gauche : on s'y accroche
               et on bascule en mode main-gauche permanent. */
            self->has_left_wall = 1;
        } else {
            /* Pas (encore) de mur à gauche: on essaie juste d'avancer. */
            if (wall_front) {
                /* Mur devant: on tourne sur place jusqu'à trouver une ouverture. */
                if (!wall_right) {
                    self->h += 90.0f;
                } else {
                    self->h += 180.0f; /* complètement bloqué: demi-tour */
                }
                if (self->h >= 360.0f) self->h -= 360.0f;
                if (self->h <   0.0f)  self->h += 360.0f;
            }
            /* Avancer dans la direction actuelle (après rotation éventuelle). */
            heading_rad = self->h * 3.1415926f / 180.0f;
            sinH = sinf(heading_rad);
            cosH = cosf(heading_rad);
            float target_x = self->x + self->m * (self->s * sinH + self->str * cosH);
            float target_y = self->y + self->m * (self->s * cosH - self->str * sinH);
            obj_forward(target_x, target_y, bf, self);
            return;
        }
    }

    /* 3) Mode main-gauche permanent : on ne perd plus le mur une fois accroché */

    /* On refait un petit scan dans l'orientation courante (après accrochage) */
    heading_rad = self->h * 3.1415926f / 180.0f;
    sinH = sinf(heading_rad);
    cosH = cosf(heading_rad);

    dir_x = self->s * sinH + self->str * cosH;
    dir_y = self->s * cosH - self->str * sinH;
    if (fabsf(dir_x) < 1e-4f && fabsf(dir_y) < 1e-4f) {
        dir_x = sinH;
        dir_y = cosH;
    }
    {
        float len = sqrtf(dir_x * dir_x + dir_y * dir_y);
        if (len > 1e-4f) {
            dir_x /= len;
            dir_y /= len;
        }
    }
    vx_fwd_x   = dir_x;
    vx_fwd_y   = dir_y;
    vx_left_x  = -vx_fwd_y;
    vx_left_y  =  vx_fwd_x;
    vx_right_x =  vx_fwd_y;
    vx_right_y = -vx_fwd_x;

    px = self->x;
    py = self->y;

    wall_left  = sense_wall(vx_left_x,  vx_left_y,  step_probe, self, bf, px, py);
    wall_front = sense_wall(vx_fwd_x,   vx_fwd_y,   step_probe, self, bf, px, py);
    wall_right = sense_wall(vx_right_x, vx_right_y, step_probe, self, bf, px, py);

    /* Règle main-gauche stricte, avec cooldown de 1 frame après un virage gauche "de sortie" */
    if (self->left_turn_cooldown > 0) {
        /* on vient juste de tourner à gauche pour entrer dans un couloir:
           on consomme le cooldown et on ne re-teste pas wall_left cette frame */
        self->left_turn_cooldown--;
        /* rien à faire sur h: on garde l'orientation actuelle */
    } else {
        if (!wall_left) {
            /* pas de mur à gauche -> on tourne à gauche et on active le cooldown */
            self->h -= 90.0f;
            self->left_turn_cooldown = 10;
        } else if (!wall_front) {
            /* orientation inchangée */
        } else if (!wall_right) {
            self->h += 90.0f;
        } else {
            self->h += 180.0f;
        }
    }

    if (self->h >= 360.0f) self->h -= 360.0f;
    if (self->h <   0.0f)  self->h += 360.0f;

    heading_rad = self->h * 3.1415926f / 180.0f;
    sinH = sinf(heading_rad);
    cosH = cosf(heading_rad);
    float target_x = self->x + self->m * (self->s * sinH + self->str * cosH);
    float target_y = self->y + self->m * (self->s * cosH - self->str * sinH);
    obj_forward(target_x, target_y, bf, self);
}
void generate_random_maze(void) {
    int x, y;
    int r, i, rx, ry;
    int dir, d;
    Cell stack[MAZE_WIDTH * MAZE_HEIGHT];
    int stack_size = 0;
    Room rooms[10];
    int num_rooms = 0;
    int visited[MAZE_HEIGHT][MAZE_WIDTH];
    int max_rooms = 3 + (rand() % 5);
    int current_x = 1;
    int current_y = 1;
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};
    int exit_side = rand() % 4;
    int exit_pos;

    player_finished = 0;
    ghost_finished  = 0;
    player_time = 0.0;
    ghost_time  = 0.0;
    start_time = clock();

    for (y = 0; y < MAZE_HEIGHT; y++) {
        for (x = 0; x < MAZE_WIDTH; x++) {
            mazedata[y][x] = '*';
        }
    }
    
    
    for (y = 0; y < MAZE_HEIGHT; y++) {
        for (x = 0; x < MAZE_WIDTH; x++) {
            visited[y][x] = 0;
        }
    }
    
    for ( r = 0; r < max_rooms && num_rooms < 10; r++) {
        int room_width = 3 + (rand() % 3) * 2;   // 3, 5 ou 7
        int room_height = 3 + (rand() % 3) * 2;  // 3, 5 ou 7

        int room_x = 1 + (rand() % ((MAZE_WIDTH - room_width - 2) / 2)) * 2;
        int room_y = 1 + (rand() % ((MAZE_HEIGHT - room_height - 2) / 2)) * 2;
        int overlap = 0;

        if (room_x + room_width >= MAZE_WIDTH - 1 || room_y + room_height >= MAZE_HEIGHT - 1)
            continue;

        
        for ( i = 0; i < num_rooms; i++) {
            if (!(room_x + room_width + 2 < rooms[i].x || 
                  room_x > rooms[i].x + rooms[i].width + 2 ||
                  room_y + room_height + 2 < rooms[i].y || 
                  room_y > rooms[i].y + rooms[i].height + 2)) {
                overlap = 1;
                break;
            }
        }
        
        if (overlap)
            continue;
        
        rooms[num_rooms].x = room_x;
        rooms[num_rooms].y = room_y;
        rooms[num_rooms].width = room_width;
        rooms[num_rooms].height = room_height;
        num_rooms++;

        for ( ry = room_y; ry < room_y + room_height; ry++) {
            for ( rx = room_x; rx < room_x + room_width; rx++) {
                mazedata[ry][rx] = ' ';
                visited[ry][rx] = 1;
            }
        }
    }
    
    int monster_start_room = rand() % num_rooms;
    rand();
    int monster_current_x = rooms[monster_start_room].x + rooms[monster_start_room].width / 2;
    int monster_current_y = rooms[monster_start_room].y + rooms[monster_start_room].height / 2;
    
    ghost.x = (float)monster_current_x;
    ghost.y = (float)monster_current_y;
    ghost.b = 0.0f;
    ghost.h = 0.0f;
    ghost.s = 0.1f;
    ghost.m = 0.5f;
    ghost.str = 0.0f;
    ghost.t = 0.0f;
    ghost.think = ghost_think;
    ghost.type = MONSTER;
    ghost.has_left_wall = 0;
    ghost.left_turn_cooldown = 0;

    mazedata[current_y][current_x] = ' ';
    visited[current_y][current_x] = 1;
    
    stack[stack_size++] = (Cell){current_x, current_y};
    
    ghost.x = STARTING_POINT_X;
    ghost.y = STARTING_POINT_Y;
    player_x = STARTING_POINT_X;
    player_y = STARTING_POINT_Y;
    
    while (stack_size > 0) {
        current_x = stack[stack_size - 1].x;
        current_y = stack[stack_size - 1].y;

        int neighbors[4];
        int neighbor_count = 0;
        
        for ( dir = 0; dir < 4; dir++) {
            int nx = current_x + dx[dir] * 2;
            int ny = current_y + dy[dir] * 2;

            if (nx >= 1 && nx < MAZE_WIDTH - 1 && 
                ny >= 1 && ny < MAZE_HEIGHT - 1 && 
                !visited[ny][nx]) {
                neighbors[neighbor_count++] = dir;
            }
        }
        
        if (neighbor_count > 0) {
            int chosen_dir = neighbors[rand() % neighbor_count];

            int wall_x = current_x + dx[chosen_dir];
            int wall_y = current_y + dy[chosen_dir];
            int next_x = current_x + dx[chosen_dir] * 2;
            int next_y = current_y + dy[chosen_dir] * 2;
            
            mazedata[wall_y][wall_x] = ' ';
            mazedata[next_y][next_x] = ' ';
            visited[next_y][next_x] = 1;

            stack[stack_size++] = (Cell){next_x, next_y};
        } else {
            stack_size--;
        }
    }

    for ( r = 0; r < num_rooms; r++) {
        int num_doors = 1 + (rand() % 3); // 1 à 3 portes
        
        for ( d = 0; d < num_doors; d++) {
            int side = rand() % 4;
            int door_x, door_y;
            
            switch (side) {
                case 0: // Haut
                    door_x = rooms[r].x + 1 + (rand() % (rooms[r].width - 2));
                    if (door_x % 2 == 0) door_x--;
                    door_y = rooms[r].y - 1;
                    if (door_y >= 1) {
                        mazedata[door_y][door_x] = ' ';
                    }
                    break;
                    
                case 1: // Droite
                    door_x = rooms[r].x + rooms[r].width;
                    door_y = rooms[r].y + 1 + (rand() % (rooms[r].height - 2));
                    if (door_y % 2 == 0) door_y--;
                    if (door_x < MAZE_WIDTH - 1) {
                        mazedata[door_y][door_x] = ' ';
                    }
                    break;
                    
                case 2: // Bas
                    door_x = rooms[r].x + 1 + (rand() % (rooms[r].width - 2));
                    if (door_x % 2 == 0) door_x--;
                    door_y = rooms[r].y + rooms[r].height;
                    if (door_y < MAZE_HEIGHT - 1) {
                        mazedata[door_y][door_x] = ' ';
                    }
                    break;
                    
                case 3: // Gauche
                    door_x = rooms[r].x - 1;
                    door_y = rooms[r].y + 1 + (rand() % (rooms[r].height - 2));
                    if (door_y % 2 == 0) door_y--;
                    if (door_x >= 1) {
                        mazedata[door_y][door_x] = ' ';
                    }
                    break;
            }
        }
    }
    
    mazedata[1][1] = ' ';

    switch (exit_side) {
        case 0: // Bord haut (y = 0)
            exit_pos = 1 + (rand() % (MAZE_WIDTH - 2));
            if (exit_pos % 2 == 0) exit_pos--; // Assurer une position impaire
            mazedata[0][exit_pos] = ' ';
            if (exit_pos < MAZE_WIDTH) mazedata[1][exit_pos] = ' ';
            break;
            
        case 1: // Bord droit (x = MAZE_WIDTH - 1)
            exit_pos = 1 + (rand() % (MAZE_HEIGHT - 2));
            if (exit_pos % 2 == 0) exit_pos--; // Assurer une position impaire
            mazedata[exit_pos][MAZE_WIDTH - 1] = ' ';
            if (MAZE_WIDTH >= 2) mazedata[exit_pos][MAZE_WIDTH - 2] = ' ';
            break;
            
        case 2: // Bord bas (y = MAZE_HEIGHT - 1)
            exit_pos = 1 + (rand() % (MAZE_WIDTH - 2));
            if (exit_pos % 2 == 0) exit_pos--; // Assurer une position impaire
            mazedata[MAZE_HEIGHT - 1][exit_pos] = ' ';
            if (MAZE_HEIGHT >= 2) mazedata[MAZE_HEIGHT - 2][exit_pos] = ' ';
            break;
            
        case 3: // Bord gauche (x = 0)
            exit_pos = 1 + (rand() % (MAZE_HEIGHT - 2));
            if (exit_pos % 2 == 0) exit_pos--; // Assurer une position impaire
            mazedata[exit_pos][0] = ' ';
            mazedata[exit_pos][1] = ' ';
            break;
    }
}
void readtexture() {
    unsigned char *image;
    int rc;
    // the bitmap must be a 24 bit bmp file thats 128x128
    // I think i mixed up red and blue componants - oh well :-)
    FILE *fp;
    fp = fopen("maze.bmp", "rb");
    if (!fp)
        return;
    fseek(fp, 54, SEEK_SET);
    image = (unsigned char *)malloc(128 * 128 * 3);
    assert(image);
    rc = fread(image, sizeof(unsigned char), 128 * 128 * 3, fp);
    assert(rc == 128 * 128 * 3);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    // let texture wrap-around
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // use point sampleing (fastest)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_TEXTURE_2D);

    fclose(fp);
}
static void menu_callback(int value) {
    switch (value) {
    case IDM_APPLICATION_EXIT:
        glutLeaveMainLoop();
        break;
    case IDM_APPLICATION_TEXTURE:
        enabletexture ^= 1;
        (enabletexture ? glEnable : glDisable)(GL_TEXTURE_2D);
        if (enabletexture)
            readtexture();
        break;
    case IDM_APPLICATION_BANK:
        enablebank ^= 1;
        break;
    default:
        break;
    }
}
int wall(int x, int y) {
    // true if the region at x,y is solid space that
    // should be surrounded by other solid space or polygon walls
    return (x >= 0 && y >= 0 && x < MAZE_WIDTH && y < MAZE_HEIGHT && ' ' != mazedata[y][x]);
}

int drawwalls() {
    int x, y, dl;
    glNewList(dl = glGenLists(1), GL_COMPILE);
    
    texcoordX = 0.0f;
    for (y = 0; y < MAZE_HEIGHT; y++) {
        for (x = 0; x < MAZE_WIDTH; x++) {
            
            if (wall(x, y)) {
                glBegin(GL_QUADS);
                // left side
                glNormal3fv(nrml[0]);
                glColor3fv(clr[0]);
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(x + 0.0f, y + 0.0f, 0.0f);
                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(x + 0.0f, y + 0.0f, 1.0f);
                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(x + 0.0f, y + 1.0f, 1.0f);
                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(x + 0.0f, y + 1.0f, 0.0f);
                // right side
                glNormal3fv(nrml[2]);
                glColor3fv(clr[2]);
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(x + 1.0f, y + 0.0f, 0.0f);
                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(x + 1.0f, y + 1.0f, 0.0f);
                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 1.0f, 1.0f);
                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 0.0f, 1.0f);
                // front side
                glNormal3fv(nrml[1]);
                glColor3fv(clr[1]);
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(x + 0.0f, y + 1.0f, 0.0f);
                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(x + 0.0f, y + 1.0f, 1.0f);
                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 1.0f, 1.0f);
                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 1.0f, 0.0f);
                // back side
                glNormal3fv(nrml[3]);
                glColor3fv(clr[3]);
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(x + 0.0f, y + 0.0f, 0.0f);
                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(x + 1.0f, y + 0.0f, 0.0f);
                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 0.0f, 1.0f);
                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(x + 0.0f, y + 0.0f, 1.0f);
                glEnd();
            }
        }
    }
    glEndList();
    return dl;
}
int drawtop() {
    // draws the top and the bottom of the maze
    // which is useful for overhead views
    // The display list created here is only used for the intro
    // spinning bit.  No optimizations such as using quad strips
    // or combining adjacent polygons are done here.
    int x, y, dl;
    glNewList(dl = glGenLists(1), GL_COMPILE);
    glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    for (y = 0; y < MAZE_HEIGHT; y++) {
        for (x = 0; x < MAZE_WIDTH; x++) {
            if (wall(x, y)) {
                // bottomside:
                glVertex3f(x + 0.0f, y + 0.0f, 0.0f);
                glVertex3f(x + 0.0f, y + 1.0f, 0.0f);
                glVertex3f(x + 1.0f, y + 1.0f, 0.0f);
                glVertex3f(x + 1.0f, y + 0.0f, 0.0f);
                // topside:
                glVertex3f(x + 0.0f, y + 0.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 0.0f, 1.0f);
                glVertex3f(x + 1.0f, y + 1.0f, 1.0f);
                glVertex3f(x + 0.0f, y + 1.0f, 1.0f);
            }
        }
    }
    glEnd();
    glPopAttrib();
    glEndList();
    return (dl);
}
static int drawground(void) {
    int dl;
    int x, y;
    glNewList(dl = glGenLists(1), GL_COMPILE);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glColor3f(1.0f, 0.6f, 0.0f);
    for ( y = 0; y < MAZE_HEIGHT; ++y) {
        for ( x = 0; x < MAZE_WIDTH; ++x) {
            float x0 = (float)x,     x1 = x0 + 1.0f;
            float y0 = (float)y,     y1 = y0 + 1.0f;

            glTexCoord2f(x0, y0); glVertex3f(x0, y0, 0.0f);
            glTexCoord2f(x1, y0); glVertex3f(x1, y0, 0.0f);
            glTexCoord2f(x1, y1); glVertex3f(x1, y1, 0.0f);
            glTexCoord2f(x0, y1); glVertex3f(x0, y1, 0.0f);
        }
    }
    glEnd();
    glEndList();
    return dl;
}
static int drawcelling(void) {
    int dl;
    int x, y;
    glNewList(dl = glGenLists(1), GL_COMPILE);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glColor3f(0.6f, 0.6f, 1.0f);
    for ( y = 0; y < MAZE_HEIGHT; ++y) {
        for ( x = 0; x < MAZE_WIDTH; ++x) {
            float x0 = (float)x,     x1 = x0 + 1.0f;
            float y0 = (float)y,     y1 = y0 + 1.0f;

            glTexCoord2f(x0, y0); glVertex3f(x0, y0, 1.0f);
            glTexCoord2f(x0, y1); glVertex3f(x0, y1, 1.0f);
            glTexCoord2f(x1, y1); glVertex3f(x1, y1, 1.0f);
            glTexCoord2f(x1, y0); glVertex3f(x1, y0, 1.0f);
        }
    }
    glEnd();
    glEndList();
    return dl;
}

void moveplayer() {
    float heading_rad = player_h * 3.1415926f / 180.0f;
    float sinH = sinf(heading_rad);
    float cosH = cosf(heading_rad);
    float target_x = player_x + player_m * (player_s * sinH + player_str * cosH);
    float target_y = player_y + player_m * (player_s * cosH - player_str * sinH);
    player_h += player_t;
    forward(target_x, target_y, 0.2f);
}
int forward(float px, float py, float bf) {
    // this routine does wall collision detection
    // the inputs to this routine are:
    //         - the desired location
    //         - the minimum distance to wall allowed
    // changes:
    //         - the player's x and y coordinates
    // returns:
    //         - whether a wall caused change in target position
    // This is really easy with these walls that lie only on axes.
    // If the player collides into a wall at an angle he/she will
    // still slide along the wall - I hate programs where you stick
    // to the polygon and cant move until you back away from it.
    // This collision detection isn't perfect - if you're precise you
    // can jump through at a corner - but its tough.
    int x = ((int)player_x);
    int y = ((int)player_y);
    int h = 0; // number of walls hit
    if ((px > x + 1.0f - bf) && wall(x + 1, y)) {
        px = (float)(x) + 1.0f - bf;
        h++;
    }
    if (py > y + 1.0f - bf && wall(x, y + 1)) {
        py = (float)(y) + 1.0f - bf;
        h++;
    }
    if (px < x + bf && wall(x - 1, y)) {
        px = (float)(x) + bf;
        h++;
    }
    if (py < y + bf && wall(x, y - 1)) {
        py = (float)(y) + bf;
        h++;
    }
    player_x = px;
    player_y = py;
    return h;
}
void spinmaze(void) {
    static float spin = 720.0f;
    spin -= 5.0f; // TODO: lier au temps si tu veux
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -20.0f);
    glRotatef(spin, 0.0f, 1.0f, 1.0f);
    
    glTranslatef(-MAZE_WIDTH / 2.0f, -MAZE_HEIGHT / 2.0f, 0.0f);
    glCallList(groundlist);
    glCallList(walllist);
    glCallList(mazelist);
    glTranslatef(player_x, player_y, 0.5f);
    GLfloat light_position[] = {0, 0, 0.5f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION,       light_position);
    glutSolidCube(0.5f);
    glPopMatrix();
    glutSwapBuffers();

    if (spin <= 0.0f) {
        spin = 720.0f;
        idlefunc = entermaze;
        player_x = STARTING_POINT_X;
        player_y = STARTING_POINT_Y;
        player_h = STARTING_HEADING;
    }
}
void mapmaze(void) {
    static float spin = 720.0f;
    
    moveplayer();
    ghost.think(&ghost);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -20.0f);
    glRotatef(spin, 0.0f, 1.0f, 1.0f);
    
    glTranslatef(-MAZE_WIDTH / 2.0f, -MAZE_HEIGHT / 2.0f, 0.0f);
    glCallList(groundlist);
    glCallList(walllist);
    glCallList(mazelist);
    glPushMatrix();
    glTranslatef(ghost.x, ghost.y, 0.5f);
    glutSolidCube(0.5f);
    glRotatef(ghost.h, 0.0f, 0.0f, 1.0f);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);
    glEnd();
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glTranslatef(player_x, player_y, 0.5f);
    GLfloat light_position[] = {0, 0, 0.5f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION,       light_position);
    glutSolidCube(0.5f);
    glRotatef(player_h, 0.0f, 0.0f, 1.0f);
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 2.0f, 0.0f);
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glutSwapBuffers();

    if (spin <= 0.0f) {
        spin = 720.0f;
        idlefunc = entermaze;
        player_x = STARTING_POINT_X;
        player_y = STARTING_POINT_Y;
        player_h = STARTING_HEADING;
    }
}

void entermaze(void) {
    static float p = 0.0f;
    p += 0.02f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glPushMatrix();
    glRotatef(-90.0f * p, 1.0f, 0.0f, 0.0f);
    glRotatef(player_h * p, 0.0f, 0.0f, 1.0f);
    glTranslatef(-(player_x * p + MAZE_WIDTH / 2.0f * (1 - p)), -(player_y * p + MAZE_HEIGHT / 2.0f * (1 - p)),
                 -(0.5f * p + 20.0f * (1 - p)));
    glCallList(walllist);
    glCallList(mazelist);
    glPopMatrix();
    glutSwapBuffers();

    if (p >= 1.0f) {
        p = 0.0f;
        idlefunc = navmaze;
    }
}

void navmaze(void) {
    moveplayer();
    ghost.think(&ghost);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    if (enablebank)
        glRotatef(-player_b, 0.0f, 1.0f, 0.0f);
    glRotatef(player_h, 0.0f, 0.0f, 1.0f);
    glTranslatef(-player_x, -player_y, -0.5f);

    GLfloat light_position[] = {player_x, player_y, 0.5f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glCallList(groundlist);
    glCallList(cellinglist);
    glCallList(walllist);
    
    glPushMatrix();
    glTranslatef(ghost.x, ghost.y, 0.5f);
    glutSolidCube(0.5f);
    glPopMatrix();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthFunc(GL_EQUAL);
    glDepthMask(GL_FALSE);

    
    GLfloat light_position_ghost[] = {ghost.x, ghost.y, 0.5f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position_ghost);

    glCallList(groundlist);
    glCallList(cellinglist);
    glCallList(walllist);
    
    glPushMatrix();
    glTranslatef(ghost.x, ghost.y, 0.5f);
    glutSolidCube(0.5f);
    glPopMatrix();

    glDisable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glPopMatrix();
    glutSwapBuffers();

    clock_t now = clock();
    double elapsed = (double)(now - start_time) / CLOCKS_PER_SEC;

    if (!player_finished &&
        (player_x > MAZE_WIDTH || player_y > MAZE_HEIGHT || player_x < 0 || player_y < 0)) {
        player_finished = 1;
        player_time = elapsed;
    }

    if (!ghost_finished &&
        (ghost.x > MAZE_WIDTH || ghost.y > MAZE_HEIGHT || ghost.x < 0 || ghost.y < 0)) {
        ghost_finished = 1;
        ghost_time = elapsed;
    }
    if (player_finished && ghost_finished) {
        printf("Course terminee ! Joueur: %.2fs, Fantome: %.2fs -> %s gagne.\n",
            player_time, ghost_time,
            (player_time < ghost_time) ? "Joueur" :
            (ghost_time < player_time) ? "Fantome" : "Egalite");

        generate_random_maze();
        glDeleteLists(walllist, 1);
        walllist = drawwalls();
        glDeleteLists(mazelist, 1);
        mazelist = drawtop();

        start_time = clock();
        player_finished = ghost_finished = 0;
        player_time = ghost_time = 0.0;

        idlefunc = spinmaze;
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
    case 'b':
    case 'B':
        enablebank ^= 1;
        break;
    case 't':
    case 'T':
        enabletexture ^= 1;
        (enabletexture ? glEnable : glDisable)(GL_TEXTURE_2D);
        if (enabletexture)
            readtexture();
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
    case GLUT_KEY_LEFT:
        if (altDown) {
            player_str = -0.05f;
            player_t = 0.0f;
        } else {
            player_t = -5.0f;
            player_str = 0.0f;
        }
        break;
    case GLUT_KEY_RIGHT:
        if (altDown) {
            player_str = 0.05f;
            player_t = 0.0f;
        } else {
            player_t = 5.0f;
            player_str = 0.0f;
        }
        break;
    case GLUT_KEY_UP:
        player_s = 0.05f;
        break;
    case GLUT_KEY_DOWN:
        player_s = -0.02f;
        break;
    case GLUT_KEY_SHIFT_L:
    case GLUT_KEY_SHIFT_R:
        player_m = 3.0f;
        break;
    case GLUT_KEY_F1:
        idlefunc = mapmaze;
        break;
    case GLUT_KEY_F2:
        idlefunc = navmaze;
        break;
    case GLUT_KEY_F3:
        enablelighting = !enablelighting;
        if (enablelighting) {
            glEnable(GL_LIGHTING);
        } else {
            glDisable(GL_LIGHTING);
        }
        break;
    default:
        break;
    }
}

static void specialUp(int key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
    case GLUT_KEY_LEFT:
        if (player_t < 0.0f)
            player_t = 0.0f;
        if (player_str < 0.0f)
            player_str = 0.0f;
        break;
    case GLUT_KEY_RIGHT:
        if (player_t > 0.0f)
            player_t = 0.0f;
        if (player_str > 0.0f)
            player_str = 0.0f;
        break;
    case GLUT_KEY_UP:
    case GLUT_KEY_DOWN:
        player_s = 0.0f;
        break;
    case GLUT_KEY_SHIFT_L:
    case GLUT_KEY_SHIFT_R:
        player_m = 1.0f;
        break;
    default:
        break;
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
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);
    glEnable(GL_NORMALIZE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat light_ambient[]  = {0.05f, 0.05f, 0.05f, 1.0f};
    GLfloat light_diffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat light_specular[] = {0.0f, 0.0f, 0.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    // position/direction par défaut (seront recalculées dans navmaze)
    GLfloat light_position[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION,       light_position);
    
    // atténuation un peu plus douce
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION,  0.8f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.3f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.1f);
    // Matériau de base neutre
    GLfloat mat_diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat mat_ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    walllist = drawwalls();
    mazelist = drawtop();
    groundlist = drawground();
    cellinglist = drawcelling();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 1.0, 0.1, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    idlefunc = spinmaze;
}
int main(int argc, char **argv) {
    glutInit(&argc, argv);
    // Double buffer + RGB + z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Maze Example (FreeGLUT)");

    int menu = glutCreateMenu(menu_callback);
    glutAddMenuEntry("Exit", IDM_APPLICATION_EXIT);
    glutAddMenuEntry("Add/Remove Texture", IDM_APPLICATION_TEXTURE);
    glutAddMenuEntry("Add/Remove Banking", IDM_APPLICATION_BANK);

    // bouton droit de la souris = menu
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    generate_random_maze();
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