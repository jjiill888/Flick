#include "globals.hpp"
#include "utils.hpp"
#include "file_tree.hpp"
#include "editor_window.hpp"
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Button.H>

Fl_Double_Window *win = nullptr;
Fl_Menu_Bar    *menu = nullptr;
Fl_Tree        *file_tree = nullptr;
int font_size = 14;
Fl_Menu_Button *context_menu = nullptr;
My_Text_Editor  *editor = nullptr;
Fl_Text_Buffer  *buffer = new Fl_Text_Buffer();
Fl_Text_Buffer  *style_buffer = new Fl_Text_Buffer();
bool text_changed = false;
char current_file[FL_PATH_MAX] = "";
char current_folder[FL_PATH_MAX] = "";
Fl_Box          *status_left = nullptr;
Fl_Box          *status_right = nullptr;
Fl_Box          *tree_resizer = nullptr;
Fl_Button       *item_refresh_btn = nullptr;
time_t           last_save_time = 0;
int             tree_width = 200;
Theme           current_theme = THEME_DARK;

EditorWindow::EditorWindow(int W,int H,const char* L)
    : Fl_Double_Window(W,H,L) {}

void EditorWindow::resize(int X,int Y,int W,int H) {
    Fl_Double_Window::resize(X,Y,W,H);
    if (menu && editor && status_left && status_right) {
        const int status_h = status_left->h();
        int tree_w = file_tree ? tree_width : 0;
        int resize_w = tree_resizer ? tree_resizer->w() : 0;
        editor->position(tree_w + resize_w, menu->h());
        editor->size(W - tree_w - resize_w, H - menu->h() - status_h);
        if (file_tree) {
            file_tree->position(0, menu->h());
            file_tree->size(tree_w, H - menu->h() - status_h);
            if (tree_resizer) {
                tree_resizer->position(tree_w, menu->h());
                tree_resizer->size(resize_w, H - menu->h() - status_h);
            }
        }
        menu->size(W, menu->h());
        status_left->position(0, H - status_h);
        status_left->size(W/2, status_h);
        status_right->position(W/2, H - status_h);
        status_right->size(W - W/2, status_h);
        if (item_refresh_btn && file_tree) {
            int bx = file_tree->x() + file_tree->w() - item_refresh_btn->w() - 2;
            item_refresh_btn->position(bx, item_refresh_btn->y());
            item_refresh_btn->show();
        }
    }
}

TreeResizer::TreeResizer(int X,int Y,int W,int H) : Fl_Box(X,Y,W,H) {
    box(FL_FLAT_BOX);
    color(fl_rgb_color(80,80,80));
}

int TreeResizer::handle(int e) {
    switch(e){
    case FL_PUSH:
    case FL_DRAG:
        tree_width = Fl::event_x();
        if (tree_width < 100) tree_width = 100;
        if (tree_width > parent()->w() - 100) tree_width = parent()->w() - 100;
        parent()->redraw();
        return 1;
    }
    return Fl_Box::handle(e);
}

int My_Text_Editor::handle(int e) {
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
    Fl::get_system_colors();
    Fl::set_font(FL_COURIER, "Monospace");
    Fl::set_color(FL_SELECTION_COLOR, fl_rgb_color(75,110,175));
    Fl::scheme("gtk+");
    Fl::scrollbar_size(16);

    win = new EditorWindow(1301,887,"Let‘s code");
    win->callback(quit_cb);
    menu = new Fl_Menu_Bar(0,0,win->w(),25);
    menu->add("&File/New",  FL_COMMAND + 'n', new_cb);
    menu->add("&File/Open", FL_COMMAND + 'o', open_cb);
    menu->add("&File/Open Folder", 0, open_folder_cb);
    menu->add("&File/Save", FL_COMMAND + 's', save_cb);
    menu->add("&File/Quit", FL_COMMAND + 'q', quit_cb);
    menu->add("&View/Dark Theme", 0, theme_dark_cb);
    menu->add("&View/Light Theme", 0, theme_light_cb);
    menu->add("&Find/Find...", FL_COMMAND + 'f', find_cb);
    menu->add("&Find/Replace...", FL_COMMAND + 'h', replace_cb);

    const int status_h = 20;
    font_size = load_font_size();
    file_tree = new Fl_Tree(0,25,tree_width,win->h()-25-status_h);
    file_tree->callback(tree_cb);
    file_tree->showroot(false);
    item_refresh_btn = new Fl_Button(0,0,18,18, "\u21bb");
    item_refresh_btn->tooltip("Refresh This Folder");
    int bx = file_tree->x() + file_tree->w() - item_refresh_btn->w() - 2;
    item_refresh_btn->position(bx, file_tree->y() + 2);
    item_refresh_btn->show();
    tree_resizer = new TreeResizer(tree_width,25,4,win->h()-25-status_h);
    editor = new My_Text_Editor(tree_width + tree_resizer->w(), 25,
                                win->w() - tree_width - tree_resizer->w(),
                                win->h() - 25 - status_h);
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
    status_left = new Fl_Box(0, win->h() - status_h, win->w()/2, status_h);
    status_left->box(FL_FLAT_BOX);
    status_left->labelsize(12);
    status_left->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_left->label("");
    status_right = new Fl_Box(win->w()/2, win->h() - status_h, win->w() - win->w()/2, status_h);
    status_right->box(FL_FLAT_BOX);
    status_right->labelsize(12);
    status_right->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    status_right->label("");
    apply_theme(current_theme);
    buffer->add_modify_callback(changed_cb, nullptr);
    style_init();
    update_status();
    load_last_folder_if_any();
    editor->highlight_data(style_buffer, style_table,
                           style_table_size,
                           'A', nullptr, nullptr);

    win->resizable(editor);
    win->end();
    win->show(argc, argv);

    if (argc > 1) {
        load_file(argv[1]);
    } else {
        load_last_file_if_any();
        if (!current_file[0]) update_title();
        update_status();
    }

    return Fl::run();
}