#include "file_tree.hpp"
#include "utils.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>

static void add_folder_items(const char* abs, const char* rel) {
    DIR* d = opendir(abs);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char abspath[FL_PATH_MAX];
        snprintf(abspath, sizeof(abspath), "%s/%s", abs, e->d_name);
        char relpath[FL_PATH_MAX];
        if (rel && *rel) snprintf(relpath, sizeof(relpath), "%s/%s", rel, e->d_name);
        else snprintf(relpath, sizeof(relpath), "%s", e->d_name);
        struct stat st;
        if (stat(abspath, &st) == 0) {
            file_tree->add(relpath);
            if (S_ISDIR(st.st_mode)) add_folder_items(abspath, relpath);
        }
    }
    closedir(d);
}

static void collapse_first_level() {
    Fl_Tree_Item* root = file_tree->root();
    if (!root) return;
    for (int i = 0; i < root->children(); ++i) {
        Fl_Tree_Item* child = root->child(i);
        if (child->has_children()) child->close();
    }
}

void load_folder(const char* folder) {
    strncpy(current_folder, folder, sizeof(current_folder));
    file_tree->clear();
    file_tree->root_label("");
    file_tree->showroot(false);
    add_folder_items(folder, "");
    collapse_first_level();
    save_last_folder();
}

void load_last_folder_if_any() {
    FILE* fp = fopen(last_folder_path(), "r");
    if (fp) {
        if (fgets(current_folder, sizeof(current_folder), fp)) {
            size_t len = strlen(current_folder);
            if (len && current_folder[len-1] == '\n') current_folder[len-1] = '\0';
        }
        fclose(fp);
        if (current_folder[0]) load_folder(current_folder);
    }
}

void tree_cb(Fl_Widget* w, void*) {
    Fl_Tree* tr = static_cast<Fl_Tree*>(w);
    Fl_Tree_Item* it = tr->callback_item();
    if (!it) return;
    if (tr->callback_reason() == FL_TREE_REASON_SELECTED && !it->has_children()) {
        char rel[FL_PATH_MAX];
        tr->item_pathname(rel, sizeof(rel), it);
        const char* root_lbl = file_tree->root()->label();
        if (root_lbl && *root_lbl) {
            size_t len = strlen(root_lbl);
            if (!strncmp(rel, root_lbl, len) && rel[len] == '/')
                memmove(rel, rel + len + 1, strlen(rel + len + 1) + 1);
        }
        char path[FL_PATH_MAX * 2];
        snprintf(path, sizeof(path), "%s/%s", current_folder, rel);
        load_file(path);
    }
}