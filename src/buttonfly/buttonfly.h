/*
 *	Type definitions, etc for buttonfly
 */
#pragma once
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


typedef struct popup_struct_type
{
	char *title;
	char *action;
	struct popup_struct_type *next;
} popup_struct;

typedef struct button_struct_type
{
	char name[3][20];
	int wc;		/* word count */
	char *action;
	char *submenu;
	popup_struct *popup;

	/* RGB 0 to 1 */
	float color[3];
	float backcolor[3];
	float highcolor[3];
	float default_color[3];
	float default_backcolor[3];
	float default_highcolor[3];
	struct button_struct_type *next;
	struct button_struct_type *forward;

	short ax, ay, az;
	float tx, ty, tz;
} button_struct;

typedef struct path_struct_type {

    button_struct *current_buttons;
    button_struct *button;
    struct path_struct_type *back;

} path_struct;

#define NUM_TOKENS 5
#define MAX_SPOTS 32
#define TARGET_FPS 60
#define FRAME_TIME_MS (1000 / TARGET_FPS)
#define WORDLENGTH 9
#define random(r) (r*(float)rand()/(float)(RAND_MAX))
#define BLOCK_AT_SLOW_FRAME 0

#define X 0
#define Y 1
#define Z 2

#define M_PI 3.14159265358979323846
#define XMAXSCREEN 1024
#define YMAXSCREEN 768

#define BIG	0
#define LITTLE	1

#ifdef _WIN32
    #include <process.h>
    #include <shellapi.h>
    #define CLOCK_MONOTONIC 0
    
    static int clock_gettime(int clk_id, struct timespec *spec) {
        static LARGE_INTEGER frequency;
        static int initialized = 0;
        LARGE_INTEGER count;
        
        if (!initialized) {
            QueryPerformanceFrequency(&frequency);
            initialized = 1;
        }
        
        QueryPerformanceCounter(&count);
        
        spec->tv_sec = (long)(count.QuadPart / frequency.QuadPart);
        spec->tv_nsec = (long)(((count.QuadPart % frequency.QuadPart) * 1000000000LL) / frequency.QuadPart);
        
        return 0;
    }
    // Structure pour passer les arguments au thread Windows
    typedef struct {
        char command[1024];
    } thread_args_t;
    
    // Fonction thread pour Windows - lance uniquement des exécutables
    static unsigned int __stdcall execute_command_thread(void* arg) {
        thread_args_t* args = (thread_args_t*)arg;
        
        // Analyser la commande pour extraire l'exécutable et les arguments
        char exe[512] = {0};
        char params[512] = {0};
        char* space = strchr(args->command, ' ');
        
        if (space) {
            size_t len = space - args->command;
            strncpy(exe, args->command, len < 511 ? len : 511);
            strncpy(params, space + 1, 511);
        } else {
            strncpy(exe, args->command, 511);
        }
        
        // ShellExecute lance l'exe directement sans fenêtre de commande
        HINSTANCE result = ShellExecuteA(NULL, "open", exe, 
                                       params[0] ? params : NULL, 
                                       NULL, SW_SHOWNORMAL);
        if ((INT_PTR)result <= 32) {
            fprintf(stderr, "ButtonFly: Failed to launch: %s (error: %d)\n", 
                   exe, (int)(INT_PTR)result);
        }
        
        free(args);
        return 0;
    }
    
    // Fonction pour exécuter une commande en arrière-plan (Windows)
    static void execute_async(const char* command) {
        thread_args_t* args = (thread_args_t*)malloc(sizeof(thread_args_t));
        if (args) {
            strncpy(args->command, command, sizeof(args->command) - 1);
            args->command[sizeof(args->command) - 1] = '\0';
            
            printf("ButtonFly: Launching (async): %s\n", command);
            
            uintptr_t handle = _beginthreadex(NULL, 0, execute_command_thread, args, 0, NULL);
            if (handle != 0) {
                CloseHandle((HANDLE)handle);
            } else {
                fprintf(stderr, "ButtonFly: Failed to create thread\n");
                free(args);
            }
        }
    }
#else
    #include <time.h>
    #include <unistd.h>
    #include <sys/wait.h>
    
    // Fonction pour exécuter une commande en arrière-plan (Unix)
    static void execute_async(const char* command) {
        pid_t pid = fork();
        if (pid == 0) {
            // Processus enfant - parser la commande pour extraire exe et args
            char* args[64];
            char cmd_copy[1024];
            int i = 0;
            
            strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
            cmd_copy[sizeof(cmd_copy) - 1] = '\0';
            
            char* token = strtok(cmd_copy, " ");
            while (token != NULL && i < 63) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;
            
            // Exécuter directement le programme
            execvp(args[0], args);
            
            // Si on arrive ici, exec a échoué
            fprintf(stderr, "ButtonFly: Failed to launch: %s\n", command);
            exit(1);
        } else if (pid < 0) {
            fprintf(stderr, "ButtonFly: fork() failed\n");
        }
        // Le processus parent continue immédiatement
    }
#endif

extern char *dot_tokens[NUM_TOKENS];
extern int token_nums[];
extern float *spots[32];
extern button_struct *current_button;

void draw_buttons(button_struct *buttons);
void draw_button(button_struct * button);
void push_button(button_struct *selected);
void draw_selected_button(button_struct *button,float t);
void draw_button_label(button_struct *button);
void draw_edge();
void draw_front(button_struct *button);
void draw_button_label(button_struct *button);
void add_button_to_path(button_struct *current, button_struct *button);
button_struct *load_buttons(FILE *fp);
button_struct *load_buttons_from_file(char *program_name, char *filename);
button_struct *which_button(int mx, int my);
extern button_struct *new_button(char *);

void bf_redraw();
void bf_exit();
void bf_selecting();
void bf_deselect();
void bf_quick();
void bf_fly();
void do_popup();
void toggle_window();
void bf_escdown();
void bf_escup();
void flyindraw();
void flyoutdraw();
void selectdraw();
void parse_args();
void doclear();