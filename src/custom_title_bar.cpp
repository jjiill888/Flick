#include "custom_title_bar.hpp"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <cstdio>

#ifdef __linux__
#include <X11/Xlib.h>
#include <FL/x.H>
#endif

#ifdef WIN32
#include <windows.h>
#include <FL/x.H>
#endif

CustomTitleBar::CustomTitleBar(int x, int y, int w, int h, const char* title)
    : Fl_Widget(x, y, w, h, nullptr),
      title_(title ? title : ""),
      menu_bar_(nullptr),
      dragging_(false),
      drag_start_x_(0), drag_start_y_(0),
      window_start_x_(0), window_start_y_(0),
      last_click_time_(0),
      last_click_x_(0), last_click_y_(0),
      is_maximized_(false),
      restore_x_(0), restore_y_(0), restore_w_(0), restore_h_(0),
      close_cb_(nullptr), close_data_(nullptr),
      minimize_cb_(nullptr), minimize_data_(nullptr),
      maximize_cb_(nullptr), maximize_data_(nullptr),
      bg_color_(fl_rgb_color(45, 45, 48)),
      text_color_(FL_WHITE),
      button_color_(fl_rgb_color(45, 45, 48)),
      button_hover_color_(fl_rgb_color(70, 70, 70)),
      button_pressed_color_(fl_rgb_color(30, 30, 30))
{
    // Create integrated menu bar - it will be added separately to the window
    menu_bar_ = nullptr;  // Will be set from outside
    
    update_button_positions();
}

CustomTitleBar::~CustomTitleBar() {
    if (menu_bar_) {
        delete menu_bar_;
    }
}

void CustomTitleBar::update_button_positions() {
    int btn_y = y();
    int right_edge = x() + w();

    // Windows-style button order: minimize, maximize, close (right to left)
    close_btn_ = ButtonArea(right_edge - BUTTON_WIDTH, btn_y, BUTTON_WIDTH, BUTTON_HEIGHT);
    maximize_btn_ = ButtonArea(right_edge - 2 * BUTTON_WIDTH, btn_y, BUTTON_WIDTH, BUTTON_HEIGHT);
    minimize_btn_ = ButtonArea(right_edge - 3 * BUTTON_WIDTH, btn_y, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void CustomTitleBar::resize(int x, int y, int w, int h) {
    Fl_Widget::resize(x, y, w, h);
    update_button_positions();
}

void CustomTitleBar::draw() {
    // Draw full background for title bar
    fl_color(bg_color_);
    fl_rectf(x(), y(), w(), h());

    // Draw title text in the center
    draw_title_text();

    // Draw window control buttons on the right
    draw_window_buttons();

    // Draw border line at bottom for the entire width
    fl_color(fl_rgb_color(60, 60, 60));
    fl_line(x(), y() + h() - 1, x() + w(), y() + h() - 1);
}

void CustomTitleBar::draw_title_text() {
    if (title_.empty()) return;

    // Calculate available space for title (center area, avoiding buttons on right)
    int title_start = x() + 10;
    int title_end = minimize_btn_.x - 10;
    int title_width = title_end - title_start;

    if (title_width > 50) { // Only draw if we have reasonable space
        fl_color(text_color_);
        fl_font(FL_HELVETICA, 12);

        // Truncate title if too long
        std::string display_title = title_;
        int text_width = (int)fl_width(display_title.c_str());

        while (text_width > title_width - 20 && display_title.length() > 10) {
            display_title = display_title.substr(0, display_title.length() - 4) + "...";
            text_width = (int)fl_width(display_title.c_str());
        }

        // Center the title in the available space
        int text_x = title_start + (title_width - text_width) / 2;
        int text_y = y() + h() / 2 + 4; // Center vertically

        fl_draw(display_title.c_str(), text_x, text_y);
    }
}

void CustomTitleBar::draw_window_buttons() {
    // Minimize button
    draw_button(minimize_btn_, "−", minimize_btn_.hovered ? button_hover_color_ : button_color_);
    
    // Maximize/Restore button
    const char* maximize_symbol = is_maximized_ ? "❐" : "☐";
    draw_button(maximize_btn_, maximize_symbol, maximize_btn_.hovered ? button_hover_color_ : button_color_);
    
    // Close button
    Fl_Color close_bg = close_btn_.hovered ? fl_rgb_color(232, 17, 35) : button_color_;
    draw_button(close_btn_, "×", close_bg);
}

void CustomTitleBar::draw_button(const ButtonArea& btn, const char* symbol, Fl_Color bg_color) {
    // Draw button background with modern flat style
    fl_color(btn.pressed ? button_pressed_color_ : bg_color);
    fl_rectf(btn.x, btn.y, btn.w, btn.h);

    // Draw symbol with better positioning and size
    fl_color(text_color_);
    fl_font(FL_HELVETICA, 16);  // Slightly larger for better visibility

    int text_width = (int)fl_width(symbol);
    int text_height = fl_height();
    int text_x = btn.x + (btn.w - text_width) / 2;
    int text_y = btn.y + (btn.h + text_height) / 2 - 3;

    fl_draw(symbol, text_x, text_y);
}

int CustomTitleBar::handle(int event) {
    int ret = 0;
    int mx = Fl::event_x();
    int my = Fl::event_y();
    
    // Handle title bar events
    
    switch (event) {
        case FL_PUSH: {
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                // Check window control buttons first
                if (close_btn_.contains(mx, my)) {
                    close_btn_.pressed = true;
                    redraw();
                    ret = 1;
                } else if (maximize_btn_.contains(mx, my)) {
                    maximize_btn_.pressed = true;
                    redraw();
                    ret = 1;
                } else if (minimize_btn_.contains(mx, my)) {
                    minimize_btn_.pressed = true;
                    redraw();
                    ret = 1;
                } else {
                    // Allow dragging anywhere in title bar except on buttons
                    double current_time = (double)time(nullptr);
                    if (current_time - last_click_time_ < DOUBLE_CLICK_TIME &&
                        abs(mx - last_click_x_) < DOUBLE_CLICK_DISTANCE &&
                        abs(my - last_click_y_) < DOUBLE_CLICK_DISTANCE) {
                        handle_double_click();
                    } else {
                        // Start dragging
                        start_window_move();
                    }
                    last_click_time_ = current_time;
                    last_click_x_ = mx;
                    last_click_y_ = my;
                    ret = 1;
                }
            }
            break;
        }
        
        case FL_DRAG: {
            if (dragging_) {
                handle_window_drag(event);
                ret = 1;
            }
            break;
        }
        
        case FL_RELEASE: {
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                int mx = Fl::event_x();
                int my = Fl::event_y();
                
                // Handle button releases
                if (close_btn_.pressed && close_btn_.contains(mx, my)) {
                    close_window();
                } else if (maximize_btn_.pressed && maximize_btn_.contains(mx, my)) {
                    toggle_maximize();
                } else if (minimize_btn_.pressed && minimize_btn_.contains(mx, my)) {
                    minimize_window();
                }
                
                // Reset button states
                close_btn_.pressed = false;
                maximize_btn_.pressed = false;
                minimize_btn_.pressed = false;
                dragging_ = false;
                redraw();
                ret = 1;
            }
            break;
        }
        
        case FL_MOVE: {
            int mx = Fl::event_x();
            int my = Fl::event_y();
            
            // Update button hover states
            bool old_close_hover = close_btn_.hovered;
            bool old_maximize_hover = maximize_btn_.hovered;
            bool old_minimize_hover = minimize_btn_.hovered;
            
            close_btn_.hovered = close_btn_.contains(mx, my);
            maximize_btn_.hovered = maximize_btn_.contains(mx, my);
            minimize_btn_.hovered = minimize_btn_.contains(mx, my);
            
            if (old_close_hover != close_btn_.hovered ||
                old_maximize_hover != maximize_btn_.hovered ||
                old_minimize_hover != minimize_btn_.hovered) {
                redraw();
            }
            
            ret = 1;
            break;
        }
        
        case FL_ENTER:
        case FL_LEAVE: {
            // Reset hover states when leaving
            if (event == FL_LEAVE) {
                close_btn_.hovered = false;
                maximize_btn_.hovered = false;
                minimize_btn_.hovered = false;
                redraw();
            }
            ret = 1;
            break;
        }
    }
    
    return ret;
}

bool CustomTitleBar::is_in_draggable_area(int x, int y) const {
    // Not draggable if over buttons
    if (close_btn_.contains(x, y) || maximize_btn_.contains(x, y) || minimize_btn_.contains(x, y)) {
        return false;
    }
    
    // Allow dragging anywhere in the title bar except over buttons
    return true;
}

void CustomTitleBar::start_window_move() {
    if (is_maximized_) return; // Can't drag maximized windows
    
    dragging_ = true;
    drag_start_x_ = Fl::event_x_root();
    drag_start_y_ = Fl::event_y_root();
    
    Fl_Window* win = window();
    if (win) {
        window_start_x_ = win->x();
        window_start_y_ = win->y();
    }
}

void CustomTitleBar::handle_window_drag(int event) {
    if (!dragging_) return;
    
    int new_x = window_start_x_ + (Fl::event_x_root() - drag_start_x_);
    int new_y = window_start_y_ + (Fl::event_y_root() - drag_start_y_);
    
    perform_window_move(new_x, new_y);
}

void CustomTitleBar::perform_window_move(int new_x, int new_y) {
    Fl_Window* win = window();
    if (win) {
        win->position(new_x, new_y);
    }
}

void CustomTitleBar::handle_double_click() {
    toggle_maximize();
}

void CustomTitleBar::toggle_maximize() {
    Fl_Window* win = window();
    if (!win) return;
    
    if (is_maximized_) {
        // Restore window
        win->resize(restore_x_, restore_y_, restore_w_, restore_h_);
        is_maximized_ = false;
    } else {
        // Save current position and size
        restore_x_ = win->x();
        restore_y_ = win->y();
        restore_w_ = win->w();
        restore_h_ = win->h();
        
        // Maximize to screen size
        int screen_x, screen_y, screen_w, screen_h;
        Fl::screen_work_area(screen_x, screen_y, screen_w, screen_h, win->x(), win->y());
        
        win->resize(screen_x, screen_y, screen_w, screen_h);
        is_maximized_ = true;
    }
    
    if (maximize_cb_) {
        maximize_cb_(this, maximize_data_);
    }
    
    redraw();
}

void CustomTitleBar::minimize_window() {
    Fl_Window* win = window();
    if (!win) return;
    
#ifdef WIN32
    HWND hwnd = get_hwnd();
    if (hwnd) {
        ShowWindow(hwnd, SW_MINIMIZE);
    }
#elif defined(__linux__)
    // On X11, iconify the window
    win->iconize();
#endif
    
    if (minimize_cb_) {
        minimize_cb_(this, minimize_data_);
    }
}

void CustomTitleBar::close_window() {
    if (close_cb_) {
        close_cb_(this, close_data_);
    } else {
        // Default behavior: close the window
        Fl_Window* win = window();
        if (win) {
            win->hide();
        }
    }
}

void CustomTitleBar::set_title(const char* title) {
    title_ = title ? title : "";
    redraw();
}

// Menu methods removed - menu bar is now managed externally

void CustomTitleBar::set_theme_colors(Fl_Color bg_color, Fl_Color text_color, Fl_Color button_color) {
    bg_color_ = bg_color;
    text_color_ = text_color;
    button_color_ = button_color;
    button_hover_color_ = fl_color_average(button_color, FL_WHITE, 0.3f);
    button_pressed_color_ = fl_color_average(button_color, FL_BLACK, 0.3f);
    
    if (menu_bar_) {
        menu_bar_->color(bg_color_);
        menu_bar_->textcolor(text_color_);
    }
    
    redraw();
}

#ifdef WIN32
HWND CustomTitleBar::get_hwnd() {
    Fl_Window* win = window();
    if (!win) return nullptr;
    
    return fl_xid(win);
}
#endif

void* CustomTitleBar::get_window_handle() {
    Fl_Window* win = window();
    if (!win) return nullptr;
    
#ifdef WIN32
    return (void*)fl_xid(win);
#elif defined(__linux__)
    return (void*)fl_xid(win);
#else
    return nullptr;
#endif
}