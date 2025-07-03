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
    // 保存当前文件夹路径
    strncpy(current_folder, folder, sizeof(current_folder));
    current_folder[sizeof(current_folder) - 1] = '\0';

    // ✅ 提前声明 len
    size_t len = strlen(current_folder);
    while (len > 1 && (current_folder[len - 1] == '/' || current_folder[len - 1] == '\\')) {
        current_folder[len - 1] = '\0';
        --len;
    }

    // 清空旧树结构并启用根节点显示
    file_tree->clear();
    file_tree->showroot(true);

    // 提取文件夹名作为根节点 label
    const char* base = fl_filename_name(current_folder);
    strncpy(root_label_buf, base ? base : current_folder, sizeof(root_label_buf));
    root_label_buf[sizeof(root_label_buf) - 1] = '\0';

    // 设置 root label（逻辑路径）
    file_tree->root_label(root_label_buf);

    // 加载文件夹内容
    add_folder_items(folder, "");

    // ✅ 强制设置根节点 label（解决显示为 "ROOT" 的问题）
    Fl_Tree_Item* root_item = file_tree->root();
    if (root_item) {
        root_item->label(root_label_buf);
        root_item->open();  // 可选：默认展开根节点
    }

    // 折叠一级目录
    collapse_first_level();

    // 保存上次打开的文件夹路径
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
        if (it->has_children()) {
            int bx_refresh = file_tree->x() + file_tree->w() - item_refresh_btn->w() -
                             item_new_file_btn->w() - item_new_folder_btn->w() - 6;
            int by = Fl::event_y() - item_refresh_btn->h()/2;
            item_new_folder_btn->position(bx_refresh, by);
            item_new_folder_btn->callback(new_folder_cb, it);
            item_new_folder_btn->show();
            bx_refresh += item_new_folder_btn->w() + 2;
            item_new_file_btn->position(bx_refresh, by);
            item_new_file_btn->callback(new_file_cb, it);
            item_new_file_btn->show();
            bx_refresh += item_new_file_btn->w() + 2;
            item_refresh_btn->position(bx_refresh, by);
            item_refresh_btn->callback(refresh_subdir_cb, it);
            item_refresh_btn->show();
        } else {
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
            item_new_folder_btn->hide();
            item_new_file_btn->hide();
            item_refresh_btn->hide();
        }
     }
}