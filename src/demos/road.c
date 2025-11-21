
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

Road* generateRoad(RoadParam params);
void freeRoad(Road* road);



static float randf() {
    return (float)rand() / (float)RAND_MAX;
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
    
    // Couleurs alternées pour l'herbe
    if (alternate) {
        glColor3f(0.2f, 0.5f, 0.2f);  // Vert foncé
    } else {
        glColor3f(0.3f, 0.6f, 0.3f);  // Vert clair
    }
    
    // Herbe gauche
    glBegin(GL_QUADS);
        glVertex3f(-roadWidth - borderWidth - grassWidth + seg->curve, seg->height, zPos);
        glVertex3f(-roadWidth - borderWidth + seg->curve, seg->height, zPos);
        glVertex3f(-roadWidth - borderWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
        glVertex3f(-roadWidth - borderWidth - grassWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glEnd();
    
    // Herbe droite
    glBegin(GL_QUADS);
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
        glVertex3f(-roadWidth - borderWidth + seg->curve, seg->height, zPos);
        glVertex3f(-roadWidth + seg->curve, seg->height, zPos);
        glVertex3f(-roadWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
        glVertex3f(-roadWidth - borderWidth + nextSeg->curve, nextSeg->height, zPos + segmentSize);
    glEnd();
    
    // Bordure droite
    glBegin(GL_QUADS);
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
    
    // Draw road quad
    glBegin(GL_QUADS);
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
        // atan2 donne l'angle en radians, on convertit en degrés
        float roadAngle = atan2f(curveDelta, segmentSize) * 180.0f / M_PI;
        
        // Calculer l'angle de pente (montée/descente)
        float heightDelta = nextSeg->height - seg->height;
        float slopeAngle = atan2f(heightDelta, segmentSize) * 180.0f / M_PI;
        
        // Positionner le sprite
        glTranslatef(spriteX, spriteY, spriteZ);
        
        // Orienter le sprite perpendiculairement à la route
        glRotatef(-roadAngle, 0.0f, 1.0f, 0.0f);  // Rotation autour de Y pour suivre la courbe
        glRotatef(slopeAngle, 1.0f, 0.0f, 0.0f);   // Rotation autour de X pour suivre la pente
        
        if (seg->sprite->type == 0) {
            // Rocher (cube marron)
            glColor3f(0.4f, 0.3f, 0.2f);  // Marron
            glTranslatef(0.0f, 5.0f, 0.0f);  // Légèrement au-dessus du sol
            glutSolidCube(10.0f);
        } else {
            // Arbre (cube vert)
            glColor3f(0.1f, 0.6f, 0.1f);  // Vert foncé
            glTranslatef(0.0f, 15.0f, 0.0f);  // Plus haut que le rocher
            glutSolidCube(30.0f);
        }
        
        glPopMatrix();
    }
}
void drawPlayerCar(void) {
    glPushMatrix();
    
    // La voiture est positionnée DANS LE REPERE MONDE 3D
    // mais à une position qui reste fixe par rapport à la caméra
    
    // Position de la voiture : légèrement devant et en dessous de la caméra
    float carX = 0.0f;           // Centrée horizontalement (pas de décalage latéral)
    float carY = -20.0f;         // En dessous de la caméra (sur le "sol" visible)
    float carZ = 80.0f;          // Devant la caméra (distance visible)
    
    glTranslatef(carX, carY, carZ);
    
    // Rotation pour orienter la voiture dans le bon sens
    // (dépend de comment vous modélisez la voiture)
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);  // Faire face à la route
    
    // Couleur de la voiture (rouge)
    glColor3f(1.0f, 0.0f, 0.0f);
    
    // Dessiner la voiture comme un cube simple (temporaire)
    // Plus tard, remplacez par un vrai modèle 3D
    glutSolidCube(15.0f);
    
    glPopMatrix();
}
void displayManual(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    int currentSegment = (int)(cameraPosition / segmentSize);
    float segmentProgress = (cameraPosition - currentSegment * segmentSize) / segmentSize;

    // Segment actuel et suivant (pour interpolation comme dans racer.js)
    RoadSegment* currentSeg = &road->segments[currentSegment % road->count];
    RoadSegment* nextSeg = &road->segments[(currentSegment + 1) % road->count];
    
    // Calculer baseOffset (comme dans racer.js)
    // C'est l'interpolation de la courbe entre le segment actuel et le suivant
    float baseOffset = currentSeg->curve + (nextSeg->curve - currentSeg->curve) * segmentProgress;
    
    // Calculer la hauteur interpolée
    float playerHeight = currentSeg->height + (nextSeg->height - currentSeg->height) * segmentProgress;
    
    float road_climb = (nextSeg->height - currentSeg->height) / segmentSize;

    // Position de la caméra
    float camX = baseOffset;  // Suivre exactement la courbe de la route
    float camY = playerHeight + cameraHeight;
    float camZ = 0.0f;
    // === Transformations manuelles ===
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    // 1. Incliner la vue vers le bas pour voir la route devant

    float pitchAngle = atanf(road_climb) * 180.0f / M_PI;
    static float pitchValue = 0.0f;
    if (pitchValue < pitchAngle ) pitchValue += 0.1f;
    else if (pitchValue > pitchAngle) pitchValue -= 0.1f;
    if (pitchValue < -30.0f) pitchValue = -30.0f;
    glRotatef(pitchValue, 1.0f, 0.0f, 0.0f);  // Rotation sur X pour regarder vers le bas
    
    glPushMatrix();
    // 1. Déplacer le monde dans la direction opposée de la caméra
    glTranslatef(-camX, -camY, -camZ);
    
    // Draw road segments
    for (int i = 0; i < 150; i++) {
        int segIndex = (currentSegment + i) % road->count;
        float z = (i * segmentSize) - (cameraPosition - currentSegment * segmentSize);
        drawRoadSegment(segIndex, z);
    }

    glPopMatrix();
    drawPlayerCar();
    // Afficher les informations de debug
    char buffer[256];
    
    sprintf(buffer, "Camera X: %.2f  Y: %.2f  Z: %.2f", camX, camY, camZ);
    drawText(10, 580, buffer);
    
    sprintf(buffer, "Road Height: %.2f  Curve: %.2f", currentSeg->height, currentSeg->curve);
    drawText(10, 560, buffer);
    
    sprintf(buffer, "Segment: %d  Position: %.2f", currentSegment, cameraPosition);
    drawText(10, 540, buffer);
    
    glutSwapBuffers();
}
void idle(void) {
    cameraPosition += 1.5f;
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
    gluPerspective(60.0, (double)w / (double)h, 1.0, 1000.0);
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
    glClearColor(0.8f, 0.85f, 0.6f, 1.0f);
    
    glutDisplayFunc(displayManual);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    
    glutMainLoop();
    
    freeRoad(road);
    return 0;
}