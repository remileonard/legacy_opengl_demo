#include <GL/freeglut.h>
#include <stdlib.h>
#include <string.h>
#include "event.h"
#include "irisgl_compat.h"

#define MAX_EVENTS 100
#define MAX_UPDATES 50

// Structure pour stocker les événements enregistrés
typedef struct {
    int window;
    int device;
    int value;
    void (*callback)(void *, int);
    void *data;
} EventHandler;

// Structure pour les updates
typedef struct {
    int *active_flag;
    void (*callback)(void *);
    void *data;
} UpdateHandler;

static EventHandler events[MAX_EVENTS];
static int event_count = 0;

static UpdateHandler updates[MAX_UPDATES];
static int update_count = 0;

int context, state, device;

void add_event(int win, int dev, int val, void (*fn)(void *, int), char *data) {
    if (event_count < MAX_EVENTS) {
        events[event_count].window = win;
        events[event_count].device = dev;
        events[event_count].value = val;
        events[event_count].callback = fn;
        events[event_count].data = data;
        event_count++;
    }
}

void add_update(int *flag, void (*fn)(void *), char *data) {
    if (update_count < MAX_UPDATES) {
        updates[update_count].active_flag = flag;
        updates[update_count].callback = fn;
        updates[update_count].data = data;
        update_count++;
    }
}

// Cette fonction est appelée par glutIdleFunc
void event(void) {
    // Traiter les updates actives (animations, etc.)
    for (int i = 0; i < update_count; i++) {
        if (updates[i].active_flag && *(updates[i].active_flag)) {
            updates[i].callback(updates[i].data);
        }
    }
    
    // Demander un rafraîchissement si nécessaire
    glutPostRedisplay();
}