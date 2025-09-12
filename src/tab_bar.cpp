#include "tab_bar.hpp"
#include "globals.hpp"
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

// TabButton implementation
TabButton::TabButton(int X, int Y, int W, int H, Tab* t)
    : Fl_Button(X, Y, W, H), tab(t) {
    box(FL_FLAT_BOX);
    calculate_close_button_bounds();
}

void TabButton::calculate_close_button_bounds() {
    close_size = 10;
    close_x = x() + 6;
    close_y = y() + (h() - close_size) / 2;
}

bool TabButton::is_point_in_close_button(int px, int py) {
    return px >= close_x && px <= close_x + close_size && 
           py >= close_y && py <= close_y + close_size;
}

void TabButton::draw() {
    // Update close button position in case tab was moved
    calculate_close_button_bounds();
    
    // Draw tab background
    Fl_Color bg_color, text_color;
    if (tab->is_active) {
        bg_color = current_theme == THEME_DARK ? fl_rgb_color(60, 60, 60) : fl_rgb_color(255, 255, 255);
        text_color = current_theme == THEME_DARK ? FL_WHITE : FL_BLACK;
    } else {
        bg_color = current_theme == THEME_DARK ? fl_rgb_color(40, 40, 40) : fl_rgb_color(240, 240, 240);
        text_color = current_theme == THEME_DARK ? fl_rgb_color(180, 180, 180) : fl_rgb_color(60, 60, 60);
    }
    
    fl_color(bg_color);
    fl_rectf(x(), y(), w(), h());
    
    // Draw border
    Fl_Color border_color = current_theme == THEME_DARK ? fl_rgb_color(70, 70, 70) : fl_rgb_color(200, 200, 200);
    fl_color(border_color);
    fl_rect(x(), y(), w(), h());
    
    // Draw close button (×)
    Fl_Color close_color = close_hovered ? 
        (current_theme == THEME_DARK ? fl_rgb_color(255, 100, 100) : fl_rgb_color(200, 50, 50)) :
        (current_theme == THEME_DARK ? fl_rgb_color(150, 150, 150) : fl_rgb_color(100, 100, 100));
    
    fl_color(close_color);
    fl_font(FL_HELVETICA, 10);
    fl_draw("×", close_x, close_y + 6);
    
    // Draw filename
    fl_color(text_color);
    fl_font(FL_HELVETICA, 12);
    
    std::string display_name = tab->filename;
    if (tab->is_modified) {
        display_name = "• " + display_name;
    }
    
    // Calculate text area (excluding close button)
    int text_x = close_x + close_size + 3;
    int text_w = x() + w() - text_x - 3;
    
    // Calculate text width and center it
    fl_font(FL_HELVETICA, 12);
    int text_width = (int)fl_width(display_name.c_str());
    int centered_x = text_x + (text_w - text_width) / 2;
    
    fl_push_clip(text_x, y(), text_w, h());
    fl_draw(display_name.c_str(), centered_x, y() + h()/2 + 3);
    fl_pop_clip();
}

int TabButton::handle(int e) {
    switch (e) {
    case FL_MOVE:
    case FL_ENTER:
    case FL_LEAVE: {
        bool was_hovered = close_hovered;
        close_hovered = is_point_in_close_button(Fl::event_x(), Fl::event_y());
        if (was_hovered != close_hovered) {
            redraw();
        }
        return 1;
    }
    case FL_PUSH:
        if (Fl::event_button() == FL_LEFT_MOUSE) {
            if (is_point_in_close_button(Fl::event_x(), Fl::event_y())) {
                if (on_close) on_close(tab);
                return 1;
            } else {
                if (on_select) on_select(tab);
                return 1;
            }
        }
        break;
    }
    return Fl_Button::handle(e);
}

// TabBar implementation
TabBar::TabBar(int X, int Y, int W, int H) : Fl_Group(X, Y, W, H) {
    box(FL_FLAT_BOX);
    end();
}

TabBar::~TabBar() {
    clear_tab_buttons();
    for (Tab* tab : tabs) {
        delete tab;
    }
}

void TabBar::add_tab(const std::string& filename, const std::string& filepath) {
    // Check if tab already exists
    if (find_tab_by_filepath(filepath)) {
        set_active_tab(filepath);
        return;
    }
    
    // Extract just filename from path
    std::filesystem::path path(filepath);
    std::string display_name = path.filename().string();
    if (display_name.empty()) {
        display_name = filename;
    }
    
    // Deactivate current active tab
    for (Tab* tab : tabs) {
        tab->is_active = false;
    }
    
    // Create new tab
    Tab* new_tab = new Tab(display_name, filepath, true);
    tabs.push_back(new_tab);
    
    // Load file content into the tab's buffer
    if (std::filesystem::exists(filepath)) {
        new_tab->buffer->loadfile(filepath.c_str());
    }
    
    relayout_tabs();
    
    if (on_tab_selected) {
        on_tab_selected(filepath);
    }
}

void TabBar::remove_tab(const std::string& filepath) {
    int index = find_tab_index_by_filepath(filepath);
    if (index == -1) return;
    
    Tab* tab_to_remove = tabs[index];
    bool was_active = tab_to_remove->is_active;
    
    delete tab_to_remove;
    tabs.erase(tabs.begin() + index);
    
    // If we removed the active tab, activate another one
    if (was_active && !tabs.empty()) {
        int new_active_index = std::min(index, (int)tabs.size() - 1);
        tabs[new_active_index]->is_active = true;
        
        if (on_tab_selected) {
            on_tab_selected(tabs[new_active_index]->filepath);
        }
    }
    
    relayout_tabs();
    redraw();
}

void TabBar::set_active_tab(const std::string& filepath) {
    for (Tab* tab : tabs) {
        tab->is_active = (tab->filepath == filepath);
    }
    relayout_tabs();
    redraw();
}

void TabBar::update_tab_modified(const std::string& filepath, bool modified) {
    Tab* tab = find_tab_by_filepath(filepath);
    if (tab) {
        tab->is_modified = modified;
        redraw();
    }
}

Tab* TabBar::get_active_tab() {
    for (Tab* tab : tabs) {
        if (tab->is_active) {
            return tab;
        }
    }
    return nullptr;
}

std::vector<Tab*> TabBar::get_all_tabs() {
    return tabs;
}

void TabBar::clear_tab_buttons() {
    for (TabButton* btn : tab_buttons) {
        remove(btn);
        delete btn;
    }
    tab_buttons.clear();
}

void TabBar::create_tab_buttons() {
    clear_tab_buttons();
    
    if (tabs.empty()) return;
    
    // Calculate individual tab widths based on content
    std::vector<int> tab_widths;
    int total_content_width = 0;
    fl_font(FL_HELVETICA, 12);
    
    for (Tab* tab : tabs) {
        std::string display_name = tab->filename;
        if (tab->is_modified) {
            display_name = "• " + display_name;
        }
        
        // Calculate text width + close button + padding + right margin (one character width)
        int text_width = (int)fl_width(display_name.c_str());
        int char_width = (int)fl_width("M"); // Use 'M' as reference for character width
        int tab_width = text_width + 10 + 6 + 6 + char_width; // text + close button + padding + right margin
        
        // Clamp to min/max bounds
        tab_width = std::max(TAB_MIN_WIDTH, tab_width);
        tab_width = std::min(TAB_MAX_WIDTH, tab_width);
        
        tab_widths.push_back(tab_width);
        total_content_width += tab_width;
    }
    
    // If content doesn't fit, distribute available width evenly
    if (total_content_width > w()) {
        int available_width = w();
        int tab_count = tabs.size();
        int uniform_width = available_width / tab_count;
        uniform_width = std::max(TAB_MIN_WIDTH, uniform_width);
        uniform_width = std::min(TAB_MAX_WIDTH, uniform_width);
        
        for (int i = 0; i < tab_widths.size(); ++i) {
            tab_widths[i] = uniform_width;
        }
    }
    
    int current_x = x();
    
    for (size_t i = 0; i < tabs.size(); ++i) {
        Tab* tab = tabs[i];
        int tab_width = tab_widths[i];
        TabButton* btn = new TabButton(current_x, y(), tab_width, h(), tab);
        
        // Set callbacks
        btn->on_close = [this](Tab* t) {
            if (on_tab_closed) {
                on_tab_closed(t->filepath);
            }
        };
        
        btn->on_select = [this](Tab* t) {
            set_active_tab(t->filepath);
            if (on_tab_selected) {
                on_tab_selected(t->filepath);
            }
        };
        
        add(btn);
        tab_buttons.push_back(btn);
        current_x += tab_width;
    }
}

int TabBar::calculate_tab_width() {
    if (tabs.empty()) return TAB_MIN_WIDTH;
    
    // Calculate total width needed for all tabs based on their content
    int total_content_width = 0;
    fl_font(FL_HELVETICA, 12);
    
    for (Tab* tab : tabs) {
        std::string display_name = tab->filename;
        if (tab->is_modified) {
            display_name = "• " + display_name;
        }
        
        // Calculate text width + close button + padding + right margin (one character width)
        int text_width = (int)fl_width(display_name.c_str());
        int char_width = (int)fl_width("M"); // Use 'M' as reference for character width
        int tab_width = text_width + 10 + 6 + 6 + char_width; // text + close button + padding + right margin
        
        // Clamp to min/max bounds
        tab_width = std::max(TAB_MIN_WIDTH, tab_width);
        tab_width = std::min(TAB_MAX_WIDTH, tab_width);
        
        total_content_width += tab_width;
    }
    
    // If total content fits in available width, use content-based width
    if (total_content_width <= w()) {
        return TAB_MIN_WIDTH; // Will be overridden in create_tab_buttons
    }
    
    // Otherwise, distribute available width evenly
    int available_width = w();
    int tab_count = tabs.size();
    int calculated_width = available_width / tab_count;
    
    // Clamp to min/max bounds
    calculated_width = std::max(TAB_MIN_WIDTH, calculated_width);
    calculated_width = std::min(TAB_MAX_WIDTH, calculated_width);
    
    return calculated_width;
}

void TabBar::relayout_tabs() {
    create_tab_buttons();
    redraw();
}

Tab* TabBar::find_tab_by_filepath(const std::string& filepath) {
    auto it = std::find_if(tabs.begin(), tabs.end(),
        [&filepath](const Tab* tab) { return tab->filepath == filepath; });
    return it != tabs.end() ? *it : nullptr;
}

int TabBar::find_tab_index_by_filepath(const std::string& filepath) {
    for (size_t i = 0; i < tabs.size(); ++i) {
        if (tabs[i]->filepath == filepath) {
            return i;
        }
    }
    return -1;
}

int TabBar::get_tab_index_at_position(int px) {
    if (tabs.empty()) return -1;
    
    int tab_width = calculate_tab_width();
    int relative_x = px - x();
    int index = relative_x / tab_width;
    
    return std::max(0, std::min((int)tabs.size(), index));
}

void TabBar::draw() {
    // Draw background
    Fl_Color bg_color = current_theme == THEME_DARK ? 
        fl_rgb_color(30, 30, 30) : fl_rgb_color(250, 250, 250);
    fl_color(bg_color);
    fl_rectf(x(), y(), w(), h());
    
    // Draw bottom border
    Fl_Color border_color = current_theme == THEME_DARK ? 
        fl_rgb_color(70, 70, 70) : fl_rgb_color(200, 200, 200);
    fl_color(border_color);
    fl_line(x(), y() + h() - 1, x() + w(), y() + h() - 1);
    
    // Draw drag indicator if dragging
    if (dragging && drag_insert_index >= 0) {
        int tab_width = calculate_tab_width();
        int indicator_x = x() + drag_insert_index * tab_width;
        
        fl_color(current_theme == THEME_DARK ? FL_CYAN : FL_BLUE);
        fl_line_style(FL_SOLID, 3);
        fl_line(indicator_x, y(), indicator_x, y() + h());
        fl_line_style(FL_SOLID, 1); // Reset line style
    }
    
    Fl_Group::draw();
}

int TabBar::handle(int e) {
    switch (e) {
    case FL_PUSH:
        if (Fl::event_button() == FL_LEFT_MOUSE) {
            dragging = false;
            drag_start_x = Fl::event_x();
            
            // Find which tab was clicked
            for (size_t i = 0; i < tab_buttons.size(); ++i) {
                TabButton* btn = tab_buttons[i];
                if (Fl::event_x() >= btn->x() && Fl::event_x() < btn->x() + btn->w() &&
                    Fl::event_y() >= btn->y() && Fl::event_y() < btn->y() + btn->h()) {
                    dragged_tab = btn->tab;
                    break;
                }
            }
        }
        break;
        
    case FL_DRAG:
        if (dragged_tab && abs(Fl::event_x() - drag_start_x) > 10) {
            if (!dragging) {
                dragging = true;
                Fl::belowmouse(this); // Capture mouse events
            }
            
            // Calculate where to insert the dragged tab
            drag_insert_index = get_tab_index_at_position(Fl::event_x());
            redraw();
        }
        break;
        
    case FL_RELEASE:
        if (dragging && dragged_tab && drag_insert_index >= 0) {
            // Find current index of dragged tab
            int current_index = find_tab_index_by_filepath(dragged_tab->filepath);
            
            if (current_index != -1 && current_index != drag_insert_index && 
                drag_insert_index <= (int)tabs.size()) {
                
                // Move tab in the vector
                Tab* tab = tabs[current_index];
                tabs.erase(tabs.begin() + current_index);
                
                // Adjust insert index if we removed an item before it
                int insert_idx = drag_insert_index;
                if (current_index < drag_insert_index) {
                    insert_idx--;
                }
                
                tabs.insert(tabs.begin() + insert_idx, tab);
                
                relayout_tabs();
                
                if (on_tab_moved) {
                    on_tab_moved(dragged_tab->filepath, dragged_tab->filepath);
                }
            }
        }
        
        dragging = false;
        dragged_tab = nullptr;
        drag_insert_index = -1;
        redraw();
        break;
    }
    
    int ret = Fl_Group::handle(e);
    return ret ? ret : 1; // Always consume events to enable dragging
}

void TabBar::save_tab_state() {
    const char* home = getenv("HOME");
    if (!home) return;
    
    std::string state_file = std::string(home) + "/.flick_tabs";
    std::ofstream file(state_file);
    
    if (file.is_open()) {
        // Save active tab first
        Tab* active_tab = get_active_tab();
        if (active_tab) {
            file << "ACTIVE:" << active_tab->filepath << std::endl;
        }
        
        // Save all tabs
        for (Tab* tab : tabs) {
            file << "TAB:" << tab->filepath << "|" << (tab->is_modified ? "1" : "0") << std::endl;
        }
        
        file.close();
    }
}

void TabBar::load_tab_state() {
    const char* home = getenv("HOME");
    if (!home) return;
    
    std::string state_file = std::string(home) + "/.flick_tabs";
    std::ifstream file(state_file);
    
    if (!file.is_open()) return;
    
    std::string line;
    std::string active_filepath;
    std::vector<std::string> tab_filepaths;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        if (line.substr(0, 7) == "ACTIVE:") {
            active_filepath = line.substr(7);
        } else if (line.substr(0, 4) == "TAB:") {
            size_t pipe_pos = line.find('|');
            
            if (pipe_pos != std::string::npos) {
                std::string filepath = line.substr(4, pipe_pos - 4);
                tab_filepaths.push_back(filepath);
            }
        }
    }
    
    file.close();
    
    // Load tabs
    for (const std::string& filepath : tab_filepaths) {
        if (std::filesystem::exists(filepath)) {
            add_tab("", filepath);
        }
    }
    
    // Set active tab
    if (!active_filepath.empty() && std::filesystem::exists(active_filepath)) {
        set_active_tab(active_filepath);
        if (on_tab_selected) {
            on_tab_selected(active_filepath);
        }
    }
}

Fl_Text_Buffer* TabBar::get_tab_buffer(const std::string& filepath) {
    Tab* tab = find_tab_by_filepath(filepath);
    return tab ? tab->buffer : nullptr;
}

void TabBar::switch_to_tab_buffer(const std::string& filepath) {
    Tab* tab = find_tab_by_filepath(filepath);
    if (tab && tab->buffer) {
        // This will be called from the editor to switch buffers
        // The actual buffer switching will be handled in the editor
    }
}