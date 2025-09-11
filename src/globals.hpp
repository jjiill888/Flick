#pragma once

// Include necessary headers for forward declarations
#include <FL/Fl.H>
#include <FL/filename.H>
#include <ctime>

// Forward declarations
class Fl_Double_Window;
class Fl_Menu_Bar;
class Fl_Tree;
class Fl_Menu_Button;
class Fl_Box;
class Fl_Text_Buffer;
class My_Text_Editor;
class Fl_Widget;
class TabBar;

// Theme enum declaration
enum Theme { THEME_DARK, THEME_LIGHT };

// Global variable declarations
extern Fl_Double_Window *win;
extern Fl_Menu_Bar    *menu;
extern Fl_Tree        *file_tree;
extern int font_size;
extern Fl_Menu_Button *context_menu;
extern My_Text_Editor  *editor;
extern Fl_Text_Buffer  *buffer;
extern Fl_Text_Buffer  *style_buffer;
extern bool text_changed;
extern bool switching_tabs;
extern char current_file[FL_PATH_MAX];
extern char current_folder[FL_PATH_MAX];
extern Fl_Box          *status_left;
extern Fl_Box          *status_right;
extern Fl_Box          *tree_resizer;
extern Fl_Menu_Button *tree_context_menu;
extern TabBar          *tab_bar;
extern time_t           last_save_time;
extern int             tree_width;
extern Theme           current_theme;

// Window position and size variables
extern int             window_x;
extern int             window_y;
extern int             window_w;
extern int             window_h;

// Function declarations
void style_init();
void update_linenumber_width();
void set_font_size(int sz);
void save_font_size(int sz);
int load_font_size(void);
void save_last_file(void);
void load_last_file_if_any(void);
void update_title(void);
void update_status(void);
void changed_cb(int, int, int, int, const char*, void*);
void new_cb(Fl_Widget*, void*);
void load_file(const char *file);
void open_cb(Fl_Widget*, void*);
void open_folder_cb(Fl_Widget*, void*);
void refresh_folder_cb(Fl_Widget*, void*);
void refresh_subdir_cb(Fl_Widget*, void*);
void delete_cb(Fl_Widget*, void*);
void new_file_cb(Fl_Widget*, void*);
void new_folder_cb(Fl_Widget*, void*);
void save_to(const char *file);
void save_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);
void apply_theme(Theme theme);
void theme_light_cb(Fl_Widget*, void*);
void theme_dark_cb(Fl_Widget*, void*);
void cut_cb(Fl_Widget*, void*);
void copy_cb(Fl_Widget*, void*);
void paste_cb(Fl_Widget*, void*);
void select_all_cb(Fl_Widget*, void*);
void count_in_file(const char* file, const char* search, int* count);
void replace_in_file(const char* file, const char* search, const char* replace);
int highlight_in_buffer(const char* search, int* first_pos);
void count_in_folder(const char* folder, const char* search, int* count);
void replace_in_folder(const char* folder, const char* search, const char* replace);
void find_cb(Fl_Widget*, void*);
void replace_cb(Fl_Widget*, void*);
void global_search_cb(Fl_Widget*, void*);
void load_folder(const char* folder);
void load_last_folder_if_any(void);
void refresh_tree_item(class Fl_Tree_Item* it);
void tree_cb(Fl_Widget* w, void*);

// Window position and size functions
void save_window_state(void);
void load_window_state(void);
const char* window_state_path(void);