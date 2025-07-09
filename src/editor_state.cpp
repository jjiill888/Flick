#include "editor_state.hpp"
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <cstdio>
#include <cstdlib>
#include <cstring>

EditorState& EditorState::getInstance() {
    static EditorState instance;
    return instance;
}

void EditorState::updateTitle() {
    if (!win) return;
    
    std::string title = "Let's Code";
    if (strlen(this->current_file) > 0) {
        title += " - ";
        title += fl_filename_name(this->current_file);
        if (this->text_changed) {
            title += " *";
        }
    }
    win->label(title.c_str());
}

void EditorState::updateStatus() {
    if (!status_left || !status_right || !editor || !buffer) return;
    
    // Update left status bar
    char left_status[256];
    int line = editor->linenumber_width() > 0 ? 
               buffer->line_start(editor->insert_position()) + 1 : 0;
    int col = editor->insert_position() - buffer->line_start(editor->insert_position()) + 1;
    snprintf(left_status, sizeof(left_status), "Line %d, Col %d", line, col);
    status_left->label(left_status);
    
    // Update right status bar
    char right_status[256];
    int total_lines = buffer->count_lines(0, buffer->length()) + 1;
    int total_chars = buffer->length();
    snprintf(right_status, sizeof(right_status), "Total lines: %d | Chars: %d", total_lines, total_chars);
    status_right->label(right_status);
}

void EditorState::setFontSize(int size) {
    font_size = size;
    if (editor) {
        editor->textsize(size);
        editor->damage(FL_DAMAGE_ALL);
    }
    saveSettings();
}

void EditorState::applyTheme(Theme theme) {
    current_theme = theme;
    
    if (!win || !editor || !status_left || !status_right) return;
    
    if (theme == THEME_DARK) {
        // Dark theme
        win->color(fl_rgb_color(30, 30, 30));
        editor->color(fl_rgb_color(40, 40, 40));
        editor->textcolor(fl_rgb_color(220, 220, 220));
        status_left->color(fl_rgb_color(50, 50, 50));
        status_left->labelcolor(fl_rgb_color(220, 220, 220));
        status_right->color(fl_rgb_color(50, 50, 50));
        status_right->labelcolor(fl_rgb_color(220, 220, 220));
    } else {
        // Light theme
        win->color(fl_rgb_color(240, 240, 240));
        editor->color(fl_rgb_color(255, 255, 255));
        editor->textcolor(fl_rgb_color(0, 0, 0));
        status_left->color(fl_rgb_color(230, 230, 230));
        status_left->labelcolor(fl_rgb_color(0, 0, 0));
        status_right->color(fl_rgb_color(230, 230, 230));
        status_right->labelcolor(fl_rgb_color(0, 0, 0));
    }
    
    if (win) win->redraw();
    saveSettings();
}

void EditorState::saveSettings() {
    const char* home = getenv("HOME");
    if (!home) return;
    
    char config_dir[FL_PATH_MAX];
    snprintf(config_dir, sizeof(config_dir), "%s/.lets_code", home);
    
    // Create config directory
    #ifdef _WIN32
    _mkdir(config_dir);
    #else
    mkdir(config_dir, 0755);
    #endif
    
    // Save font size
    char font_size_file[FL_PATH_MAX];
    snprintf(font_size_file, sizeof(font_size_file), "%s/font_size", config_dir);
    FILE* fp = fopen(font_size_file, "w");
    if (fp) {
        fprintf(fp, "%d", font_size);
        fclose(fp);
    }
    
    // Save theme setting
    char theme_file[FL_PATH_MAX];
    snprintf(theme_file, sizeof(theme_file), "%s/theme", config_dir);
    fp = fopen(theme_file, "w");
    if (fp) {
        fprintf(fp, "%d", current_theme);
        fclose(fp);
    }
    
    // Save tree width
    char tree_width_file[FL_PATH_MAX];
    snprintf(tree_width_file, sizeof(tree_width_file), "%s/tree_width", config_dir);
    fp = fopen(tree_width_file, "w");
    if (fp) {
        fprintf(fp, "%d", tree_width);
        fclose(fp);
    }
}

void EditorState::loadSettings() {
    const char* home = getenv("HOME");
    if (!home) return;
    
    char config_dir[FL_PATH_MAX];
    snprintf(config_dir, sizeof(config_dir), "%s/.lets_code", home);
    
    // Load font size
    char font_size_file[FL_PATH_MAX];
    snprintf(font_size_file, sizeof(font_size_file), "%s/font_size", config_dir);
    FILE* fp = fopen(font_size_file, "r");
    if (fp) {
        int size;
        if (fscanf(fp, "%d", &size) == 1) {
            font_size = size;
        }
        fclose(fp);
    }
    
    // Load theme setting
    char theme_file[FL_PATH_MAX];
    snprintf(theme_file, sizeof(theme_file), "%s/theme", config_dir);
    fp = fopen(theme_file, "r");
    if (fp) {
        int theme;
        if (fscanf(fp, "%d", &theme) == 1) {
            current_theme = static_cast<Theme>(theme);
        }
        fclose(fp);
    }
    
    // Load tree width
    char tree_width_file[FL_PATH_MAX];
    snprintf(tree_width_file, sizeof(tree_width_file), "%s/tree_width", config_dir);
    fp = fopen(tree_width_file, "r");
    if (fp) {
        int width;
        if (fscanf(fp, "%d", &width) == 1) {
            tree_width = width;
        }
        fclose(fp);
    }
} 