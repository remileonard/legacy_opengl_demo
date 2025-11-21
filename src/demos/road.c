
#ifdef _WIN32
    #include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glut.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    int type;  // 0=rock, 1=tree
    float pos;
} Sprite;

typedef struct {
    float height;
    float curve;
    Sprite* sprite;  // NULL if no sprite
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
    RoadSegment* segments;
    int count;
    RoadParam params;
} Road;


Road* road = NULL;
float cameraPosition = 0.0f;
float cameraHeight = 40.0f;
int segmentSize = 5;

float currentCamX = 0.0f;
float currentCamY = 0.0f;

float playerX = 0.0f;           // Position latérale de la voiture (-roadWidth à +roadWidth)
float playerSpeed = 0.0f;       // Vitesse actuelle
float maxSpeed = 5.0f;          // Vitesse maximale
float acceleration = 0.15f;     // Accélération
float deceleration = 0.1f;      // Décélération
float braking = 0.3f;           // Freinage
float offRoadDecel = 0.5f;      // Décélération hors route
float turnSpeed = 0.5f;         // Vitesse de virage
float centrifugalForce = 0.8f;  // Force centrifuge
float backgroundOffset = 0.0f;  // Décalage horizontal du décor

int keyStates[256] = {0};        // État des touches normales
int specialKeyStates[256] = {0}; // État des touches spéciales (flèches)


Road* generateRoad(RoadParam params);
void freeRoad(Road* road);



static float randf() {
    return (float)rand() / (float)RAND_MAX;
}
void drawBackground(float curveOffset) {
    // Sauvegarder l'état de l'éclairage et du depth test
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    // Sauvegarder la matrice
    glPushMatrix();
    glLoadIdentity();
    
    // Appliquer seulement les rotations de la caméra (pas la translation)
    //glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    
    // Calculer le pitch pour que le fond suive la caméra
    int currentSegment = (int)(cameraPosition / segmentSize);
    float segmentProgress = (cameraPosition - currentSegment * segmentSize) / segmentSize;
    RoadSegment* currentSeg = &road->segments[currentSegment % road->count];
    RoadSegment* nextSeg = &road->segments[(currentSegment + 1) % road->count];
    float road_climb = (nextSeg->height - currentSeg->height) / segmentSize;
    float pitchAngle = atanf(road_climb) * 180.0f / M_PI;
    static float bgPitchValue = 0.0f;
    if (fabsf(bgPitchValue - pitchAngle) < 0.5f) {
        bgPitchValue = pitchAngle;
    } else if (bgPitchValue < pitchAngle) bgPitchValue += 0.5f;
    else if (bgPitchValue > pitchAngle) bgPitchValue -= 0.5f;
    if (bgPitchValue < -60.0f) bgPitchValue = -60.0f;
    if (bgPitchValue > 0.0f) bgPitchValue = 0.0f;
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
    float bgShift = curveOffset * 0.5f;
    
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

void drawText(float x, float y, const char* text) {
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
    
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
    }
    
    glEnable(GL_DEPTH_TEST);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}
Road* generateRoad(RoadParam params) {
    Road* road = (Road*)malloc(sizeof(Road));
    if (!road) return NULL;
    
    road->params = params;
    int totalSegments = params.length * params.zoneSize;
    road->segments = (RoadSegment*)malloc(sizeof(RoadSegment) * totalSegments);
    if (!road->segments) {
        free(road);
        return NULL;
    }
    road->count = totalSegments;
    
    int currentStateH = 0;  // 0=flat 1=up 2=down
    int transitionH[3][3] = {{0,1,2},{0,2,2},{0,1,1}};
    
    int currentStateC = 0;  // 0=straight 1=left 2=right
    int transitionC[3][3] = {{0,1,2},{0,2,2},{0,1,1}};
    
    float currentHeight = 0;
    float currentCurve = 0;
    
    int segmentIndex = 0;
    
    for (int zones = params.length; zones > 0; zones--) {
        // Generate current Zone
        float finalHeight;
        switch(currentStateH) {
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
        switch(currentStateC) {
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
            Sprite* sprite = NULL;
            if (i % (params.zoneSize / 4) == 0) {
                sprite = (Sprite*)malloc(sizeof(Sprite));
                sprite->type = 0;  // rock
                sprite->pos = -0.55f;
            } else if (randf() < 0.05f) {
                sprite = (Sprite*)malloc(sizeof(Sprite));
                sprite->type = 1;  // tree
                sprite->pos = 0.6f + 4.0f * randf();
                if (randf() < 0.5f) {
                    sprite->pos = -sprite->pos;
                }
            }
            
            float t = (float)i / (float)params.zoneSize;
            // Interpolation sinusoïdale : commence à 0, monte à 1, redescend à 0
            // Cela garantit que chaque zone commence et termine au même niveau
            float sineFactor = 0.5f * (1.0f + sinf(t * M_PI - M_PI / 2.0f));
            
            // La hauteur et la courbe sont interpolées de manière à revenir au point de départ
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
void freeRoad(Road* road) {
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

void drawRoadSegment(int index, float zPos) {
    if (index < 0 || index >= road->count) return;
    
    RoadSegment* seg = &road->segments[index];
    RoadSegment* nextSeg = &road->segments[(index + 1) % road->count];
    
    float roadWidth = 50.0f;
    float borderWidth = 3.0f;
    float grassWidth = 2000.0f;
    int alternate = (index / 4) % 2;
    
    // Vérifier si c'est le dernier segment (ligne d'arrivée)
    int isFinishLine = (index == road->count - 1);
    
    // Calculer la normale : uniquement basée sur la pente (changement de hauteur)
    float heightDelta = nextSeg->height - seg->height;
    
    // Vecteur le long de la route (dans la direction Z)
    // Direction: (0, heightDelta, segmentSize)
    // La normale est perpendiculaire à ce vecteur dans le plan vertical
    
    // Pour un quad horizontal pentu, la normale pointe "vers le haut et légèrement en arrière"
    float normal[3];
    normal[0] = 0.0f;                    // Pas de composante X (la route ne penche pas latéralement)
    normal[1] = segmentSize;             // Composante Y (pointe vers le haut)
    normal[2] = -heightDelta;            // Composante Z (opposée à la pente)
    
    // Normaliser le vecteur
    float length = sqrtf(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
    if (length > 0.0f) {
        normal[0] /= length;
        normal[1] /= length;
        normal[2] /= length;
    }
    
    // Couleurs alternées pour l'herbe
    if (alternate) {
        glColor3f(0.2f, 0.5f, 0.2f);  // Vert foncé
    } else {
        glColor3f(0.3f, 0.6f, 0.3f);  // Vert clair
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
        glColor3f(1.0f, 1.0f, 1.0f);  // Blanc pour la ligne d'arrivée
    } else if (alternate) {
        glColor3f(1.0f, 0.0f, 0.0f);  // Rouge
    } else {
        glColor3f(1.0f, 1.0f, 1.0f);  // Blanc
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
        glColor3f(1.0f, 1.0f, 1.0f);  // Blanc pour la ligne d'arrivée
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
        
        // Position du sprite
        float spriteX = seg->curve + seg->sprite->pos * (2 * roadWidth);
        float spriteY = seg->height;
        float spriteZ = zPos + segmentSize / 2.0f;  // Milieu du segment
        
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
        
        if (seg->sprite->type == 0) {
            // Rocher (cube marron)
            glColor3f(0.4f, 0.3f, 0.2f);
            glTranslatef(0.0f, 5.0f, 0.0f);
            glutSolidCube(10.0f);
        } else {
            // Arbre (cube vert)
            glColor3f(0.1f, 0.6f, 0.1f);
            glTranslatef(0.0f, 15.0f, 0.0f);
            glutSolidCube(30.0f);
        }
        
        glPopMatrix();
    }
}
void drawPlayerCar(void) {
    // La voiture est à 80 unités devant la caméra
    float carDistanceAhead = 80.0f;
    
    // Calculer la position de la voiture (pas celle de la caméra !)
    float carPositionOnRoad = cameraPosition + carDistanceAhead;
    
    int carSegment = (int)(carPositionOnRoad / segmentSize);
    float carSegmentProgress = (carPositionOnRoad - carSegment * segmentSize) / segmentSize;
    
    RoadSegment* carCurrentSeg = &road->segments[carSegment % road->count];
    RoadSegment* carNextSeg = &road->segments[(carSegment + 1) % road->count];
    
    // Position interpolée de la voiture sur SA PROPRE position
    float carCurve = carCurrentSeg->curve + (carNextSeg->curve - carCurrentSeg->curve) * carSegmentProgress;
    float carHeight = carCurrentSeg->height + (carNextSeg->height - carCurrentSeg->height) * carSegmentProgress;
    
    // Calculer l'angle de la route pour orienter la voiture
    float curveDelta = carNextSeg->curve - carCurrentSeg->curve;
    float heightDelta = carNextSeg->height - carCurrentSeg->height;
    
    float roadAngle = atan2f(curveDelta, segmentSize) * 180.0f / M_PI;
    float slopeAngle = atan2f(heightDelta, segmentSize) * 180.0f / M_PI;
    
    glPushMatrix();
    
    // Position de la voiture : courbe de la route + déplacement latéral du joueur
    glTranslatef(carCurve + playerX, carHeight + 5.0f, carDistanceAhead);
    
    // Orienter la voiture
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(roadAngle, 0.0f, 1.0f, 0.0f);
    glRotatef(slopeAngle, 1.0f, 0.0f, 0.0f);
    
    // Dessiner la carrosserie (cube rouge)
    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 8.0f, 0.0f);
    glutSolidCube(15.0f);
    glPopMatrix();
    
    // Dimensions pour positionner les roues
    float carHalfSize = 15.0f / 2.0f;  // 7.5f
    float wheelSize = 3.0f;
    float wheelHalfSize = wheelSize / 2.0f;  // 1.5f
    glColor3f(0.0f, 0.0f, 0.0f);

    // Les roues sont positionnées SOUS et AUTOUR de la carrosserie
    float wheelOffsetX = carHalfSize;              // Au bord gauche/droite de la carrosserie
    float wheelOffsetY = -carHalfSize - wheelHalfSize;  // SOUS la carrosserie
    float wheelOffsetZ = carHalfSize - 1.0f;       // Légèrement vers l'intérieur
    
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
// Remplacer les fonctions keyboard et specialKeys par celles-ci :
void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;
    keyStates[key] = 1;  // Marquer la touche comme pressée
    
    if (key == 27) {  // ESC
        freeRoad(road);
        exit(0);
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
    specialKeyStates[key] = 1;  // Marquer la touche spéciale comme pressée
}

void specialKeysUp(int key, int x, int y) {
    (void)x;
    (void)y;
    specialKeyStates[key] = 0;  // Marquer la touche spéciale comme relâchée
}

void displayManual(void) {

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

    RoadSegment* currentSeg = &road->segments[currentSegment % road->count];
    RoadSegment* nextSeg = &road->segments[(currentSegment + 1) % road->count];
    
    float baseOffset = currentSeg->curve + (nextSeg->curve - currentSeg->curve) * segmentProgress;
    float playerHeight = currentSeg->height + (nextSeg->height - currentSeg->height) * segmentProgress;
    float road_climb = (nextSeg->height - currentSeg->height) / segmentSize;

    float camX = baseOffset;
    float camY = playerHeight + cameraHeight;
    float camZ = 0.0f;

    drawBackground(baseOffset);

    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

    float pitchAngle = atanf(road_climb) * 180.0f / M_PI;
    static float pitchValue = 0.0f;
    if (fabsf(pitchValue - pitchAngle) < 0.5f) {
        pitchValue = pitchAngle;
    } else if (pitchValue < pitchAngle ) pitchValue += 0.5f;
    else if (pitchValue > pitchAngle) pitchValue -= 0.5f;
    if (pitchValue < -60.0f) pitchValue = -60.0f;
    if (pitchValue > 0.0f) pitchValue = 0.0f;
    glRotatef(pitchValue, 1.0f, 0.0f, 0.0f);
    
    glPushMatrix();
    glTranslatef(-camX, -camY, -camZ);
    
    // Draw road segments
    for (int i = 0; i < 150; i++) {
        int segIndex = (currentSegment + i) % road->count;
        float z = (i * segmentSize) - (cameraPosition - currentSegment * segmentSize);
        drawRoadSegment(segIndex, z);
    }
    drawPlayerCar();
    glPopMatrix();
    
    // Afficher les informations de debug
    char buffer[256];
    
    sprintf(buffer, "Speed: %.2f  Player X: %.2f", playerSpeed, playerX);
    drawText(10, 580, buffer);
    
    sprintf(buffer, "Camera X: %.2f  Y: %.2f", camX, camY);
    drawText(10, 560, buffer);
    
    sprintf(buffer, "Segment: %d  Position: %.2f", currentSegment, cameraPosition);
    drawText(10, 540, buffer);
    
    sprintf(buffer, "Camera Pitch: %.2f", pitchValue);
    drawText(10, 520, buffer);

    sprintf(buffer, "Curve: %.2f  Height: %.2f", currentSeg->curve - nextSeg->curve, currentSeg->height);
    drawText(10, 500, buffer);
    float roadWidth = 50.0f;
    if (playerX < -roadWidth || playerX > roadWidth) {
        sprintf(buffer, "OFF ROAD!");
        drawText(350, 580, buffer);
        playerX *= 0.9f;  // Ramener progressivement la voiture vers la route
    }
    
    glutSwapBuffers();
}
void idle(void) {
    float roadWidth = 50.0f;
    // Traiter les touches pressées en continu
    if (specialKeyStates[GLUT_KEY_UP]) {
        // Accélération
        if (playerSpeed < maxSpeed) {
            playerSpeed += acceleration;
        }
    }
    
    if (specialKeyStates[GLUT_KEY_DOWN]) {
        // Freinage
        if (playerSpeed > 0.0f) {
            playerSpeed -= braking;
            if (playerSpeed < 0.0f) playerSpeed = 0.0f;
        }
    }
    
    if (specialKeyStates[GLUT_KEY_LEFT]) {
        // Tourner à gauche
        if (playerSpeed > 0.0f) {  // Seulement si la voiture avance
            playerX += turnSpeed * (playerSpeed / maxSpeed);
        }
    }
    
    if (specialKeyStates[GLUT_KEY_RIGHT]) {
        // Tourner à droite
        if (playerSpeed > 0.0f) {  // Seulement si la voiture avance
            playerX -= turnSpeed * (playerSpeed / maxSpeed);
        }
    }
    // Appliquer la vitesse du joueur
    cameraPosition += playerSpeed;
    
    // Calculer la position de la voiture pour la force centrifuge
    float carPositionOnRoad = cameraPosition + 80.0f;
    int carSegment = (int)(carPositionOnRoad / segmentSize);
    
    RoadSegment* carCurrentSeg = &road->segments[carSegment % road->count];
    RoadSegment* carNextSeg = &road->segments[(carSegment + 1) % road->count];
    
    // Calculer la courbure de la route (force centrifuge)
    float curveDelta = carCurrentSeg->curve - carNextSeg->curve;
    
    // Appliquer la force centrifuge (pousse la voiture vers l'extérieur du virage)
    if (playerSpeed > 0.0f) {
        float centrifugal = (curveDelta / segmentSize) * centrifugalForce * (playerSpeed / maxSpeed) * 2.0f;
        playerX -= centrifugal;
    }
    
    // Vérifier si la voiture est hors de la route
    if (playerX < -roadWidth || playerX > roadWidth) {
        // Ralentir considérablement hors route
        playerSpeed -= offRoadDecel;
        if (playerSpeed < 0.0f) playerSpeed = 0.0f;
        
        // Limiter la position pour ne pas aller trop loin
        if (playerX < -roadWidth - 20.0f) playerX = -roadWidth - 20.0f;
        if (playerX > roadWidth + 20.0f) playerX = roadWidth + 20.0f;
    }
    
    // Décélération naturelle si aucune touche n'est pressée
    if (playerSpeed > 0.0f) {
        playerSpeed -= deceleration * 0.1f;
        if (playerSpeed < 0.0f) playerSpeed = 0.0f;
    }
    
    // Boucler la route
    float maxPosition = road->count * segmentSize;
    if (cameraPosition >= maxPosition) {
        cameraPosition = fmodf(cameraPosition, maxPosition);
    }
    
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 1.0, 1000000.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));
    
    // Generate road
    RoadParam params = {
        .maxHeight = 900.0f,
        .maxCurve = 400.0f,
        .length = 12,
        .curvy = 0.8f,
        .mountainy = 0.8f,
        .zoneSize = 250
    };
    
    road = generateRoad(params);
    if (!road) {
        printf("Failed to generate road\n");
        return 1;
    }
    
    // Initialize OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Road Racer - OpenGL");
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.4f, 0.6f, 1.0f, 1.0f);
    
    glutDisplayFunc(displayManual);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);        // AJOUTER : Détection des touches relâchées
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);      // AJOUTER : Détection des touches spéciales relâchées
    
    glutMainLoop();
    
    freeRoad(road);
    return 0;
}