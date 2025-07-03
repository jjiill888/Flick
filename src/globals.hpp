#pragma once
#include "editor_window.hpp"
#include <FL/filename.H>

extern Fl_Double_Window* win;
extern Fl_Menu_Bar* menu;
extern Fl_Tree* file_tree;
extern int font_size;
extern Fl_Menu_Button* context_menu;
extern My_Text_Editor* editor;
extern Fl_Text_Buffer* buffer;
extern Fl_Text_Buffer* style_buffer;
extern bool text_changed;
extern char current_file[FL_PATH_MAX];
extern char current_folder[FL_PATH_MAX];
extern Fl_Box* status_left;
extern Fl_Box* status_right;
extern Fl_Box* tree_resizer;
extern Fl_Menu_Button* tree_context_menu;
extern time_t last_save_time;
extern int tree_width;
extern Theme current_theme;