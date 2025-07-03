#pragma once
#include "globals.hpp"

void load_folder(const char* folder);
void load_last_folder_if_any();
void tree_cb(Fl_Widget* w, void* data);
void refresh_tree_item(Fl_Tree_Item* it);