#include "file_tree.hpp"
#include "utils.hpp"
#include <FL/Fl_Tree.H>
#include <FL/filename.H>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>
#include <set>
#include <string>

static char root_label_buf[FL_PATH_MAX];

// Add file type filtering and depth limits
static const int MAX_DIR_DEPTH = 5;
static const int MAX_FILES_PER_DIR = 1000;

// Common source code file extensions
static const std::set<std::string> source_extensions = {
    ".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".hxx",
    ".java", ".py", ".js", ".ts", ".html", ".css", ".scss",
    ".php", ".rb", ".go", ".rs", ".swift", ".kt", ".scala",
    ".cs", ".vb", ".sql", ".sh", ".bash", ".zsh", ".fish",
    ".cmake", ".make", ".mk", ".md", ".txt", ".json", ".xml",
    ".yaml", ".yml", ".toml", ".ini", ".cfg", ".conf"
};

// Directories and files to ignore
static const std::set<std::string> ignore_patterns = {
    ".git", ".svn", ".hg", ".bzr",
    "node_modules", "vendor", "target", "build", "dist",
    ".cache", ".tmp", ".temp", "__pycache__",
    ".DS_Store", "Thumbs.db", "desktop.ini"
};

static bool should_ignore_item(const char* name) {
    std::string item_name(name);
    
    // Check ignore patterns
    for (const auto& pattern : ignore_patterns) {
        if (item_name == pattern) return true;
    }
    
    return false;
}

static bool is_source_file(const char* name) {
    std::string filename(name);
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) return false;
    
    std::string ext = filename.substr(dot_pos);
    return source_extensions.find(ext) != source_extensions.end();
}

// Forward declaration
static bool has_source_files(const char* dir_path);

static void add_folder_items(const char* abs, const char* rel, int depth = 0) {
    if (depth > MAX_DIR_DEPTH) return;
    
    DIR* d = opendir(abs);
    if (!d) return;
    
    struct dirent* e;
    int file_count = 0;
    
    while ((e = readdir(d)) && file_count < MAX_FILES_PER_DIR) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (should_ignore_item(e->d_name)) continue;
        
        char abspath[FL_PATH_MAX];
        snprintf(abspath, sizeof(abspath), "%s/%s", abs, e->d_name);
        char relpath[FL_PATH_MAX];
        if (rel && *rel) snprintf(relpath, sizeof(relpath), "%s/%s", rel, e->d_name);
        else snprintf(relpath, sizeof(relpath), "%s", e->d_name);
        
        struct stat st;
        if (stat(abspath, &st) == 0) {
            Fl_Tree_Item* item = file_tree->add(relpath);
            if (S_ISDIR(st.st_mode)) {
                // For deep directories, only show subdirectories with source files
                if (depth < 2 || has_source_files(abspath)) {
                    add_folder_items(abspath, relpath, depth + 1);
                    if (item) item->close();
                }
            } else {
                // Only show source code files
                if (!is_source_file(e->d_name)) {
                    if (item) file_tree->remove(item);
                    continue;
                }
            }
            file_count++;
        }
    }
    closedir(d);
}

// Check if directory contains source code files
static bool has_source_files(const char* dir_path) {
    DIR* d = opendir(dir_path);
    if (!d) return false;
    
    struct dirent* e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (should_ignore_item(e->d_name)) continue;
        
        char full_path[FL_PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, e->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (has_source_files(full_path)) {
                    closedir(d);
                    return true;
                }
            } else if (is_source_file(e->d_name)) {
                closedir(d);
                return true;
            }
        }
    }
    closedir(d);
    return false;
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