
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum game_state { GAME_STATE_TITLE, GAME_STATE_PLAYING, GAME_STATE_GAME_OVER };

typedef struct {
    int type; // 0=rock, 1=tree
    float pos;
    float size;
} Sprite;

typedef struct {
    float height;
    float curve;
    Sprite *sprite; // NULL if no sprite
} RoadSegment;

typedef struct {
    float maxHeight;
    float maxCurve;
    int length;
    float curvy;
    float mountainy;
    int zoneSize;
} RoadParam;

typedef struct {
    RoadSegment *segments;
    int count;
    RoadParam params;
} Road;

static enum game_state current_game_state = GAME_STATE_TITLE;

typedef struct {
    float z;       // Position le long de la route
    float x;       // Position latérale (offset par rapport au centre)
    float speed;   // Vitesse
    float r, g, b; // Couleur
} TrafficCar;

#define MAX_TRAFFIC 20
#define CAR_LENGTH 20.0f // Longueur de la voiture
#define CAR_WIDTH 15.0f  // Largeur de la voiture

#define TARGET_FPS 60
#define FRAME_TIME_MS (1000.0 / TARGET_FPS)

TrafficCar traffic[MAX_TRAFFIC];

void displayIntro(void);
void displayManual(void);
void (*idlefunc)(void) = NULL;

Road *road = NULL;
float cameraPosition = 0.0f;
float cameraHeight = 40.0f;
int segmentSize = 5;

float currentCamX = 0.0f;
float currentCamY = 0.0f;

float playerX = 0.0f;     // Position latérale de la voiture (-roadWidth à +roadWidth)
float playerSpeed = 0.0f; // Vitesse actuelle
float maxSpeed = 8.0f;    // Vitesse maximale
float maxRoadSpeed = 8.0f;
float maxOffRoadSpeed = 2.0f;
float acceleration = 0.15f;    // Accélération
float deceleration = 0.1f;     // Décélération
float braking = 0.2f;          // Freinage
float offRoadDecel = 0.5f;     // Décélération hors route
float turnSpeed = 5.0f;        // Vitesse de virage
float centrifugalForce = 0.8f; // Force centrifuge
float backgroundOffset = 0.0f; // Décalage horizontal du décor
float centrifugal = 0.0f;
int keyStates[256] = {0};        // État des touches normales
int specialKeyStates[256] = {0}; // État des touches spéciales (flèches)
int turning = 0;                 // 0=aucun, -1=gauche, 1=droite
float roadX = 0.0f;              // position X actuelle de la route
int offRoad = 0;                 // 1 si la voiture est hors de la route, 0 sinon
float roadWidth = 50.0f;
float borderWidth = 3.0f;
float grassWidth = 2000.0f;

float lapTime = 0.0f;
float currentLapTime = 0.0f;
float bestLapTime = 0.0f;
static clock_t last_frame_time = 0;

static float randf();
static void drawPyramid();
static void initTraffic();
static void drawTrafficCar(TrafficCar *car);
static void drawBackground(float curveOffset);
static void drawText(float x, float y, const char *text);
static Road *generateRoad(RoadParam params);
static void freeRoad(Road *road);
static void drawRoadSegment(int index, float zPos);
static void drawPlayerCar(void);
static void keyboard(unsigned char key, int x, int y);
static void keyboardUp(unsigned char key, int x, int y);
static void specialKeys(int key, int x, int y);
static void specialKeysUp(int key, int x, int y);
static void displayManual(void);
static void displayIntro(void);
static int checkCollision(float pZ, float pX, float tZ, float tX);
static void game_loop(double delta_time);
static void display(void);
static void idle(void);
static void reshape(int w, int h);
static float randf() { return (float)rand() / (float)RAND_MAX; }
static void drawPyramid() {
// Fonction helper pour calculer et définir la normale d'un triangle
#define CALC_AND_SET_NORMAL(v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z)                                               \
    {                                                                                                                  \
        float ux = v2x - v1x, uy = v2y - v1y, uz = v2z - v1z;                                                          \
        float vx = v3x - v1x, vy = v3y - v1y, vz = v3z - v1z;                                                          \
        float nx = uy * vz - uz * vy;                                                                                  \
        float ny = uz * vx - ux * vz;                                                                                  \
        float nz = ux * vy - uy * vx;                                                                                  \
        float len = sqrtf(nx * nx + ny * ny + nz * nz);                                                                \
        if (len > 0.0001f) {                                                                                           \
            nx /= len;                                                                                                 \
            ny /= len;                                                                                                 \
            nz /= len;                                                                                                 \
        }                                                                                                              \
        glNormal3f(nx, ny, nz);                                                                                        \
    }

    glBegin(GL_TRIANGLES);

    // Face Avant (face +Z)
    CALC_AND_SET_NORMAL(0.0f, 10.0f, 0.0f, -5.0f, 0.0f, 5.0f, 5.0f, 0.0f, 5.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    glVertex3f(-5.0f, 0.0f, 5.0f);
    glVertex3f(5.0f, 0.0f, 5.0f);

    // Face Droite (face +X)
    CALC_AND_SET_NORMAL(0.0f, 10.0f, 0.0f, 5.0f, 0.0f, 5.0f, 5.0f, 0.0f, -5.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    glVertex3f(5.0f, 0.0f, 5.0f);
    glVertex3f(5.0f, 0.0f, -5.0f);

    // Face Arrière (face -Z)
    CALC_AND_SET_NORMAL(0.0f, 10.0f, 0.0f, 5.0f, 0.0f, -5.0f, -5.0f, 0.0f, -5.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    glVertex3f(5.0f, 0.0f, -5.0f);
    glVertex3f(-5.0f, 0.0f, -5.0f);

    // Face Gauche (face -X)
    CALC_AND_SET_NORMAL(0.0f, 10.0f, 0.0f, -5.0f, 0.0f, -5.0f, -5.0f, 0.0f, 5.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    glVertex3f(-5.0f, 0.0f, -5.0f);
    glVertex3f(-5.0f, 0.0f, 5.0f);

    glEnd();

    // Base (carré) - normale pointant vers le bas
    glBegin(GL_QUADS);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-5.0f, 0.0f, 5.0f);
    glVertex3f(5.0f, 0.0f, 5.0f);
    glVertex3f(5.0f, 0.0f, -5.0f);
    glVertex3f(-5.0f, 0.0f, -5.0f);
    glEnd();

#undef CALC_AND_SET_NORMAL
}
static void initTraffic() {
    float totalLength = road->count * segmentSize;
    for (int i = 0; i < MAX_TRAFFIC; i++) {
        traffic[i].z = randf() * totalLength;

        // Placer sur une voie aléatoire (gauche ou droite)
        // roadWidth est 50, on les place à +/- 25
        traffic[i].x = (randf() > 0.5f ? 1.0f : -1.0f) * (roadWidth * 0.5f);

        // Vitesse aléatoire (plus lente que le joueur max)
        traffic[i].speed = 2.0f + randf() * 3.0f;

        // Couleur aléatoire
        traffic[i].r = randf();
        traffic[i].g = randf();
        traffic[i].b = randf();
    }
}
static void drawTrafficCar(TrafficCar *car) {
    float maxTrack = road->count * segmentSize;
    float currentZ = car->z;

    // Gestion du bouclage pour l'affichage
    // Si la voiture est au début du circuit (ex: 10) et la caméra à la fin (ex: 9000)
    // On veut dessiner la voiture à 9010, pas à 10.
    if (currentZ < cameraPosition - 500.0f)
        currentZ += maxTrack;

    // Si la voiture est loin derrière, on ne dessine pas
    if (currentZ < cameraPosition - 20.0f)
        return;
    // Si trop loin devant
    if (currentZ > cameraPosition + 150.0f * segmentSize)
        return;

    // Position Z relative à la caméra (pour OpenGL)
    float zPos = currentZ - cameraPosition;

    // Trouver le segment (sur la vraie position Z de la voiture)
    int carSegment = (int)(car->z / segmentSize);
    float carSegmentProgress = (car->z - carSegment * segmentSize) / segmentSize;

    RoadSegment *carCurrentSeg = &road->segments[carSegment % road->count];
    RoadSegment *carNextSeg = &road->segments[(carSegment + 1) % road->count];

    float currentRoadX = carCurrentSeg->curve + (carNextSeg->curve - carCurrentSeg->curve) * carSegmentProgress;
    float currentRoadY = carCurrentSeg->height + (carNextSeg->height - carCurrentSeg->height) * carSegmentProgress;

    float heightDelta = carNextSeg->height - carCurrentSeg->height;
    float slopeAngle = atan2f(heightDelta, segmentSize) * 180.0f / M_PI;

    float curveDelta = carNextSeg->curve - carCurrentSeg->curve;
    float turnAngle = atan2f(curveDelta, segmentSize) * 180.0f / M_PI;

    glPushMatrix();
    // On dessine à la position X, Y de la route, et Z relatif
    glTranslatef(currentRoadX + car->x, currentRoadY, zPos);

    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-turnAngle, 0.0f, 1.0f, 0.0f);
    glRotatef(slopeAngle, 1.0f, 0.0f, 0.0f);

    glColor3f(car->r, car->g, car->b);
    glTranslatef(0.0f, 5.0f, 0.0f);
    glutSolidCube(12.0f);

    glPopMatrix();
}
static void drawBackground(float curveOffset) {
    // Sauvegarder l'état de l'éclairage et du depth test
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Sauvegarder la matrice
    glPushMatrix();
    glLoadIdentity();

    // Calculer le pitch pour que le fond suive la caméra
    int currentSegment = (int)(cameraPosition / segmentSize);
    float segmentProgress = (cameraPosition - currentSegment * segmentSize) / segmentSize;
    RoadSegment *currentSeg = &road->segments[currentSegment % road->count];
    RoadSegment *nextSeg = &road->segments[(currentSegment + 1) % road->count];
    float road_climb = (nextSeg->height - currentSeg->height) / segmentSize;
    float pitchAngle = atanf(road_climb) * 180.0f / M_PI;
    static float bgPitchValue = 0.0f;
    if (fabsf(bgPitchValue - pitchAngle) < 0.5f) {
        bgPitchValue = pitchAngle;
    } else if (bgPitchValue < pitchAngle)
        bgPitchValue += 0.5f;
    else if (bgPitchValue > pitchAngle)
        bgPitchValue -= 0.5f;
    if (bgPitchValue < -60.0f)
        bgPitchValue = -60.0f;
    if (bgPitchValue > 0.0f)
        bgPitchValue = 0.0f;
    glRotatef(bgPitchValue, 1.0f, 0.0f, 0.0f);

    // Dessiner le ciel (dégradé bleu)
    glBegin(GL_QUADS);
    // Haut du ciel (bleu clair)
    glColor3f(0.4f, 0.6f, 1.0f);
    glVertex3f(-5000.0f, 1000.0f, -1500.0f);
    glVertex3f(5000.0f, 1000.0f, -1500.0f);

    // Horizon (bleu plus pâle)
    glColor3f(0.7f, 0.85f, 1.0f);
    glVertex3f(5000.0f, -200.0f, -1500.0f);
    glVertex3f(-5000.0f, -200.0f, -1500.0f);
    glEnd();

    // Décalage parallaxe selon la courbe de la route
    float bgShift = curveOffset * 2.0f;

    // Couche 1 : Montagnes très lointaines (violet/bleu sombre)
    glColor3f(0.3f, 0.3f, 0.5f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = -15; i <= 15; i++) {
        float x = i * 300.0f + bgShift * 0.2f;
        float height = 200.0f + 80.0f * sinf(i * 0.4f);
        glVertex3f(x, height, -1400.0f);
        glVertex3f(x, -200.0f, -1400.0f);
    }
    glEnd();

    // Couche 2 : Montagnes moyennes (gris-bleu)
    glColor3f(0.4f, 0.5f, 0.6f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = -15; i <= 15; i++) {
        float x = i * 250.0f + bgShift * 0.4f;
        float height = 150.0f + 60.0f * sinf(i * 0.6f + 1.5f);
        glVertex3f(x, height, -1200.0f);
        glVertex3f(x, -200.0f, -1200.0f);
    }
    glEnd();

    // Couche 3 : Collines proches (vert)
    glColor3f(0.3f, 0.6f, 0.4f);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = -15; i <= 15; i++) {
        float x = i * 200.0f + bgShift * 0.8f;
        float height = 100.0f + 40.0f * sinf(i * 0.8f + 2.5f);
        glVertex3f(x, height, -1000.0f);
        glVertex3f(x, -2000.0f, -1000.0f);
    }
    glEnd();

    // Nuages (blancs)
    glColor3f(0.95f, 0.95f, 1.0f);
    for (int i = 0; i < 8; i++) {
        float cloudX = -1500.0f + i * 400.0f + bgShift * 0.15f;
        float cloudY = 300.0f + (i % 3) * 80.0f;
        float cloudZ = -1300.0f;

        // Plusieurs sphères pour former un nuage
        glPushMatrix();
        glTranslatef(cloudX, cloudY, cloudZ);
        glScalef(80.0f, 30.0f, 30.0f);
        glutSolidSphere(1.0f, 8, 6);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(cloudX + 50.0f, cloudY + 10.0f, cloudZ);
        glScalef(70.0f, 25.0f, 25.0f);
        glutSolidSphere(1.0f, 8, 6);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(cloudX + 90.0f, cloudY, cloudZ);
        glScalef(60.0f, 20.0f, 20.0f);
        glutSolidSphere(1.0f, 8, 6);
        glPopMatrix();
    }

    // Restaurer les états
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void drawText(float x, float y, const char *text) {
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 800, 0, 600, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);

    for (const char *c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
    }

    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING);
}
static Road *generateRoad(RoadParam params) {
    Road *road = (Road *)malloc(sizeof(Road));
    if (!road)
        return NULL;

    road->params = params;
    int totalSegments = params.length * params.zoneSize;
    road->segments = (RoadSegment *)malloc(sizeof(RoadSegment) * totalSegments);
    if (!road->segments) {
        free(road);
        return NULL;
    }
    road->count = totalSegments;

    int currentStateH = 0; // 0=flat 1=up 2=down
    int transitionH[3][3] = {{0, 1, 2}, {0, 2, 2}, {0, 1, 1}};

    int currentStateC = 0; // 0=straight 1=left 2=right
    int transitionC[3][3] = {{0, 1, 2}, {0, 2, 2}, {0, 1, 1}};

    float currentHeight = 0;
    float currentCurve = 0;

    int segmentIndex = 0;

    for (int zones = params.length; zones > 0; zones--) {
        // Generate current Zone
        float finalHeight;
        switch (currentStateH) {
        case 0:
            finalHeight = 0;
            break;
        case 1:
            finalHeight = params.maxHeight * randf();
            break;
        case 2:
            finalHeight = -params.maxHeight * randf();
            break;
        }

        float finalCurve;
        switch (currentStateC) {
        case 0:
            finalCurve = 0;
            break;
        case 1:
            finalCurve = -params.maxCurve * randf();
            break;
        case 2:
            finalCurve = params.maxCurve * randf();
            break;
        }

        for (int i = 0; i < params.zoneSize; i++) {
            // Add a sprite
            Sprite *sprite = NULL;
            if (i % (params.zoneSize / 4) == 0) {
                sprite = (Sprite *)malloc(sizeof(Sprite));
                sprite->type = 0; // rock
                sprite->pos = -0.55f;
                sprite->size = 0.5f + randf() * 1.5f;
            } else if (randf() < 0.05f) {
                sprite = (Sprite *)malloc(sizeof(Sprite));
                sprite->type = 1; // tree
                sprite->pos = 0.6f + 4.0f * randf();
                if (randf() < 0.5f) {
                    sprite->pos = -sprite->pos;
                }
                sprite->size = 0.5f + randf() * 1.5f;
            } else if (randf() < 0.05f) {
                sprite = (Sprite *)malloc(sizeof(Sprite));
                sprite->type = 2; // pyramid
                sprite->pos = 0.6f + 4.0f * randf();
                if (randf() < 0.5f) {
                    sprite->pos = -sprite->pos;
                }
                sprite->size = 0.5f + randf() * 1.5f;
            }

            float t = (float)i / (float)params.zoneSize;
            float sineFactor = 0.5f * (1.0f + sinf(t * M_PI - M_PI / 2.0f));

            road->segments[segmentIndex].height = currentHeight + finalHeight * sineFactor;
            road->segments[segmentIndex].curve = currentCurve + finalCurve * sineFactor;
            road->segments[segmentIndex].sprite = sprite;

            segmentIndex++;
        }

        currentHeight += finalHeight;
        currentCurve += finalCurve;

        // Find next zone
        if (randf() < params.mountainy) {
            currentStateH = transitionH[currentStateH][1 + (int)(randf() + 0.5f)];
        } else {
            currentStateH = transitionH[currentStateH][0];
        }

        if (randf() < params.curvy) {
            currentStateC = transitionC[currentStateC][1 + (int)(randf() + 0.5f)];
        } else {
            currentStateC = transitionC[currentStateC][0];
        }
    }

    float heightDiff = road->segments[road->count - 1].height - road->segments[0].height;
    float curveDiff = road->segments[road->count - 1].curve - road->segments[0].curve;

    for (int i = 0; i < road->count; i++) {
        float ratio = (float)i / (float)road->count;
        road->segments[i].height -= heightDiff * ratio;
        road->segments[i].curve -= curveDiff * ratio;
    }

    return road;
}
static void freeRoad(Road *road) {
    if (road) {
        if (road->segments) {
            for (int i = 0; i < road->count; i++) {
                if (road->segments[i].sprite) {
                    free(road->segments[i].sprite);
                }
            }
            free(road->segments);
        }
        free(road);
    }
}

static void drawRoadSegment(int index, float zPos) {
    if (index < 0 || index >= road->count)
        return;

    RoadSegment *seg = &road->segments[index];
    RoadSegment *nextSeg = &road->segments[(index + 1) % road->count];

    int alternate = (index / 4) % 2;

    int isFinishLine = (index == road->count - 1);

    float heightDelta = nextSeg->height - seg->height;

    float normal[3];
    normal[0] = 0.0f;
    normal[1] = segmentSize;
    normal[2] = -heightDelta;

    float length = sqrtf(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (length > 0.0f) {
        normal[0] /= length;
        normal[1] /= length;
        normal[2] /= length;
    }

    if (alternate) {
        glColor3f(0.2f, 0.5f, 0.2f); // Vert foncé
    } else {
        glColor3f(0.3f, 0.6f, 0.3f); // Vert clair
    }

    // Herbe gauche
    glBegin(GL_QUADS);
    glNormal3fv(normal);
    glVertex3f(-roadWidth - borderWidth - grassWidth + seg->curve, seg->height, zPos);
    glVertex3f(-roadWidth - borderWidth + seg->curve, seg->height, zPos);
    glVertex3f(-roadWidth - borderWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glVertex3f(-roadWidth - borderWidth - grassWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glEnd();

    // Herbe droite
    glBegin(GL_QUADS);
    glNormal3fv(normal);
    glVertex3f(roadWidth + borderWidth + seg->curve, seg->height, zPos);
    glVertex3f(roadWidth + borderWidth + grassWidth + seg->curve, seg->height, zPos);
    glVertex3f(roadWidth + borderWidth + grassWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glVertex3f(roadWidth + borderWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glEnd();

    // Couleurs alternées pour les bordures (ou blanc pour la ligne d'arrivée)
    if (isFinishLine) {
        glColor3f(1.0f, 1.0f, 1.0f); // Blanc pour la ligne d'arrivée
    } else if (alternate) {
        glColor3f(1.0f, 0.0f, 0.0f); // Rouge
    } else {
        glColor3f(1.0f, 1.0f, 1.0f); // Blanc
    }

    // Bordure gauche
    glBegin(GL_QUADS);
    glNormal3fv(normal);
    glVertex3f(-roadWidth - borderWidth + seg->curve, seg->height, zPos);
    glVertex3f(-roadWidth + seg->curve, seg->height, zPos);
    glVertex3f(-roadWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glVertex3f(-roadWidth - borderWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glEnd();

    // Bordure droite
    glBegin(GL_QUADS);
    glNormal3fv(normal);
    glVertex3f(roadWidth + seg->curve, seg->height, zPos);
    glVertex3f(roadWidth + borderWidth + seg->curve, seg->height, zPos);
    glVertex3f(roadWidth + borderWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glVertex3f(roadWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glEnd();

    // Route (blanc pour ligne d'arrivée, sinon gris alterné)
    if (isFinishLine) {
        glColor3f(1.0f, 1.0f, 1.0f); // Blanc pour la ligne d'arrivée
    } else if (alternate) {
        glColor3f(0.6f, 0.6f, 0.6f);
    } else {
        glColor3f(0.47f, 0.47f, 0.47f);
    }

    // Draw road quad avec normale
    glBegin(GL_QUADS);
    glNormal3fv(normal);
    glVertex3f(-roadWidth + seg->curve, seg->height, zPos);
    glVertex3f(roadWidth + seg->curve, seg->height, zPos);
    glVertex3f(roadWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glVertex3f(-roadWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glEnd();

    // Dessiner les sprites (rochers et arbres)
    if (seg->sprite != NULL) {
        glPushMatrix();

        float spriteX = seg->curve + seg->sprite->pos * (2 * roadWidth);
        float spriteY = seg->height;
        float spriteZ = zPos + segmentSize / 2.0f; // Milieu du segment

        // Calculer l'angle de la route (différence de courbe entre segment actuel et suivant)
        float curveDelta = nextSeg->curve - seg->curve;

        // Calculer l'angle en degrés pour orienter le sprite perpendiculairement à la route
        float roadAngle = atan2f(curveDelta, segmentSize) * 180.0f / M_PI;

        // Calculer l'angle de pente (montée/descente)
        float heightDelta = nextSeg->height - seg->height;
        float slopeAngle = atan2f(heightDelta, segmentSize) * 180.0f / M_PI;

        // Positionner le sprite
        glTranslatef(spriteX, spriteY, spriteZ);

        // Orienter le sprite perpendiculairement à la route
        glRotatef(-roadAngle, 0.0f, 1.0f, 0.0f);
        glRotatef(slopeAngle, 1.0f, 0.0f, 0.0f);

        switch (seg->sprite->type) {
        case 0:
            glColor3f(0.4f, 0.4f, 0.4f);
            glScalef(seg->sprite->size, seg->sprite->size, seg->sprite->size);
            glTranslatef(0.0f, 5.0f, 0.0f);
            glutSolidCube(10.0f);
            break;
        case 1:
            // Arbre (cube vert)
            glColor3f(0.4f, 0.3f, 0.2f);
            glScalef(seg->sprite->size, seg->sprite->size, seg->sprite->size);
            glTranslatef(0.0f, 5.0f, 0.0f);
            glutSolidCube(10.0f);
            glColor3f(0.1f, 0.6f, 0.1f);
            glTranslatef(0.0f, 20.0f, 0.0f);
            glutSolidCube(30.0f);
            break;
        case 2:
            glColor3f(0.8f, 0.7f, 0.1f);
            glScalef(seg->sprite->size * 4.0f, seg->sprite->size * 2.0f, seg->sprite->size * 4.0f);
            drawPyramid();
            break;
        }

        glPopMatrix();
    }
}
static void drawPlayerCar(void) {
    float carDistanceAhead = 80.0f;
    float carPositionOnRoad = cameraPosition + carDistanceAhead;

    int carSegment = (int)(carPositionOnRoad / segmentSize);
    float carSegmentProgress = (carPositionOnRoad - carSegment * segmentSize) / segmentSize;

    RoadSegment *carCurrentSeg = &road->segments[carSegment % road->count];
    RoadSegment *carNextSeg = &road->segments[(carSegment + 1) % road->count];

    float carCurve = carCurrentSeg->curve + (carNextSeg->curve - carCurrentSeg->curve) * carSegmentProgress;
    float carHeight = carCurrentSeg->height + (carNextSeg->height - carCurrentSeg->height) * carSegmentProgress;

    float curveDelta = carNextSeg->curve - carCurrentSeg->curve;
    float heightDelta = carNextSeg->height - carCurrentSeg->height;

    float slopeAngle = atan2f(heightDelta, segmentSize) * 180.0f / M_PI;

    glPushMatrix();

    float carAngle = turning > 0.0f ? 30.0f : -30.0f;
    static float currentCarAngle = 0.0f;

    if (turning == 0)
        carAngle = 0.0f;

    if (currentCarAngle < carAngle) {
        currentCarAngle += 5.0f;
        if (currentCarAngle > carAngle)
            currentCarAngle = carAngle;
    } else if (currentCarAngle > carAngle) {
        currentCarAngle -= 5.0f;
        if (currentCarAngle < carAngle)
            currentCarAngle = carAngle;
    }

    glTranslatef(playerX, carHeight + 5.0f, carDistanceAhead);

    // Orienter la voiture
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(currentCarAngle, 0.0f, 1.0f, 0.0f);
    glRotatef(slopeAngle, 1.0f, 0.0f, 0.0f);

    // Dessiner la carrosserie (cube rouge)
    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 8.0f, 0.0f);
    glutSolidCube(15.0f);
    glPopMatrix();

    // Dimensions pour positionner les roues
    float carHalfSize = 15.0f / 2.0f; // 7.5f
    float wheelSize = 3.0f;
    float wheelHalfSize = wheelSize / 2.0f; // 1.5f
    glColor3f(0.0f, 0.0f, 0.0f);

    // Les roues sont positionnées SOUS et AUTOUR de la carrosserie
    float wheelOffsetX = carHalfSize;                  // Au bord gauche/droite de la carrosserie
    float wheelOffsetY = -carHalfSize - wheelHalfSize; // SOUS la carrosserie
    float wheelOffsetZ = carHalfSize - 1.0f;           // Légèrement vers l'intérieur

    // Couleur des roues (noir)
    glColor3f(0.0f, 0.0f, 0.0f);

    glPushMatrix();
    glTranslatef(-10.0f, 0.0f, 10.0f);
    glutSolidCube(wheelSize);
    glPopMatrix();

    // Roue avant-droite
    glPushMatrix();
    glTranslatef(10.0f, 0.0f, 10.0f);
    glutSolidCube(wheelSize);
    glPopMatrix();

    // Roue arrière-gauche
    glPushMatrix();
    glTranslatef(-10.0f, 0.0f, -10.0f);
    glutSolidCube(wheelSize);
    glPopMatrix();

    // Roue arrière-droite
    glPushMatrix();
    glTranslatef(10.0f, 0.0f, -10.0f);
    glutSolidCube(wheelSize);
    glPopMatrix();

    glPopMatrix();
}

static void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    keyStates[key] = 1; // Marquer la touche comme pressée

    if (key == 27) { // ESC
        if (current_game_state == GAME_STATE_PLAYING) {
            current_game_state = GAME_STATE_TITLE;
            idlefunc = displayIntro;
        } else if (current_game_state == GAME_STATE_TITLE) {
            freeRoad(road);
            exit(0);
        }
    }
    if (key == 13) {
        if (current_game_state == GAME_STATE_TITLE) {
            current_game_state = GAME_STATE_PLAYING;
            idlefunc = displayManual;
            cameraPosition = 0.0f;
            playerX = 0.0f;
            playerSpeed = 0.0f;
        }
    }
}

static void keyboardUp(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    keyStates[key] = 0; // Marquer la touche comme relâchée
}

static void specialKeys(int key, int x, int y) {
    (void)x;
    (void)y;
    specialKeyStates[key] = 1; // Marquer la touche spéciale comme pressée
}

static void specialKeysUp(int key, int x, int y) {
    (void)x;
    (void)y;
    specialKeyStates[key] = 0; // Marquer la touche spéciale comme relâchée
    turning = 0;
}

static void displayManual(void) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat[]){0.4f, 0.7f, 0.3f, 0.0f});
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    int currentSegment = (int)(cameraPosition / segmentSize);
    float segmentProgress = (cameraPosition - currentSegment * segmentSize) / segmentSize;

    RoadSegment *currentSeg = &road->segments[currentSegment % road->count];
    RoadSegment *nextSeg = &road->segments[(currentSegment + 1) % road->count];

    float baseOffset = currentSeg->curve + (nextSeg->curve - currentSeg->curve) * segmentProgress;
    float playerHeight = currentSeg->height + (nextSeg->height - currentSeg->height) * segmentProgress;
    float road_climb = (nextSeg->height - currentSeg->height) / segmentSize;

    float camX = playerX;
    float camY = playerHeight + cameraHeight;
    float camZ = 0.0f;

    drawBackground(baseOffset);

    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

    float pitchAngle = atanf(road_climb) * 180.0f / M_PI;
    static float pitchValue = 0.0f;
    if (fabsf(pitchValue - pitchAngle) < 0.5f) {
        pitchValue = pitchAngle;
    } else if (pitchValue < pitchAngle)
        pitchValue += 0.5f;
    else if (pitchValue > pitchAngle)
        pitchValue -= 0.5f;
    if (pitchValue < -60.0f)
        pitchValue = -60.0f;
    if (pitchValue > 0.0f)
        pitchValue = 0.0f;
    glRotatef(pitchValue, 1.0f, 0.0f, 0.0f);

    glPushMatrix();
    glTranslatef(-camX, -camY, -camZ);

    // Draw road segments
    for (int i = 0; i < 150; i++) {
        int segIndex = (currentSegment + i) % road->count;
        float z = (i * segmentSize) - (cameraPosition - currentSegment * segmentSize);
        drawRoadSegment(segIndex, z);
    }

    for (int i = 0; i < MAX_TRAFFIC; i++) {
        drawTrafficCar(&traffic[i]);
    }
    // glPopMatrix();
    drawPlayerCar();
    glPopMatrix();

    // Afficher les informations de debug
    char buffer[256];
    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buffer, "Speed: %.2f km", (playerSpeed / maxRoadSpeed) * 200.0f);
    drawText(10, 580, buffer);

    float currentMinutes = currentLapTime / 60.0f;
    float currentSeconds = fmodf(currentLapTime, 60.0f);
    sprintf(buffer, "Current Lap: %02.0f:%05.2f", floorf(currentMinutes), currentSeconds);
    drawText(10, 540, buffer);

    currentMinutes = lapTime / 60.0f;
    currentSeconds = fmodf(lapTime, 60.0f);
    sprintf(buffer, "Lap Time: %02.0f:%05.2f", floorf(currentMinutes), currentSeconds);
    drawText(10, 560, buffer);

    currentMinutes = bestLapTime / 60.0f;
    currentSeconds = fmodf(bestLapTime, 60.0f);
    sprintf(buffer, "Best Lap: %02.0f:%05.2f", floorf(currentMinutes), currentSeconds);
    drawText(10, 520, buffer);
    glutSwapBuffers();
}

static void displayIntro(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    drawBackground(0.0f);

    // Afficher le texte du menu

    int currentSegment = (int)(cameraPosition / segmentSize);
    float segmentProgress = (cameraPosition - currentSegment * segmentSize) / segmentSize;
    RoadSegment *currentSeg = &road->segments[currentSegment % road->count];
    RoadSegment *nextSeg = &road->segments[(currentSegment + 1) % road->count];
    float playerHeight = currentSeg->height + (nextSeg->height - currentSeg->height) * segmentProgress;
    float baseOffset = currentSeg->curve + (nextSeg->curve - currentSeg->curve) * segmentProgress;
    // Draw road segments
    glPushMatrix();
    glLoadIdentity();
    glLightfv(GL_LIGHT0, GL_POSITION, (GLfloat[]){0.4f, 0.7f, 0.3f, 0.0f});
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    float camY = 10.0f + playerHeight;

    glTranslatef(baseOffset, -camY, -10.0f);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    cameraPosition += 2.0f; // Avancer la caméra pour l'animation du menu
    for (int i = 0; i < 150; i++) {
        int segIndex = (currentSegment + i) % road->count;
        float z = (i * segmentSize) - (cameraPosition - currentSegment * segmentSize);
        drawRoadSegment(segIndex, z);
    }
    glPopMatrix();
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(300, 400, "3D Road Racing Demo");
    drawText(280, 350, "Press ENTER to Start");
    drawText(250, 300, "Use Arrow Keys to Drive the Car");

    glutSwapBuffers();
}
static int checkCollision(float pZ, float pX, float tZ, float tX) {
    if (fabsf(pZ - tZ) < CAR_LENGTH) {
        if (fabsf(pX - tX) < CAR_WIDTH) {
            return 1; // Collision !
        }
    }
    return 0;
}
static void game_loop(double delta_time) {
    if (specialKeyStates[GLUT_KEY_UP]) {
        // Accélération
        if (playerSpeed < maxSpeed) {
            playerSpeed += acceleration;
        } else if (playerSpeed > maxSpeed) {
            playerSpeed -= braking; // Freinage si on dépasse la vitesse max
        }
    }

    if (specialKeyStates[GLUT_KEY_DOWN]) {
        // Freinage
        if (playerSpeed > 0.0f) {
            playerSpeed -= braking;
            if (playerSpeed < 0.0f)
                playerSpeed = 0.0f;
        }
    }
    float steeringSensitivity = 1.0f - (playerSpeed / maxRoadSpeed) * 0.2f;
    if (specialKeyStates[GLUT_KEY_LEFT]) {
        // Tourner à gauche
        if (playerSpeed > 0.0f) { // Seulement si la voiture avance
            playerX += turnSpeed * steeringSensitivity;
        }
        turning = 1.0f;
    }

    if (specialKeyStates[GLUT_KEY_RIGHT]) {
        // Tourner à droite
        if (playerSpeed > 0.0f) { // Seulement si la voiture avance
            playerX -= turnSpeed * steeringSensitivity;
        }
        turning = -1.0f;
    }

    cameraPosition += playerSpeed;
    float carPositionOnRoad = cameraPosition + 80.0f;
    int currentSegmentIndex = (int)(carPositionOnRoad / segmentSize);
    float segmentProgress = (carPositionOnRoad - currentSegmentIndex * segmentSize) / segmentSize;

    RoadSegment *currentSeg = &road->segments[currentSegmentIndex % road->count];
    RoadSegment *nextSeg = &road->segments[(currentSegmentIndex + 1) % road->count];

    roadX = currentSeg->curve + (nextSeg->curve - currentSeg->curve) * segmentProgress;

    float curveDelta = nextSeg->curve - currentSeg->curve;
    centrifugal = curveDelta * (playerSpeed / maxRoadSpeed);
    // playerX -= centrifugal * centrifugalForce;

    if (playerX < roadX - roadWidth) {
        offRoad = 1;
        maxSpeed = maxOffRoadSpeed;
    } else if (playerX > roadX + roadWidth) {
        offRoad = 1;
        maxSpeed = maxOffRoadSpeed;
    } else {
        offRoad = 0;
        maxSpeed = maxRoadSpeed;
    }

    if (playerSpeed > 0.0f) {
        playerSpeed -= deceleration * 0.1f;
        if (playerSpeed < 0.0f)
            playerSpeed = 0.0f;
    }

    // Boucler la route
    float maxPosition = road->count * segmentSize;
    if (cameraPosition >= maxPosition) {
        cameraPosition = fmodf(cameraPosition, maxPosition);
        lapTime = currentLapTime;
        if (lapTime < bestLapTime || bestLapTime == 0.0f) {
            bestLapTime = lapTime;
        }
        currentLapTime = 0.0f;
    }
    currentLapTime += (float)delta_time;
    if (carPositionOnRoad >= maxPosition)
        carPositionOnRoad -= maxPosition;

    for (int i = 0; i < MAX_TRAFFIC; i++) {
        traffic[i].z += traffic[i].speed;
        if (traffic[i].z >= maxPosition) {
            traffic[i].z -= maxPosition;
        }
        float distZ = fabsf(carPositionOnRoad - traffic[i].z);

        if (distZ > maxPosition - CAR_LENGTH) {
            distZ = maxPosition - distZ;
        }

        if (distZ < CAR_LENGTH) {
            if (fabsf((playerX - roadX) - traffic[i].x) < CAR_WIDTH) {
                playerSpeed = 0.0f;
                if (playerX > traffic[i].x)
                    playerX += 5.0f;
                else
                    playerX -= 5.0f;
            }
        }
    }
    int checkRange = 4;
    for (int i = -checkRange; i <= checkRange; i++) {
        int segIdx = (currentSegmentIndex + i);
        while (segIdx < 0)
            segIdx += road->count;
        while (segIdx >= road->count)
            segIdx -= road->count;

        RoadSegment *seg = &road->segments[segIdx];

        if (seg->sprite) {
            float spriteZ = segIdx * segmentSize + segmentSize * 0.5f;
            float distZ = fabsf(carPositionOnRoad - spriteZ);
            if (distZ > maxPosition * 0.5f) {
                distZ = maxPosition - distZ;
            }

            if (distZ < (CAR_LENGTH * 0.5f + 5.0f)) {
                float spriteWorldX = seg->curve + seg->sprite->pos * (2.0f * roadWidth);

                // Distance X
                if (fabsf(playerX - spriteWorldX) < (CAR_WIDTH * 0.5f + 5.0f)) {
                    // COLLISION !
                    playerSpeed = 0.0f; // Arrêt brutal

                    if (playerX > spriteWorldX)
                        playerX += 5.0f;
                    else
                        playerX -= 5.0f;
                }
            }
        }
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
static void idle(void) {
    clock_t current_time = clock();
    double elapsed_ms = (double)(current_time - last_frame_time) * 1000.0 / CLOCKS_PER_SEC;

    if (elapsed_ms >= FRAME_TIME_MS) {
        last_frame_time = current_time;
        if (current_game_state == GAME_STATE_PLAYING) {
            game_loop(elapsed_ms / 1000.0);
        }

        glutPostRedisplay();
    }
}

static void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 1.0, 1000000.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
    srand((unsigned int)time(NULL));

    // Generate road
    RoadParam params = {
        .maxHeight = 200.0f, .maxCurve = 600.0f, .length = 12, .curvy = 0.8f, .mountainy = 0.3f, .zoneSize = 250};

    road = generateRoad(params);
    if (!road) {
        printf("Failed to generate road\n");
        return 1;
    }
    initTraffic();
    // Initialize OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Road Racer - OpenGL");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.4f, 0.6f, 1.0f, 1.0f);
    idlefunc = displayIntro;
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);

    glutMainLoop();

    freeRoad(road);
    return 0;
}