#include "file_tree.hpp"
#include "utils.hpp"
#include <FL/Fl_Tree.H>
#include <FL/Fl_Menu.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>
#include <set>
#include <string>
#include <unistd.h>
#include <algorithm>
#include <vector>

static char root_label_buf[FL_PATH_MAX];

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

// Configuration: Set to true for minimal icons, false for pure text-only
static const bool USE_MINIMAL_ICONS = false;

// Minimal flat icon function - VS Code style monochrome icons
static const char* get_file_icon(const char* filename, bool is_directory) {
    if (!USE_MINIMAL_ICONS) {
        return "";  // Pure text-only mode - no icons at all
    }

    if (is_directory) {
        return "▸ ";  // Simple chevron for folders
    }

    const char* ext = strrchr(filename, '.');
    if (!ext) return "◦ ";  // Simple dot for files without extension

    std::string extension(ext);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Source code files - use subtle circles
    if (extension == ".c" || extension == ".cpp" || extension == ".cc" || extension == ".cxx" ||
        extension == ".h" || extension == ".hpp" || extension == ".hxx") return "◯ ";

    // Web files - simple geometric shapes
    if (extension == ".html" || extension == ".htm") return "◇ ";
    if (extension == ".css" || extension == ".scss") return "◈ ";
    if (extension == ".js" || extension == ".ts" || extension == ".jsx" || extension == ".tsx") return "◉ ";

    // Data/Config files - simple square
    if (extension == ".json" || extension == ".xml" || extension == ".yaml" || extension == ".yml" ||
        extension == ".toml" || extension == ".ini" || extension == ".cfg" || extension == ".conf") return "◾ ";

    // Script files - simple triangle
    if (extension == ".sh" || extension == ".bash" || extension == ".zsh" || extension == ".fish" ||
        extension == ".py" || extension == ".rb" || extension == ".go" || extension == ".rs" ||
        extension == ".java" || extension == ".php" || extension == ".swift" || extension == ".kt") return "▲ ";

    // Build files - simple diamond
    if (extension == ".cmake" || extension == ".make" || extension == ".mk") return "◆ ";

    // Documents - simple dot
    if (extension == ".md" || extension == ".txt" || extension == ".rst") return "◦ ";

    return "◦ ";  // Default: simple dot
}

static bool is_source_file(const char* name) {
    std::string filename(name);
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) return false;

    std::string ext = filename.substr(dot_pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return source_extensions.find(ext) != source_extensions.end();
}

static void load_dir_recursive(const char* dir_path, Fl_Tree_Item* parent_item, int depth = 0) {
    if (depth > 3) return; // Limit recursion depth

    DIR* d = opendir(dir_path);
    if (!d) return;

    struct dirent* e;
    std::vector<std::pair<std::string, bool>> entries;

    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (should_ignore_item(e->d_name)) continue;

        std::string full_path = std::string(dir_path) + "/" + e->d_name;
        struct stat st;

        if (stat(full_path.c_str(), &st) == 0) {
            bool is_dir = S_ISDIR(st.st_mode);

            // Only include source files or directories
            if (!is_dir && !is_source_file(e->d_name)) {
                continue;
            }

            entries.push_back({e->d_name, is_dir});
        }
    }
    closedir(d);

    // Sort entries: directories first, then by name
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second; // dirs first
        return a.first < b.first; // then alphabetical
    });

    // Add entries to tree
    for (const auto& entry : entries) {
        std::string full_path = std::string(dir_path) + "/" + entry.first;

        // Build relative path from root
        std::string rel_path;
        if (parent_item == file_tree->root()) {
            rel_path = entry.first;
        } else {
            char parent_rel_path[FL_PATH_MAX];
            file_tree->item_pathname(parent_rel_path, sizeof(parent_rel_path), parent_item);

            // Remove root label prefix if present
            const char* root_lbl = file_tree->root()->label();
            if (root_lbl && *root_lbl) {
                size_t len = strlen(root_lbl);
                if (!strncmp(parent_rel_path, root_lbl, len) && parent_rel_path[len] == '/') {
                    memmove(parent_rel_path, parent_rel_path + len + 1, strlen(parent_rel_path + len + 1) + 1);
                }
            }
            rel_path = std::string(parent_rel_path) + "/" + entry.first;
        }

        Fl_Tree_Item* item = file_tree->add(rel_path.c_str());
        if (item) {
            // Set icon in label
            const char* icon = get_file_icon(entry.first.c_str(), entry.second);
            std::string labeled_name = std::string(icon) + entry.first;
            item->label(labeled_name.c_str());

            if (entry.second) {
                // Recursively load subdirectories
                load_dir_recursive(full_path.c_str(), item, depth + 1);
                item->close(); // Start collapsed
            }
        }
    }
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

    if (USE_MINIMAL_ICONS) {
        snprintf(root_label_buf, sizeof(root_label_buf), "▸ %s", fl_filename_name(current_folder));
    } else {
        snprintf(root_label_buf, sizeof(root_label_buf), "%s", fl_filename_name(current_folder));
    }

    file_tree->clear();
    file_tree->root_label(root_label_buf);

    load_dir_recursive(current_folder, file_tree->root());
    collapse_first_level();

    // Save current folder
    FILE* fp = fopen(last_folder_path(), "w");
    if (fp) {
        fprintf(fp, "%s", current_folder);
        fclose(fp);
    }

    file_tree->redraw();
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

    // Get the full path for this item
    char item_path[FL_PATH_MAX];
    file_tree->item_pathname(item_path, sizeof(item_path), it);

    // Remove root label prefix if present
    const char* root_lbl = file_tree->root()->label();
    if (root_lbl && *root_lbl) {
        size_t len = strlen(root_lbl);
        if (!strncmp(item_path, root_lbl, len) && item_path[len] == '/') {
            memmove(item_path, item_path + len + 1, strlen(item_path + len + 1) + 1);
        }
    }

    std::string full_path = std::string(current_folder) + "/" + item_path;

    // Remove all children
    while (it->children() > 0) {
        file_tree->remove(it->child(0));
    }

    // Reload directory content
    bool was_open = it->is_open();
    load_dir_recursive(full_path.c_str(), it);

    if (was_open) {
        it->open();
    } else {
        it->close();
    }

    file_tree->redraw();
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

            char full_path[FL_PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", current_folder, rel);
            load_file(full_path);
        }
    }
}

// ======================
// VSCode-like Features
// ======================

void show_tree_context_menu(int x, int y, Fl_Tree_Item* item) {
    static Fl_Menu_Item context_menu[] = {
        {"New File", 0, tree_new_file_cb, item, 0},
        {"New Folder", 0, tree_new_folder_cb, item, 0},
        {0}, // separator
        {"Rename", 0, tree_rename_item_cb, item, 0},
        {"Delete", 0, tree_delete_item_cb, item, 0},
        {0}, // separator
        {"Copy Path", 0, tree_copy_path_cb, item, 0},
        {0}, // separator
        {"Refresh", 0, tree_refresh_cb, item, 0},
        {"Collapse All", 0, tree_collapse_all_cb, item, 0},
        {"Expand All", 0, tree_expand_all_cb, item, 0},
        {0}
    };

    const Fl_Menu_Item* selected = context_menu->popup(x, y, "File Tree");
    if (selected && selected->callback_) {
        selected->callback_(nullptr, selected->user_data_);
    }
}

void tree_new_file_cb(Fl_Widget* w, void* data) {
    Fl_Tree_Item* item = static_cast<Fl_Tree_Item*>(data);
    if (!item) return;

    const char* filename = fl_input("Enter filename:", "");
    if (!filename || !*filename) return;

    // Get directory path
    char item_path[FL_PATH_MAX];
    if (item->has_children() || item == file_tree->root()) {
        // Directory selected
        if (item == file_tree->root()) {
            strcpy(item_path, "");
        } else {
            file_tree->item_pathname(item_path, sizeof(item_path), item);
            const char* root_lbl = file_tree->root()->label();
            if (root_lbl && *root_lbl) {
                size_t len = strlen(root_lbl);
                if (!strncmp(item_path, root_lbl, len) && item_path[len] == '/') {
                    memmove(item_path, item_path + len + 1, strlen(item_path + len + 1) + 1);
                }
            }
        }
    } else {
        // File selected, get parent directory
        if (item->parent() && item->parent() != file_tree->root()) {
            file_tree->item_pathname(item_path, sizeof(item_path), item->parent());
            const char* root_lbl = file_tree->root()->label();
            if (root_lbl && *root_lbl) {
                size_t len = strlen(root_lbl);
                if (!strncmp(item_path, root_lbl, len) && item_path[len] == '/') {
                    memmove(item_path, item_path + len + 1, strlen(item_path + len + 1) + 1);
                }
            }
        } else {
            strcpy(item_path, "");
        }
    }

    char full_path[FL_PATH_MAX];
    if (strlen(item_path) > 0) {
        snprintf(full_path, sizeof(full_path), "%s/%s/%s", current_folder, item_path, filename);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", current_folder, filename);
    }

    // Create file
    FILE* fp = fopen(full_path, "w");
    if (fp) {
        fclose(fp);
        load_folder(current_folder); // Refresh tree
    } else {
        fl_alert("Could not create file: %s", filename);
    }
}

void tree_new_folder_cb(Fl_Widget* w, void* data) {
    Fl_Tree_Item* item = static_cast<Fl_Tree_Item*>(data);
    if (!item) return;

    const char* foldername = fl_input("Enter folder name:", "");
    if (!foldername || !*foldername) return;

    // Get directory path
    char item_path[FL_PATH_MAX];
    if (item->has_children() || item == file_tree->root()) {
        // Directory selected
        if (item == file_tree->root()) {
            strcpy(item_path, "");
        } else {
            file_tree->item_pathname(item_path, sizeof(item_path), item);
            const char* root_lbl = file_tree->root()->label();
            if (root_lbl && *root_lbl) {
                size_t len = strlen(root_lbl);
                if (!strncmp(item_path, root_lbl, len) && item_path[len] == '/') {
                    memmove(item_path, item_path + len + 1, strlen(item_path + len + 1) + 1);
                }
            }
        }
    } else {
        // File selected, get parent directory
        if (item->parent() && item->parent() != file_tree->root()) {
            file_tree->item_pathname(item_path, sizeof(item_path), item->parent());
            const char* root_lbl = file_tree->root()->label();
            if (root_lbl && *root_lbl) {
                size_t len = strlen(root_lbl);
                if (!strncmp(item_path, root_lbl, len) && item_path[len] == '/') {
                    memmove(item_path, item_path + len + 1, strlen(item_path + len + 1) + 1);
                }
            }
        } else {
            strcpy(item_path, "");
        }
    }

    char full_path[FL_PATH_MAX];
    if (strlen(item_path) > 0) {
        snprintf(full_path, sizeof(full_path), "%s/%s/%s", current_folder, item_path, foldername);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", current_folder, foldername);
    }

    // Create directory
    if (mkdir(full_path, 0755) == 0) {
        load_folder(current_folder); // Refresh tree
    } else {
        fl_alert("Could not create folder: %s", foldername);
    }
}

void tree_rename_item_cb(Fl_Widget* w, void* data) {
    Fl_Tree_Item* item = static_cast<Fl_Tree_Item*>(data);
    if (!item || item == file_tree->root()) return;

    // Get current name (without icon)
    const char* current_label = item->label();
    const char* current_name = current_label;
    if (current_label && strlen(current_label) > 2 && current_label[1] == ' ') {
        current_name = current_label + 2; // Skip emoji and space
    }

    const char* new_name = fl_input("Rename to:", current_name);
    if (!new_name || !*new_name || strcmp(new_name, current_name) == 0) return;

    // Get old path
    char old_rel_path[FL_PATH_MAX];
    file_tree->item_pathname(old_rel_path, sizeof(old_rel_path), item);
    const char* root_lbl = file_tree->root()->label();
    if (root_lbl && *root_lbl) {
        size_t len = strlen(root_lbl);
        if (!strncmp(old_rel_path, root_lbl, len) && old_rel_path[len] == '/') {
            memmove(old_rel_path, old_rel_path + len + 1, strlen(old_rel_path + len + 1) + 1);
        }
    }

    // Build full paths
    char old_full_path[FL_PATH_MAX];
    snprintf(old_full_path, sizeof(old_full_path), "%s/%s", current_folder, old_rel_path);

    // Get parent directory
    char new_full_path[FL_PATH_MAX];
    if (item->parent() && item->parent() != file_tree->root()) {
        char parent_rel_path[FL_PATH_MAX];
        file_tree->item_pathname(parent_rel_path, sizeof(parent_rel_path), item->parent());
        if (root_lbl && *root_lbl) {
            size_t len = strlen(root_lbl);
            if (!strncmp(parent_rel_path, root_lbl, len) && parent_rel_path[len] == '/') {
                memmove(parent_rel_path, parent_rel_path + len + 1, strlen(parent_rel_path + len + 1) + 1);
            }
        }
        snprintf(new_full_path, sizeof(new_full_path), "%s/%s/%s", current_folder, parent_rel_path, new_name);
    } else {
        snprintf(new_full_path, sizeof(new_full_path), "%s/%s", current_folder, new_name);
    }

    // Rename
    if (rename(old_full_path, new_full_path) == 0) {
        load_folder(current_folder); // Refresh tree
    } else {
        fl_alert("Could not rename: %s", current_name);
    }
}

void tree_delete_item_cb(Fl_Widget* w, void* data) {
    Fl_Tree_Item* item = static_cast<Fl_Tree_Item*>(data);
    if (!item || item == file_tree->root()) return;

    // Get current name (without icon)
    const char* current_label = item->label();
    const char* current_name = current_label;
    if (current_label && strlen(current_label) > 2 && current_label[1] == ' ') {
        current_name = current_label + 2; // Skip emoji and space
    }

    if (fl_choice("Delete %s?", "Cancel", "Delete", 0, current_name) != 1) return;

    // Get path
    char rel_path[FL_PATH_MAX];
    file_tree->item_pathname(rel_path, sizeof(rel_path), item);
    const char* root_lbl = file_tree->root()->label();
    if (root_lbl && *root_lbl) {
        size_t len = strlen(root_lbl);
        if (!strncmp(rel_path, root_lbl, len) && rel_path[len] == '/') {
            memmove(rel_path, rel_path + len + 1, strlen(rel_path + len + 1) + 1);
        }
    }

    char full_path[FL_PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", current_folder, rel_path);

    // Delete
    if (item->has_children()) {
        // Directory - use rmdir or system call
        char cmd[FL_PATH_MAX + 10];
        snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", full_path);
        if (system(cmd) == 0) {
            load_folder(current_folder); // Refresh tree
        } else {
            fl_alert("Could not delete folder: %s", current_name);
        }
    } else {
        // File
        if (unlink(full_path) == 0) {
            load_folder(current_folder); // Refresh tree
        } else {
            fl_alert("Could not delete file: %s", current_name);
        }
    }
}

void tree_copy_path_cb(Fl_Widget* w, void* data) {
    Fl_Tree_Item* item = static_cast<Fl_Tree_Item*>(data);
    if (!item) return;

    // Get path
    char rel_path[FL_PATH_MAX];
    if (item == file_tree->root()) {
        strcpy(rel_path, current_folder);
    } else {
        file_tree->item_pathname(rel_path, sizeof(rel_path), item);
        const char* root_lbl = file_tree->root()->label();
        if (root_lbl && *root_lbl) {
            size_t len = strlen(root_lbl);
            if (!strncmp(rel_path, root_lbl, len) && rel_path[len] == '/') {
                memmove(rel_path, rel_path + len + 1, strlen(rel_path + len + 1) + 1);
            }
        }
        char full_path[FL_PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", current_folder, rel_path);
        strcpy(rel_path, full_path);
    }

    // Copy to clipboard (simplified)
    Fl::copy(rel_path, strlen(rel_path), 1);
    // Note: Real clipboard support would require platform-specific code
}

void tree_refresh_cb(Fl_Widget* w, void* data) {
    Fl_Tree_Item* item = static_cast<Fl_Tree_Item*>(data);
    if (item == file_tree->root()) {
        load_folder(current_folder);
    } else {
        refresh_tree_item(item);
    }
}

void tree_collapse_all_cb(Fl_Widget* w, void* data) {
    Fl_Tree_Item* root = file_tree->root();
    if (root) {
        for (int i = 0; i < root->children(); ++i) {
            Fl_Tree_Item* child = root->child(i);
            if (child->has_children()) child->close();
        }
    }
}

void tree_expand_all_cb(Fl_Widget* w, void* data) {
    Fl_Tree_Item* root = file_tree->root();
    if (root) {
        for (int i = 0; i < root->children(); ++i) {
            Fl_Tree_Item* child = root->child(i);
            if (child->has_children()) child->open();
        }
    }
}

int tree_handle_key(int key) {
    Fl_Tree_Item* selected = file_tree->item_clicked();
    if (!selected) return 0;

    switch (key) {
        case FL_F + 2: // F2 - Rename
            tree_rename_item_cb(nullptr, selected);
            return 1;

        case FL_Delete: // Delete key
            tree_delete_item_cb(nullptr, selected);
            return 1;

        case FL_F + 5: // F5 - Refresh
            tree_refresh_cb(nullptr, selected);
            return 1;

        default:
            return 0;
    }
}