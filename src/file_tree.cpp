#include "file_tree.hpp"
#include "utils.hpp"
#include <FL/filename.H>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>

static char root_label_buf[FL_PATH_MAX];

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
            Fl_Tree_Item* item = file_tree->add(relpath);
            if (S_ISDIR(st.st_mode)) {
                add_folder_items(abspath, relpath);
                if (item) item->close();
            }
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
    current_folder[sizeof(current_folder) - 1] = '\0';
    size_t len = strlen(current_folder);
    while (len > 1 && (current_folder[len - 1] == '/' || current_folder[len - 1] == '\\')) {
        current_folder[len - 1] = '\0';
        --len;
    }

    file_tree->clear();
    file_tree->showroot(true);
    const char* base = fl_filename_name(current_folder);
    strncpy(root_label_buf, base ? base : current_folder, sizeof(root_label_buf));
    root_label_buf[sizeof(root_label_buf) - 1] = '\0';
    file_tree->root_label(root_label_buf);
    add_folder_items(folder, "");
    Fl_Tree_Item* root_item = file_tree->root();
    if (root_item) {
        root_item->label(root_label_buf);
        root_item->open();  
    }

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

void refresh_tree_item(Fl_Tree_Item* it) {
    if (!it) return;
    char rel[FL_PATH_MAX] = "";
    if (it != file_tree->root()) {
        file_tree->item_pathname(rel, sizeof(rel), it);
        const char* root_lbl = file_tree->root()->label();
        if (root_lbl && *root_lbl) {
            size_t len = strlen(root_lbl);
            if (!strncmp(rel, root_lbl, len) && rel[len] == '/')
                memmove(rel, rel + len + 1, strlen(rel + len + 1) + 1);
        }
    }
    char abs[FL_PATH_MAX * 2];
    if (*rel)
        snprintf(abs, sizeof(abs), "%s/%s", current_folder, rel);
    else
        snprintf(abs, sizeof(abs), "%s", current_folder);
    bool was_open = it->is_open();
    while (it->children()) {
        file_tree->remove(it->child(0));
    }
    add_folder_items(abs, rel);
    if (was_open)
        it->open();
    else
        it->close();
}

void tree_cb(Fl_Widget* w, void*) {
    Fl_Tree* tr = static_cast<Fl_Tree*>(w);
    Fl_Tree_Item* it = tr->callback_item();
    if (!it) return;
    if (tr->callback_reason() == FL_TREE_REASON_SELECTED) {
          if (!it->has_children()) {
            char rel[FL_PATH_MAX];
            tr->item_pathname(rel, sizeof(rel), it);
            const char* root_lbl = file_tree->root()->label();
            if (root_lbl && *root_lbl) {
                size_t len = strlen(root_lbl);
                if (!strncmp(rel, root_lbl, len) && rel[len] == '/')
                    memmove(rel, rel + len + 1, strlen(rel + len + 1) + 1);
            }
            char path[FL_PATH_MAX * 2];
            if (snprintf(path, sizeof(path), "%s/%s", current_folder, rel) < (int)sizeof(path))
                load_file(path);
        }
     }
}