#pragma once
#include <FL/filename.H>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Text_Buffer.H>
#include <ctime>
#include <string>

enum Theme { THEME_DARK, THEME_LIGHT };

class EditorState {
public:
    static EditorState& getInstance();
    
    // Window and UI components
    Fl_Double_Window* win = nullptr;
    Fl_Menu_Bar* menu = nullptr;
    Fl_Tree* file_tree = nullptr;
    Fl_Menu_Button* context_menu = nullptr;
    Fl_Text_Editor* editor = nullptr;
    Fl_Text_Buffer* buffer = nullptr;
    Fl_Text_Buffer* style_buffer = nullptr;
    Fl_Box* status_left = nullptr;
    Fl_Box* status_right = nullptr;
    Fl_Box* tree_resizer = nullptr;
    Fl_Menu_Button* tree_context_menu = nullptr;
    
    // State variables
    bool text_changed = false;
    char current_file[FL_PATH_MAX] = "";
    char current_folder[FL_PATH_MAX] = "";
    time_t last_save_time = 0;
    int tree_width = 200;
    int font_size = 14;
    Theme current_theme = THEME_DARK;
    
    // Methods
    void updateTitle();
    void updateStatus();
    void setFontSize(int size);
    void applyTheme(Theme theme);
    void saveSettings();
    void loadSettings();
    
private:
    EditorState() = default;
    EditorState(const EditorState&) = delete;
    EditorState& operator=(const EditorState&) = delete;
}; 