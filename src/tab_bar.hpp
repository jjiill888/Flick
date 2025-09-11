#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_draw.H>
#include <vector>
#include <string>
#include <functional>

struct Tab {
    std::string filename;
    std::string filepath;
    bool is_active;
    bool is_modified;
    Fl_Text_Buffer* buffer;
    
    Tab(const std::string& file, const std::string& path, bool active = false, bool modified = false)
        : filename(file), filepath(path), is_active(active), is_modified(modified) {
        buffer = new Fl_Text_Buffer();
    }
    
    ~Tab() {
        if (buffer) {
            delete buffer;
        }
    }
};

class TabButton : public Fl_Button {
public:
    Tab* tab;
    std::function<void(Tab*)> on_close;
    std::function<void(Tab*)> on_select;
    
    TabButton(int X, int Y, int W, int H, Tab* t);
    void draw() override;
    int handle(int e) override;
    
private:
    bool close_hovered = false;
    int close_x, close_y, close_size;
    void calculate_close_button_bounds();
    bool is_point_in_close_button(int x, int y);
};

class TabBar : public Fl_Group {
public:
    TabBar(int X, int Y, int W, int H);
    ~TabBar();
    
    // Tab management
    void add_tab(const std::string& filename, const std::string& filepath);
    void remove_tab(const std::string& filepath);
    void set_active_tab(const std::string& filepath);
    void update_tab_modified(const std::string& filepath, bool modified);
    
    // Get current active tab
    Tab* get_active_tab();
    std::vector<Tab*> get_all_tabs();
    
    // Buffer management
    Fl_Text_Buffer* get_tab_buffer(const std::string& filepath);
    void switch_to_tab_buffer(const std::string& filepath);
    
    // Tab state persistence
    void save_tab_state();
    void load_tab_state();
    
    // Callbacks
    std::function<void(const std::string& filepath)> on_tab_selected;
    std::function<void(const std::string& filepath)> on_tab_closed;
    std::function<void(const std::string& old_path, const std::string& new_path)> on_tab_moved;
    
    void relayout_tabs();
    void draw() override;
    int handle(int e) override;
    
private:
    std::vector<Tab*> tabs;
    std::vector<TabButton*> tab_buttons;
    
    static const int TAB_HEIGHT = 22;
    static const int TAB_MIN_WIDTH = 40;
    static const int TAB_MAX_WIDTH = 150;
    static const int CLOSE_BUTTON_SIZE = 12;
    
    // Drag and drop state
    bool dragging = false;
    int drag_start_x = 0;
    Tab* dragged_tab = nullptr;
    int drag_insert_index = -1;
    
    void clear_tab_buttons();
    void create_tab_buttons();
    int calculate_tab_width();
    Tab* find_tab_by_filepath(const std::string& filepath);
    int find_tab_index_by_filepath(const std::string& filepath);
    int get_tab_index_at_position(int x);
};