#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include <string>

#ifdef WIN32
#include <windows.h>
#endif

class CustomTitleBar : public Fl_Widget {
public:
    CustomTitleBar(int x, int y, int w, int h, const char* title = nullptr);
    ~CustomTitleBar();
    
    void draw() override;
    int handle(int event) override;
    
    // Title and window management
    void set_title(const char* title);
    const char* get_title() const { return title_.c_str(); }
    
    // Menu integration
    void set_menu_bar(Fl_Menu_Bar* menu) { menu_bar_ = menu; }
    Fl_Menu_Bar* get_menu_bar() { return menu_bar_; }
    
    // Window state callbacks
    void on_close_callback(Fl_Callback* cb, void* data = nullptr) { close_cb_ = cb; close_data_ = data; }
    void on_minimize_callback(Fl_Callback* cb, void* data = nullptr) { minimize_cb_ = cb; minimize_data_ = data; }
    void on_maximize_callback(Fl_Callback* cb, void* data = nullptr) { maximize_cb_ = cb; maximize_data_ = data; }
    
    // Window state management
    void set_maximized(bool maximized) { is_maximized_ = maximized; }
    bool is_maximized() const { return is_maximized_; }
    
    // Theme support
    void set_theme_colors(Fl_Color bg_color, Fl_Color text_color, Fl_Color button_color);
    
    // Constants
    static constexpr int TITLE_BAR_HEIGHT = 30;
    static constexpr int BUTTON_WIDTH = 46;
    static constexpr int BUTTON_HEIGHT = 30;
    static constexpr int BUTTON_SPACING = 0;
    
private:
    std::string title_;
    Fl_Menu_Bar* menu_bar_;
    
    // Window control button areas
    struct ButtonArea {
        int x, y, w, h;
        bool hovered;
        bool pressed;
        ButtonArea() : x(0), y(0), w(0), h(0), hovered(false), pressed(false) {}
        ButtonArea(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_), hovered(false), pressed(false) {}
        bool contains(int px, int py) const {
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    };
    
    ButtonArea close_btn_;
    ButtonArea maximize_btn_;
    ButtonArea minimize_btn_;
    
    // Dragging state
    bool dragging_;
    int drag_start_x_;
    int drag_start_y_;
    int window_start_x_;
    int window_start_y_;
    
    // Double-click detection
    double last_click_time_;
    int last_click_x_;
    int last_click_y_;
    static constexpr double DOUBLE_CLICK_TIME = 0.5;
    static constexpr int DOUBLE_CLICK_DISTANCE = 5;
    
    // Window state
    bool is_maximized_;
    int restore_x_, restore_y_, restore_w_, restore_h_;
    
    // Callbacks
    Fl_Callback* close_cb_;
    void* close_data_;
    Fl_Callback* minimize_cb_;
    void* minimize_data_;
    Fl_Callback* maximize_cb_;
    void* maximize_data_;
    
    // Theme colors
    Fl_Color bg_color_;
    Fl_Color text_color_;
    Fl_Color button_color_;
    Fl_Color button_hover_color_;
    Fl_Color button_pressed_color_;
    
    // Helper methods
    void update_button_positions();
    void draw_window_buttons();
    void draw_button(const ButtonArea& btn, const char* symbol, Fl_Color bg_color);
    void draw_title_text();
    void handle_button_click(const ButtonArea& btn);
    void handle_window_drag(int event);
    void handle_double_click();
    bool is_in_draggable_area(int x, int y) const;
    void start_window_move();
    void perform_window_move(int new_x, int new_y);
    void toggle_maximize();
    void minimize_window();
    void close_window();
    
    // Platform-specific helpers
    void* get_window_handle();
    void set_window_position(int x, int y);
    void get_window_position(int& x, int& y);
    void get_window_size(int& w, int& h);
    void set_window_size(int w, int h);
    
#ifdef WIN32
    HWND get_hwnd();
#endif
};