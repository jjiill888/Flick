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
#include <cstdint>
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

static bool has_subdirectories(const char* dir_path) {
    if (!dir_path || !*dir_path) return false;

    DIR* d = opendir(dir_path);
    if (!d) {
        // Debug: Directory couldn't be opened
        return false;
    }

    struct dirent* e;
    bool has_dirs = false;

    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (should_ignore_item(e->d_name)) continue;

        // Build full path more carefully
        std::string full_path = std::string(dir_path);
        if (full_path.back() != '/') {
            full_path += "/";
        }
        full_path += e->d_name;

        struct stat st;
        if (stat(full_path.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Found a directory that's not ignored
                has_dirs = true;
                break;
            }
        }
    }
    closedir(d);
    return has_dirs;
}

static void load_dir_recursive(const char* dir_path, Fl_Tree_Item* parent_item, bool lazy_load = false) {
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
                // For directories in lazy load mode
                if (lazy_load) {
                    // Be optimistic: add placeholder if directory might have content
                    // We use a more permissive check - if we can't determine or if it has subdirs
                    bool should_add_placeholder = true;

                    // Only skip placeholder if we're absolutely sure directory is empty
                    DIR* check_dir = opendir(full_path.c_str());
                    if (check_dir) {
                        struct dirent* check_entry;
                        bool found_any_item = false;
                        while ((check_entry = readdir(check_dir))) {
                            if (!strcmp(check_entry->d_name, ".") || !strcmp(check_entry->d_name, "..")) continue;
                            if (should_ignore_item(check_entry->d_name)) continue;
                            found_any_item = true;
                            break;
                        }
                        closedir(check_dir);
                        should_add_placeholder = found_any_item;
                    }

                    if (should_add_placeholder) {
                        std::string placeholder_path = rel_path + "/__LAZY_LOAD_PLACEHOLDER__";
                        Fl_Tree_Item* placeholder = file_tree->add(placeholder_path.c_str());
                        if (placeholder) {
                            placeholder->label("...");
                        }
                    }
                } else {
                    // Initial load: load immediate children with lazy loading
                    load_dir_recursive(full_path.c_str(), item, true);
                }
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

    // Load and restore the expansion state for this folder
    load_tree_expansion_state();

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
    load_dir_recursive(full_path.c_str(), it, true);

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
        // Auto-scroll horizontally to make selected item visible
        if (it) {
            // Calculate item position based on tree indentation and text width
            int icon_width = 16; // Default icon width
            if (tr->openicon() && tr->openicon()->w() > 0) {
                icon_width = tr->openicon()->w();
            }
            int indent = it->depth() * icon_width;
            int label_width = 0;
            fl_font(tr->labelfont(), tr->labelsize());
            if (it->label()) {
                label_width = (int)fl_width(it->label());
            }

            int item_x = indent;
            int item_w = label_width + 40; // Add some padding for icon space

            // Get current scroll position and tree viewport
            int scroll_x = tr->hposition();
            int tree_view_w = tr->w();

            // Check if item extends beyond right edge of visible area
            if (item_x + item_w > scroll_x + tree_view_w) {
                // Scroll right to show the item with some padding
                int new_scroll_x = (item_x + item_w) - tree_view_w + 20;
                tr->hposition(new_scroll_x);
            }
            // Check if item is too far left
            else if (item_x < scroll_x) {
                // Scroll left to show the item with some padding
                int new_scroll_x = item_x - 20;
                if (new_scroll_x < 0) new_scroll_x = 0;
                tr->hposition(new_scroll_x);
            }
        }

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
    } else if (tr->callback_reason() == FL_TREE_REASON_OPENED) {
        // Auto-scroll horizontally to make expanded item visible
        if (it) {
            // Calculate item position based on tree indentation and text width
            int icon_width = 16; // Default icon width
            if (tr->openicon() && tr->openicon()->w() > 0) {
                icon_width = tr->openicon()->w();
            }
            int indent = it->depth() * icon_width;
            int label_width = 0;
            fl_font(tr->labelfont(), tr->labelsize());
            if (it->label()) {
                label_width = (int)fl_width(it->label());
            }

            int item_x = indent;
            int item_w = label_width + 40; // Add some padding for icon space

            // Get current scroll position and tree viewport
            int scroll_x = tr->hposition();
            int tree_view_w = tr->w();

            // Check if expanded item extends beyond right edge of visible area
            if (item_x + item_w > scroll_x + tree_view_w) {
                // Scroll right to show the expanded item with some padding
                int new_scroll_x = (item_x + item_w) - tree_view_w + 20;
                tr->hposition(new_scroll_x);
            }
        }

        // Handle lazy loading when a directory is expanded
        if (it->has_children()) {
            // Check if this directory has placeholder children
            bool has_placeholder = false;
            std::vector<Fl_Tree_Item*> placeholders_to_remove;

            // Find all placeholder children
            for (int i = 0; i < it->children(); ++i) {
                Fl_Tree_Item* child = it->child(i);
                if (child) {
                    // Get the child's path to check for placeholder pattern
                    char child_path[FL_PATH_MAX];
                    file_tree->item_pathname(child_path, sizeof(child_path), child);
                    const char* label = child->label();

                    // Check for placeholder by path pattern or label content
                    if ((label && (strstr(label, "...") || strstr(label, "Loading..."))) ||
                        strstr(child_path, "__LAZY_LOAD_PLACEHOLDER__") ||
                        strstr(child_path, "__PLACEHOLDER_")) {
                        placeholders_to_remove.push_back(child);
                        has_placeholder = true;
                    }
                }
            }

            if (has_placeholder) {
                // Remove all placeholders - iterate backwards to avoid index issues
                for (int i = placeholders_to_remove.size() - 1; i >= 0; --i) {
                    file_tree->remove(placeholders_to_remove[i]);
                }

                // Build the full path more carefully
                char item_path[FL_PATH_MAX];
                file_tree->item_pathname(item_path, sizeof(item_path), it);

                // Remove root label prefix if present - be more careful with deep paths
                const char* root_lbl = file_tree->root()->label();
                char* clean_path = item_path;
                if (root_lbl && *root_lbl) {
                    // Extract just the folder name from root label (remove icon if present)
                    const char* root_name = root_lbl;
                    if (strlen(root_lbl) > 2 && root_lbl[1] == ' ') {
                        root_name = root_lbl + 2; // Skip icon
                    }

                    size_t len = strlen(root_name);
                    if (!strncmp(item_path, root_name, len) && (item_path[len] == '/' || item_path[len] == '\0')) {
                        if (item_path[len] == '/') {
                            clean_path = item_path + len + 1;
                        } else {
                            clean_path = item_path + len;
                        }
                    }
                }

                // Build full filesystem path
                std::string full_path;
                if (strlen(clean_path) > 0) {
                    full_path = std::string(current_folder) + "/" + clean_path;
                } else {
                    full_path = current_folder;
                }

                // Load directory contents and check if any children were actually added
                int children_before = it->children();
                load_dir_recursive(full_path.c_str(), it, true);
                int children_after = it->children();

                // If no children were added (directory is empty or has no displayable content),
                // the directory will naturally not show a "+" icon, which is correct

                file_tree->redraw();
            }
        }

        // Save expansion state after loading directory
        save_tree_expansion_state();
    } else if (tr->callback_reason() == FL_TREE_REASON_CLOSED) {
        // Save expansion state when an item is collapsed
        save_tree_expansion_state();
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

// ======================
// Tree Expansion State Persistence
// ======================

const char* tree_expansion_state_path() {
    static char path[FL_PATH_MAX];
    const char* home = getenv("HOME");
    if (home) {
        snprintf(path, sizeof(path), "%s/.flick_tree_expansion_%s", home, fl_filename_name(current_folder));
    } else {
        snprintf(path, sizeof(path), ".flick_tree_expansion_%s", fl_filename_name(current_folder));
    }
    return path;
}

static void collect_expanded_paths(Fl_Tree_Item* item, const std::string& parent_path, std::vector<std::string>& expanded_paths) {
    if (!item) return;

    // Build the current item's path
    std::string current_path;
    if (parent_path.empty()) {
        // Root level - just use the item label (without icon)
        const char* label = item->label();
        if (label) {
            const char* clean_label = label;
            // Skip icon if present
            if (strlen(label) > 2 && label[1] == ' ') {
                clean_label = label + 2;
            }
            current_path = clean_label;
        }
    } else {
        // Child item - build path from parent
        const char* label = item->label();
        if (label) {
            const char* clean_label = label;
            // Skip icon if present
            if (strlen(label) > 2 && label[1] == ' ') {
                clean_label = label + 2;
            }
            current_path = parent_path + "/" + clean_label;
        }
    }

    // If this item is expanded and has children, save its path
    if (item->is_open() && item->has_children()) {
        expanded_paths.push_back(current_path);
    }

    // Recursively collect from children
    for (int i = 0; i < item->children(); ++i) {
        Fl_Tree_Item* child = item->child(i);
        if (child) {
            collect_expanded_paths(child, current_path, expanded_paths);
        }
    }
}

void save_tree_expansion_state() {
    if (!file_tree || !file_tree->root()) return;

    std::vector<std::string> expanded_paths;

    // Collect expanded paths from all root children
    Fl_Tree_Item* root = file_tree->root();
    for (int i = 0; i < root->children(); ++i) {
        Fl_Tree_Item* child = root->child(i);
        if (child) {
            collect_expanded_paths(child, "", expanded_paths);
        }
    }

    // Save to file
    FILE* fp = fopen(tree_expansion_state_path(), "w");
    if (fp) {
        for (const std::string& path : expanded_paths) {
            fprintf(fp, "%s\n", path.c_str());
        }
        fclose(fp);
    }
}

static void expand_path_in_tree(const std::string& target_path) {
    if (!file_tree || target_path.empty()) return;

    // Split the path into components
    std::vector<std::string> path_components;
    std::string current_component;
    for (char c : target_path) {
        if (c == '/') {
            if (!current_component.empty()) {
                path_components.push_back(current_component);
                current_component.clear();
            }
        } else {
            current_component += c;
        }
    }
    if (!current_component.empty()) {
        path_components.push_back(current_component);
    }

    // Navigate through the tree and expand items
    Fl_Tree_Item* current_item = file_tree->root();
    std::string built_path;

    for (const std::string& component : path_components) {
        if (!built_path.empty()) built_path += "/";
        built_path += component;

        // Find the child item with this component name
        Fl_Tree_Item* found_child = nullptr;
        for (int i = 0; current_item && i < current_item->children(); ++i) {
            Fl_Tree_Item* child = current_item->child(i);
            if (child && child->label()) {
                const char* label = child->label();
                // Skip icon if present
                if (strlen(label) > 2 && label[1] == ' ') {
                    label = label + 2;
                }
                if (component == label) {
                    found_child = child;
                    break;
                }
            }
        }

        if (found_child) {
            current_item = found_child;
            // Expand this item
            current_item->open();

            // If this item has placeholder children, trigger lazy loading
            if (current_item->has_children()) {
                bool has_placeholder = false;
                for (int i = 0; i < current_item->children(); ++i) {
                    Fl_Tree_Item* child = current_item->child(i);
                    if (child && child->label()) {
                        const char* label = child->label();
                        if (strstr(label, "...") || strstr(label, "Loading...")) {
                            has_placeholder = true;
                            break;
                        }
                    }
                }

                if (has_placeholder) {
                    // Remove placeholders and load directory contents
                    std::vector<Fl_Tree_Item*> placeholders_to_remove;
                    for (int i = 0; i < current_item->children(); ++i) {
                        Fl_Tree_Item* child = current_item->child(i);
                        if (child && child->label()) {
                            const char* label = child->label();
                            if (strstr(label, "...") || strstr(label, "Loading...")) {
                                placeholders_to_remove.push_back(child);
                            }
                        }
                    }

                    // Remove placeholders
                    for (int i = placeholders_to_remove.size() - 1; i >= 0; --i) {
                        file_tree->remove(placeholders_to_remove[i]);
                    }

                    // Build full filesystem path for this item
                    std::string full_path = std::string(current_folder) + "/" + built_path;

                    // Load directory contents
                    load_dir_recursive(full_path.c_str(), current_item, true);
                }
            }
        } else {
            // Path component not found - might need lazy loading
            // Check if current_item has placeholder children that need loading
            if (current_item && current_item->has_children()) {
                bool has_placeholder = false;
                for (int i = 0; i < current_item->children(); ++i) {
                    Fl_Tree_Item* child = current_item->child(i);
                    if (child && child->label()) {
                        const char* label = child->label();
                        if (strstr(label, "...") || strstr(label, "Loading...")) {
                            has_placeholder = true;
                            break;
                        }
                    }
                }

                if (has_placeholder) {
                    // Remove placeholders and load directory contents
                    std::vector<Fl_Tree_Item*> placeholders_to_remove;
                    for (int i = 0; i < current_item->children(); ++i) {
                        Fl_Tree_Item* child = current_item->child(i);
                        if (child && child->label()) {
                            const char* label = child->label();
                            if (strstr(label, "...") || strstr(label, "Loading...")) {
                                placeholders_to_remove.push_back(child);
                            }
                        }
                    }

                    // Remove placeholders
                    for (int i = placeholders_to_remove.size() - 1; i >= 0; --i) {
                        file_tree->remove(placeholders_to_remove[i]);
                    }

                    // Build full filesystem path for current item
                    std::string parent_path;
                    if (built_path.find('/') != std::string::npos) {
                        parent_path = built_path.substr(0, built_path.find_last_of('/'));
                    }
                    std::string full_path = parent_path.empty() ? current_folder : std::string(current_folder) + "/" + parent_path;

                    // Load directory contents
                    load_dir_recursive(full_path.c_str(), current_item, true);

                    // Try to find the child again after loading
                    for (int i = 0; current_item && i < current_item->children(); ++i) {
                        Fl_Tree_Item* child = current_item->child(i);
                        if (child && child->label()) {
                            const char* label = child->label();
                            // Skip icon if present
                            if (strlen(label) > 2 && label[1] == ' ') {
                                label = label + 2;
                            }
                            if (component == label) {
                                found_child = child;
                                current_item = found_child;
                                current_item->open();
                                break;
                            }
                        }
                    }
                }
            }

            // If still not found after lazy loading, stop here
            if (!found_child) {
                break;
            }
        }
    }
}

void load_tree_expansion_state() {
    FILE* fp = fopen(tree_expansion_state_path(), "r");
    if (!fp) return;

    char line[FL_PATH_MAX];
    while (fgets(line, sizeof(line), fp)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        // Expand this path in the tree
        expand_path_in_tree(std::string(line));
    }

    fclose(fp);

    // Redraw the tree to show the expanded state
    if (file_tree) {
        file_tree->redraw();
    }
}