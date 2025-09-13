#pragma once
#include <FL/Fl_Button.H>

class DockButton : public Fl_Button {
private:
    bool is_hovered_;
    bool is_pressed_;
    bool is_active_;
    bool tree_visible_;

public:
    DockButton(int x, int y, int w, int h, const char* label = nullptr);

    void draw() override;
    int handle(int event) override;

    // State management
    void set_active(bool active);
    void set_tree_visible(bool visible);
    bool is_tree_visible() const { return tree_visible_; }

    // Context menu callback
    static void show_dock_menu(int x, int y);
    static void pin_panel_cb(Fl_Widget* w, void* data);
    static void auto_hide_cb(Fl_Widget* w, void* data);
    static void reveal_active_cb(Fl_Widget* w, void* data);
};