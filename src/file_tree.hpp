#pragma once
#include "globals.hpp"

void load_folder(const char* folder);
void load_last_folder_if_any();
void tree_cb(Fl_Widget* w, void* data);
void refresh_tree_item(Fl_Tree_Item* it);

// VSCode-like features
void show_tree_context_menu(int x, int y, Fl_Tree_Item* item);
void tree_new_file_cb(Fl_Widget* w, void* data);
void tree_new_folder_cb(Fl_Widget* w, void* data);
void tree_rename_item_cb(Fl_Widget* w, void* data);
void tree_delete_item_cb(Fl_Widget* w, void* data);
void tree_copy_path_cb(Fl_Widget* w, void* data);
void tree_refresh_cb(Fl_Widget* w, void* data);
void tree_collapse_all_cb(Fl_Widget* w, void* data);
void tree_expand_all_cb(Fl_Widget* w, void* data);
int tree_handle_key(int key);