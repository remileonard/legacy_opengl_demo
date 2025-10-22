/*
 *	Type definitions, etc for buttonfly
 */
#define random(r) (r*(float)rand()/(float)(RAND_MAX))



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
#define WORDLENGTH 11

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