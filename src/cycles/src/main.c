/*
 * main.c - GLUT entry point for Cycles
 * 
 * This file provides the GLUT initialization wrapper for Cycles.
 * The actual game code is in cycles/main.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "porting/iris2ogl.h"

// Forward declaration - the actual main is in cycles/main.c
// but we need to initialize GLUT first
extern int cycles_main_impl(int argc, char **argv);

int main(int argc, char **argv) {
    // Initialize GLUT first
    
    // Initialize the color map
    iris_init_colormap();
    
    printf("=================================\n");
    printf("  Cycles - OpenGL/GLUT Port\n");
    printf("=================================\n");
    printf("\n");
    printf("Controls:\n");
    printf("  Left Mouse    - Turn left\n");
    printf("  Right Mouse   - Turn right\n");
    printf("  Middle / A    - Accelerate\n");
    printf("  Space         - Jump\n");
    printf("  Arrow Keys    - Change view\n");
    printf("  H             - Toggle help\n");
    printf("  ESC           - Quit\n");
    printf("\n");
    
    // Call the actual Cycles main implementation
    // which is in cycles/main.c renamed to cycles_main_impl
    return cycles_main_impl(argc, argv);
}

