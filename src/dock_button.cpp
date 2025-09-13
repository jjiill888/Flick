#include "dock_button.hpp"
#include "globals.hpp"
#include <FL/fl_draw.H>
#include <FL/Fl_Menu.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>

// Global state for dock button
static bool tree_panel_pinned = false;
static bool auto_hide_narrow = true;

DockButton::DockButton(int x, int y, int w, int h, const char* label)
    : Fl_Button(x, y, w, h, label)
    , is_hovered_(false)
    , is_pressed_(false)
    , is_active_(false)
    , tree_visible_(true)
{
    box(FL_NO_BOX);
    clear_visible_focus();
    when(FL_WHEN_RELEASE);
    tooltip("Tree Panel (Right-click for options)");
}

void DockButton::draw() {
    // Get status bar background color to blend seamlessly
    Fl_Color status_bg_color, text_color, hover_color;

    if (current_theme == THEME_DARK) {
        // Match the status bar's dark background
        status_bg_color = fl_rgb_color(37, 37, 38);  // VSCode status bar color
        text_color = fl_rgb_color(204, 204, 204);    // Muted text color
        hover_color = fl_rgb_color(45, 45, 46);      // Very subtle hover
    } else {
        // Match the status bar's light background
        status_bg_color = fl_rgb_color(240, 240, 240);
        text_color = fl_rgb_color(80, 80, 80);       // Muted dark text
        hover_color = fl_rgb_color(235, 235, 235);   // Very subtle hover
    }

    // Start with status bar background (completely flat)
    fl_color(status_bg_color);
    fl_rectf(x(), y(), w(), h());

    // Only add subtle visual feedback on hover/press - no borders or boxes
    if (is_hovered_ || is_pressed_) {
        fl_color(is_pressed_ ? fl_darker(hover_color) : hover_color);
        fl_rectf(x(), y(), w(), h());
    }

    // Draw text with subtle color variation based on tree visibility
    if (tree_visible_) {
        // When active, text is slightly brighter but still muted
        fl_color(current_theme == THEME_DARK ? fl_rgb_color(220, 220, 220) : fl_rgb_color(60, 60, 60));
    } else {
        // When inactive, text is more muted
        fl_color(current_theme == THEME_DARK ? fl_rgb_color(160, 160, 160) : fl_rgb_color(120, 120, 120));
    }

    // Use smaller, lighter font to be less prominent
    fl_font(FL_HELVETICA, 10);

    const char* text = "Tree";
    int text_w = 0, text_h = 0;
    fl_measure(text, text_w, text_h);

    // Center text with minimal padding
    int text_x = x() + (w() - text_w) / 2;
    int text_y = y() + (h() + text_h) / 2 - fl_descent();

    fl_draw(text, text_x, text_y);
}

int DockButton::handle(int event) {
    int ret = 0;

    switch (event) {
        case FL_ENTER:
            is_hovered_ = true;
            redraw();
            ret = 1;
            break;

        case FL_LEAVE:
            is_hovered_ = false;
            redraw();
            ret = 1;
            break;

        case FL_PUSH:
            if (Fl::event_button() == FL_RIGHT_MOUSE) {
                show_dock_menu(Fl::event_x_root(), Fl::event_y_root());
                ret = 1;
            } else if (Fl::event_button() == FL_LEFT_MOUSE) {
                is_pressed_ = true;
                redraw();
                ret = 1;
            }
            break;

        case FL_RELEASE:
            if (Fl::event_button() == FL_LEFT_MOUSE && is_pressed_) {
                is_pressed_ = false;

                // Toggle tree visibility
                tree_visible_ = !tree_visible_;
                toggle_file_tree();
                redraw();
                ret = 1;
            }
            break;

        default:
            ret = Fl_Button::handle(event);
            break;
    }

    return ret;
}

void DockButton::set_active(bool active) {
    is_active_ = active;
    redraw();
}

void DockButton::set_tree_visible(bool visible) {
    tree_visible_ = visible;
    redraw();
}

void DockButton::show_dock_menu(int x, int y) {
    static Fl_Menu_Item dock_menu[] = {
        {"Pin Panel", 0, pin_panel_cb, 0, tree_panel_pinned ? FL_MENU_TOGGLE | FL_MENU_VALUE : FL_MENU_TOGGLE},
        {"Auto-Hide on Narrow Width", 0, auto_hide_cb, 0, auto_hide_narrow ? FL_MENU_TOGGLE | FL_MENU_VALUE : FL_MENU_TOGGLE},
        {"", 0, 0, 0, FL_MENU_DIVIDER},
        {"Reveal Active File", 0, reveal_active_cb, 0, 0},
        {0}
    };

    // Update menu state
    dock_menu[0].flags = tree_panel_pinned ? FL_MENU_TOGGLE | FL_MENU_VALUE : FL_MENU_TOGGLE;
    dock_menu[1].flags = auto_hide_narrow ? FL_MENU_TOGGLE | FL_MENU_VALUE : FL_MENU_TOGGLE;

    const Fl_Menu_Item* selected = dock_menu->popup(x, y);
    if (selected && selected->callback_) {
        selected->callback_(nullptr, selected->user_data_);
    }
}

void DockButton::pin_panel_cb(Fl_Widget* w, void* data) {
    tree_panel_pinned = !tree_panel_pinned;
    // TODO: Implement panel pinning logic
}

void DockButton::auto_hide_cb(Fl_Widget* w, void* data) {
    auto_hide_narrow = !auto_hide_narrow;
    // TODO: Implement auto-hide logic
}

void DockButton::reveal_active_cb(Fl_Widget* w, void* data) {
    // TODO: Implement reveal active file in tree
    if (file_tree && current_file[0]) {
        // Logic to expand tree and highlight current file
        fl_message("Reveal active file: %s", current_file);
    }
}

// Function to toggle file tree visibility
void toggle_file_tree() {
    if (!file_tree || !tree_resizer) return;

    static int saved_tree_width = tree_width;

    if (tree_width > 0) {
        // Hide tree
        saved_tree_width = tree_width;
        tree_width = 0;
        file_tree->hide();
        tree_resizer->hide();
    } else {
        // Show tree
        tree_width = saved_tree_width > 0 ? saved_tree_width : 200;
        file_tree->show();
        tree_resizer->show();
    }

    // Update dock button state
    if (dock_button) {
        dock_button->set_tree_visible(tree_width > 0);
    }

    // Trigger window resize to recalculate layout
    if (win) {
        win->redraw();
        // Force layout recalculation
        win->resize(win->x(), win->y(), win->w(), win->h());
    }
}