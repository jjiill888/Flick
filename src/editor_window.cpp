#include "globals.hpp"
#include "utils.hpp"
#include "file_tree.hpp"
#include "editor_window.hpp"
#include "scrollbar_theme.hpp"
#include "tab_bar.hpp"
#include "dock_button.hpp"
#include "custom_title_bar.hpp"
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <thread>
#include <future>

Fl_Double_Window *win = nullptr;
Fl_Menu_Bar    *menu = nullptr;
Fl_Tree        *file_tree = nullptr;
int font_size = 14;
Fl_Menu_Button *context_menu = nullptr;
My_Text_Editor  *editor = nullptr;
Fl_Text_Buffer  *buffer = new Fl_Text_Buffer();
Fl_Text_Buffer  *style_buffer = new Fl_Text_Buffer();
bool text_changed = false;
bool switching_tabs = false;
char current_file[FL_PATH_MAX] = "";
char current_folder[FL_PATH_MAX] = "";
Fl_Box          *status_left = nullptr;
Fl_Box          *status_right = nullptr;
Fl_Box          *tree_resizer = nullptr;
Fl_Menu_Button *tree_context_menu = nullptr;
TabBar          *tab_bar = nullptr;
DockButton      *dock_button = nullptr;
CustomTitleBar  *title_bar = nullptr;
time_t           last_save_time = 0;
int             tree_width = 200;
Theme           current_theme = THEME_DARK;

// Window position and size variables
int             window_x = 100;
int             window_y = 100;
int             window_w = 1301;
int             window_h = 887;

// Add lazy loading flags
static bool file_tree_loaded = false;
static std::future<void> file_tree_future;

static void legacy_tree_new_file_cb(Fl_Widget* w, void*) {
    new_file_cb(w, tree_context_menu->user_data());
}

static void legacy_tree_new_folder_cb(Fl_Widget* w, void*) {
    new_folder_cb(w, tree_context_menu->user_data());
}

static void legacy_tree_refresh_cb(Fl_Widget* w, void*) {
    refresh_subdir_cb(w, tree_context_menu->user_data());
}

static void legacy_tree_delete_cb(Fl_Widget* w, void*) {
    delete_cb(w, tree_context_menu->user_data());
}

EditorWindow::EditorWindow(int W,int H,const char* L)
    : Fl_Double_Window(W,H,L) {}

void EditorWindow::resize(int X,int Y,int W,int H) {
    Fl_Double_Window::resize(X,Y,W,H);
    if (title_bar && menu && editor && status_left && status_right) {
        const int title_h = title_bar->h();
        const int menu_h = menu->h();
        const int content_y = title_h + menu_h;  // Content below title bar and menu
        const int status_h = status_left->h();
        const int tab_h = tab_bar ? tab_bar->h() : 0;
        int tree_w = file_tree ? tree_width : 0;
        int resize_w = tree_resizer ? tree_resizer->w() : 0;

        // Update title bar size (full width)
        title_bar->resize(0, 0, W, title_h);

        // Update menu bar position and size
        menu->resize(0, title_h, W, menu_h);

        // Position tab bar only on the right side (not over file tree)
        if (tab_bar) {
            tab_bar->position(tree_w + resize_w, content_y);
            tab_bar->size(W - tree_w - resize_w, tab_h);
        }

        // Position editor below tab bar
        editor->position(tree_w + resize_w, content_y + tab_h);
        editor->size(W - tree_w - resize_w, H - content_y - tab_h - status_h);

        if (file_tree) {
            file_tree->position(0, content_y);
            file_tree->size(tree_w, H - content_y - status_h);
            if (tree_resizer) {
                tree_resizer->position(tree_w, content_y);
                tree_resizer->size(resize_w, H - content_y - status_h);
            }
        }

        // Update dock button position
        if (dock_button) {
            const int dock_btn_margin = 2;
            dock_button->position(dock_btn_margin, H - status_h);
        }

        // Update status bar layout to accommodate dock button
        const int dock_btn_w = 36;
        const int dock_btn_margin = 2;
        const int status_left_x = dock_btn_w + dock_btn_margin * 2;
        const int status_left_w = W/2 - status_left_x;

        status_left->position(status_left_x, H - status_h);
        status_left->size(status_left_w, status_h);
        status_right->position(W/2, H - status_h);
        status_right->size(W - W/2, status_h);
    }
}

int EditorWindow::handle(int e) {
    const int RESIZE_BORDER = 5;  // Border thickness for resize detection
    static int resize_mode = 0;   // 0=none, 1=left, 2=right, 3=top, 4=bottom, 5=topleft, 6=topright, 7=bottomleft, 8=bottomright
    static int start_x = 0, start_y = 0, start_w = 0, start_h = 0, start_wx = 0, start_wy = 0;

    int mx = Fl::event_x();
    int my = Fl::event_y();

    // Determine which edge/corner we're near
    auto get_resize_area = [&]() -> int {
        bool near_left = mx < RESIZE_BORDER;
        bool near_right = mx > w() - RESIZE_BORDER;
        bool near_top = my < RESIZE_BORDER;
        bool near_bottom = my > h() - RESIZE_BORDER;

        if (near_top && near_left) return 5;      // Top-left corner
        if (near_top && near_right) return 6;     // Top-right corner
        if (near_bottom && near_left) return 7;   // Bottom-left corner
        if (near_bottom && near_right) return 8;  // Bottom-right corner
        if (near_left) return 1;                  // Left edge
        if (near_right) return 2;                 // Right edge
        if (near_top) return 3;                   // Top edge
        if (near_bottom) return 4;                // Bottom edge
        return 0;                                 // No resize area
    };

    if (e == FL_MOVE && resize_mode == 0) {
        int area = get_resize_area();
        // Set cursor based on resize area
        switch (area) {
            case 1: case 2: fl_cursor(FL_CURSOR_WE); break;          // Left/Right
            case 3: case 4: fl_cursor(FL_CURSOR_NS); break;          // Top/Bottom
            case 5: case 8: fl_cursor(FL_CURSOR_NWSE); break;        // NW-SE diagonal
            case 6: case 7: fl_cursor(FL_CURSOR_NESW); break;        // NE-SW diagonal
            default: fl_cursor(FL_CURSOR_DEFAULT); break;
        }
        return 1;
    }

    if (e == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE) {
        resize_mode = get_resize_area();
        if (resize_mode > 0) {
            start_x = Fl::event_x_root();
            start_y = Fl::event_y_root();
            start_w = w();
            start_h = h();
            start_wx = x();
            start_wy = y();
            return 1;
        }
    }

    if (e == FL_DRAG && resize_mode > 0) {
        int dx = Fl::event_x_root() - start_x;
        int dy = Fl::event_y_root() - start_y;
        int new_x = start_wx, new_y = start_wy, new_w = start_w, new_h = start_h;

        const int MIN_W = 400, MIN_H = 300;

        switch (resize_mode) {
            case 1: // Left edge
                new_w = start_w - dx;
                if (new_w >= MIN_W) new_x = start_wx + dx;
                else new_w = MIN_W;
                break;
            case 2: // Right edge
                new_w = start_w + dx;
                if (new_w < MIN_W) new_w = MIN_W;
                break;
            case 3: // Top edge
                new_h = start_h - dy;
                if (new_h >= MIN_H) new_y = start_wy + dy;
                else new_h = MIN_H;
                break;
            case 4: // Bottom edge
                new_h = start_h + dy;
                if (new_h < MIN_H) new_h = MIN_H;
                break;
            case 5: // Top-left corner
                new_w = start_w - dx;
                new_h = start_h - dy;
                if (new_w >= MIN_W) new_x = start_wx + dx;
                else new_w = MIN_W;
                if (new_h >= MIN_H) new_y = start_wy + dy;
                else new_h = MIN_H;
                break;
            case 6: // Top-right corner
                new_w = start_w + dx;
                new_h = start_h - dy;
                if (new_w < MIN_W) new_w = MIN_W;
                if (new_h >= MIN_H) new_y = start_wy + dy;
                else new_h = MIN_H;
                break;
            case 7: // Bottom-left corner
                new_w = start_w - dx;
                new_h = start_h + dy;
                if (new_w >= MIN_W) new_x = start_wx + dx;
                else new_w = MIN_W;
                if (new_h < MIN_H) new_h = MIN_H;
                break;
            case 8: // Bottom-right corner
                new_w = start_w + dx;
                new_h = start_h + dy;
                if (new_w < MIN_W) new_w = MIN_W;
                if (new_h < MIN_H) new_h = MIN_H;
                break;
        }

        resize(new_x, new_y, new_w, new_h);
        return 1;
    }

    if (e == FL_RELEASE) {
        if (resize_mode > 0) {
            resize_mode = 0;
            fl_cursor(FL_CURSOR_DEFAULT);
            save_window_state();
            return 1;
        }
    }

    if (e == FL_LEAVE) {
        if (resize_mode == 0) {
            fl_cursor(FL_CURSOR_DEFAULT);
        }
    }

    int ret = Fl_Double_Window::handle(e);

    // Save window state when window is moved or resized
    if (e == FL_MOVE) {
        // Update global variables
        window_x = x();
        window_y = y();
        window_w = w();
        window_h = h();

        // Save to file periodically (not on every move/resize to avoid excessive I/O)
        static int save_counter = 0;
        if (++save_counter % 10 == 0) {  // Save every 10th event
            save_window_state();
        }
    }

    return ret;
}

TreeResizer::TreeResizer(int X,int Y,int W,int H) : Fl_Box(X,Y,W,H) {
    box(FL_FLAT_BOX);
    color(fl_rgb_color(80,80,80));
}

int TreeResizer::handle(int e) {
    switch(e){
    case FL_ENTER:
        fl_cursor(FL_CURSOR_WE);
        return 1;
    case FL_LEAVE:
        fl_cursor(FL_CURSOR_DEFAULT);
        return 1;
    case FL_PUSH:
    case FL_DRAG:
        fl_cursor(FL_CURSOR_WE);
        tree_width = Fl::event_x();
        if (tree_width < 100) tree_width = 100;
        if (tree_width > parent()->w() - 200) tree_width = parent()->w() - 200;
        parent()->resize(parent()->x(), parent()->y(), parent()->w(), parent()->h());
        return 1;
    case FL_RELEASE:
        fl_cursor(FL_CURSOR_DEFAULT);
        // Save the new tree width to persist across sessions
        save_window_state();
        return 1;
    }
    return Fl_Box::handle(e);
}

int My_Tree::handle(int e) {
    // Handle keyboard shortcuts
    if (e == FL_KEYDOWN) {
        if (tree_handle_key(Fl::event_key())) {
            return 1;
        }
    }

    // Handle right-click context menu
    if (e == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE) {
        Fl_Tree_Item* it = item_clicked();
        show_tree_context_menu(Fl::event_x(), Fl::event_y(), it);
        return 1;
    }

    return Fl_Tree::handle(e);
}

int My_Text_Editor::handle(int e) {
    if (e == FL_KEYDOWN) {
        if (Fl::event_key() == 's' && (Fl::event_state() & FL_CTRL)) {
            save_cb(nullptr, nullptr);
            return 1;
        }
        if (Fl::event_key() == 'w' && (Fl::event_state() & FL_CTRL)) {
            close_current_tab_cb(nullptr, nullptr);
            return 1;
        }
    }
    if (e == FL_MOUSEWHEEL && (Fl::event_state() & FL_CTRL)) {
        int dy = Fl::event_dy();
        if (dy != 0) {
            int size = textsize();
            if (dy < 0)
                ++size;
            else if (size > 4)
                --size;
            set_font_size(size);
            return 1;
        }
    }
    if (e == FL_PUSH && Fl::event_button() == FL_RIGHT_MOUSE && context_menu) {
        context_menu->position(Fl::event_x(), Fl::event_y());
        context_menu->popup();
        return 1;
    }
    int ret = Fl_Text_Editor::handle(e);
    if (e == FL_KEYDOWN || e == FL_KEYUP || e == FL_MOVE ||
        e == FL_PUSH || e == FL_DRAG || e == FL_RELEASE)
        update_status();
    return ret;
}

int run_editor(int argc,char** argv){
    // Quick initialization of basic UI
    Fl::get_system_colors();
    // Set all fonts to JetBrains Mono variants
    Fl::set_font(FL_COURIER, "JetBrains Mono");
    Fl::set_font(FL_HELVETICA, "JetBrains Mono");
    Fl::set_font(FL_TIMES, "JetBrains Mono");
    Fl::set_font(FL_SYMBOL, "JetBrains Mono");
    Fl::set_color(FL_SELECTION_COLOR, fl_rgb_color(75,110,175));
    Fl::scheme("gtk+");
    apply_scrollbar_style();

    // Load window state before creating window
    load_window_state();

    win = new EditorWindow(window_w, window_h, "Flick");
    win->position(window_x, window_y);
    win->callback(quit_cb);
    win->border(0);  // Remove system title bar and borders

    // Create custom title bar
    const int title_h = CustomTitleBar::TITLE_BAR_HEIGHT;
    title_bar = new CustomTitleBar(0, 0, win->w(), title_h, "Flick");

    // Set up title bar window control callbacks
    title_bar->on_close_callback([](Fl_Widget*, void*) {
        quit_cb(nullptr, nullptr);
    }, nullptr);

    title_bar->on_minimize_callback([](Fl_Widget*, void*) {
        if (win) win->iconize();
    }, nullptr);

    // Create menu bar below title bar
    const int menu_h = 25;
    menu = new Fl_Menu_Bar(0, title_h, win->w(), menu_h);
    menu->add("&File/New",  FL_CTRL + 'n', new_cb);
    menu->add("&File/Open", FL_CTRL + 'o', open_cb);
    menu->add("&File/Open Folder", 0, open_folder_cb);
    menu->add("&File/Save", FL_CTRL + 's', save_cb);
    menu->add("&File/Quit", FL_CTRL + 'q', quit_cb);
    menu->add("&View/Dark Theme", 0, theme_dark_cb);
    menu->add("&View/Light Theme", 0, theme_light_cb);
    menu->add("&Find/Find...", FL_CTRL + 'f', find_cb);
    menu->add("&Find/Replace...", FL_CTRL + 'h', replace_cb);
    menu->add("&Find/Global Search...", FL_CTRL | FL_SHIFT | 'f', global_search_cb);

    const int status_h = 20;
    const int content_y = title_h + menu_h;  // Content starts below title bar and menu
    font_size = load_font_size();

    // Create file tree but don't load content immediately
    file_tree = new My_Tree(0, content_y, tree_width, win->h() - content_y - status_h);
    file_tree->callback(tree_cb);
    file_tree->showroot(false);
    file_tree->root_label("Loading...");

    tree_context_menu = new Fl_Menu_Button(0,0,0,0);
    tree_context_menu->hide();
    tree_context_menu->add("New File", 0, legacy_tree_new_file_cb);
    tree_context_menu->add("New Folder", 0, legacy_tree_new_folder_cb);
    tree_context_menu->add("Refresh", 0, legacy_tree_refresh_cb);
    tree_context_menu->add("Delete", 0, legacy_tree_delete_cb);
    tree_resizer = new TreeResizer(tree_width, content_y, 4, win->h() - content_y - status_h);

    // Create tab bar on right side only (not over file tree)
    const int tab_h = 22;
    tab_bar = new TabBar(tree_width + tree_resizer->w(), content_y,
                         win->w() - tree_width - tree_resizer->w(), tab_h);
    
    // Set up tab bar callbacks
    tab_bar->on_tab_selected = [](const std::string& filepath) {
        // Set flag to prevent changed_cb from marking tabs as modified during switching
        switching_tabs = true;
        
        // First, save current content and modified state to the current tab's buffer
        std::string current_filepath = std::string(current_file);
        if (!current_filepath.empty() && current_filepath != filepath) {
            Fl_Text_Buffer* current_tab_buffer = tab_bar->get_tab_buffer(current_filepath);
            if (current_tab_buffer) {
                // Save the current editor content to the tab's buffer
                current_tab_buffer->text(buffer->text());
                // Save the current modified state to the tab
                tab_bar->update_tab_modified(current_filepath, text_changed);
            }
        }
        
        // Switch to the new tab's buffer
        Fl_Text_Buffer* tab_buffer = tab_bar->get_tab_buffer(filepath);
        if (tab_buffer) {
            buffer->text(tab_buffer->text());
            strcpy(current_file, filepath.c_str());
            
            // Restore the modified state from the tab
            std::vector<Tab*> all_tabs = tab_bar->get_all_tabs();
            for (Tab* tab : all_tabs) {
                if (tab->filepath == filepath) {
                    text_changed = tab->is_modified;
                    break;
                }
            }
            
            update_title();
            update_status();
        }
        
        // Clear the switching flag
        switching_tabs = false;
    };
    
    tab_bar->on_tab_closed = [](const std::string& filepath) {
        // Check if this is the current file and has unsaved changes
        if (std::string(current_file) == filepath && text_changed) {
            int r = fl_choice("Save changes before closing?", "Cancel", "Save", "Don't Save");
            if (r == 0) return; // Cancel - don't close the tab
            if (r == 1) {
                // Save the file
                save_cb(nullptr, nullptr);
                // Check if save was successful
                if (text_changed) return; // Save failed or was cancelled
            }
        }
        
        // Remove the tab from the tab bar
        tab_bar->remove_tab(filepath);
        
        // If this was the current file, clear the editor
        if (std::string(current_file) == filepath) {
            buffer->text("");
            current_file[0] = '\0';
            text_changed = false;
            update_title();
            update_status();
        }
    };
    
    // Load saved tab state
    tab_bar->load_tab_state();

    editor = new My_Text_Editor(tree_width + tree_resizer->w(), content_y + tab_h,
                                win->w() - tree_width - tree_resizer->w(),
                                win->h() - content_y - tab_h - status_h);
    editor->buffer(buffer);
    editor->textfont(FL_COURIER);
    set_font_size(font_size);
    editor->linenumber_width(30);
    editor->linenumber_align(FL_ALIGN_RIGHT);
    editor->scrollbar_width(Fl::scrollbar_size());
    editor->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    context_menu = new Fl_Menu_Button(0,0,0,0);
    context_menu->hide();
    context_menu->add("Cut",0,cut_cb);
    context_menu->add("Copy",0,copy_cb);
    context_menu->add("Paste",0,paste_cb);
    context_menu->add("Select All",0,select_all_cb);
    // Create dock button first - minimal size and positioning
    const int dock_btn_w = 36;  // Smaller, more subtle width
    const int dock_btn_margin = 2;  // Minimal margin
    dock_button = new DockButton(dock_btn_margin, win->h() - status_h,
                                 dock_btn_w, status_h, "Tree");  // Full status bar height
    dock_button->set_tree_visible(tree_width > 0);

    // Adjust status bar layout to accommodate dock button
    const int status_left_x = dock_btn_w + dock_btn_margin * 2;
    const int status_left_w = win->w()/2 - status_left_x;

    status_left = new Fl_Box(status_left_x, win->h() - status_h, status_left_w, status_h);
    status_left->box(FL_FLAT_BOX);
    status_left->labelsize(13);
    status_left->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_left->label("");

    status_right = new Fl_Box(win->w()/2, win->h() - status_h, win->w() - win->w()/2, status_h);
    status_right->box(FL_FLAT_BOX);
    status_right->labelsize(13);
    status_right->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    status_right->label("");
    apply_theme(current_theme);
    buffer->add_modify_callback(changed_cb, nullptr);
    style_init();
    update_status();
    
    // Asynchronously load file tree
    file_tree_future = std::async(std::launch::async, []() {
        load_last_folder_if_any();
        file_tree_loaded = true;
    });
    
    editor->highlight_data(style_buffer, style_table,
                           style_table_size,
                           'A', nullptr, nullptr);

    win->resizable(editor);
    win->end();
    win->show(argc, argv);

    // Quick file loading (if command line arguments provided)
    if (argc > 1) {
        load_file(argv[1]);
    } else {
        // Delay loading last file to avoid blocking UI
        Fl::add_timeout(0.1, [](void*) {
            load_last_file_if_any();
            if (!current_file[0]) update_title();
            update_status();
        });
    }

    // Add callback for file tree loading completion
    Fl::add_timeout(0.05, [](void*) {
        if (file_tree_loaded && file_tree_future.valid()) {
            file_tree_future.wait();
            if (file_tree) {
                file_tree->redraw();
            }
        } else {
            Fl::repeat_timeout(0.05, nullptr);
        }
    });

    return Fl::run();
}