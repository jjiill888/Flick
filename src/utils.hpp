#pragma once
#include "globals.hpp"
#include <FL/Fl_Text_Display.H>

void new_cb(Fl_Widget*, void*);
void open_cb(Fl_Widget*, void*);
void save_cb(Fl_Widget*, void*);
void close_current_tab_cb(Fl_Widget*, void*);
void open_folder_cb(Fl_Widget*, void*);
void refresh_folder_cb(Fl_Widget*, void*);
void refresh_subdir_cb(Fl_Widget*, void*);
void new_file_cb(Fl_Widget*, void*);
void new_folder_cb(Fl_Widget*, void*);
void delete_cb(Fl_Widget*, void*);
void quit_cb(Fl_Widget*, void*);
void find_cb(Fl_Widget*, void*);
void replace_cb(Fl_Widget*, void*);
void global_search_cb(Fl_Widget*, void*);
void set_font_size(int sz);
void update_title();
void update_status();
int load_font_size();
void changed_cb(int, int, int, int, const char*, void*);
void save_to(const char* file);
void load_file(const char* file);
void load_last_file_if_any();
void save_last_file();
void save_last_folder();
const char* last_file_path();
const char* last_folder_path();
const char* font_size_path();

void apply_theme(Theme theme);
void theme_light_cb(Fl_Widget*, void*);
void theme_dark_cb(Fl_Widget*, void*);
void cut_cb(Fl_Widget*, void*);
void copy_cb(Fl_Widget*, void*);
void paste_cb(Fl_Widget*, void*);
void select_all_cb(Fl_Widget*, void*);
void update_linenumber_width();
void style_init();
// Style table declarations - defined in utils.cpp
extern Fl_Text_Display::Style_Table_Entry style_table[];
extern const int style_table_size;

































