#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Tree.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <FL/Fl_Box.H>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>

static Fl_Double_Window *win;
static Fl_Menu_Bar    *menu;
static Fl_Tree        *file_tree = nullptr;
static int font_size = 14;
static void set_font_size(int sz);
static void update_status();
static void update_title();
class My_Text_Editor : public Fl_Text_Editor {
public:
    using Fl_Text_Editor::Fl_Text_Editor;
    int handle(int e) override {
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
        int ret = Fl_Text_Editor::handle(e);
        if (e == FL_KEYDOWN || e == FL_KEYUP || e == FL_MOVE ||
            e == FL_PUSH || e == FL_DRAG || e == FL_RELEASE)
            update_status();
        return ret;
    }
};

static My_Text_Editor  *editor;
static Fl_Text_Buffer  *buffer = new Fl_Text_Buffer();
static Fl_Text_Buffer  *style_buffer = new Fl_Text_Buffer();
static bool text_changed = false;
static char current_file[FL_PATH_MAX] = "";
static char current_folder[FL_PATH_MAX] = "";
static Fl_Box          *status_left = nullptr;
static Fl_Box          *status_right = nullptr;
static Fl_Box          *tree_resizer = nullptr;
static time_t           last_save_time = 0;
static int             tree_width = 200;

enum Theme { THEME_DARK, THEME_LIGHT };
static Theme current_theme = THEME_DARK;
static void apply_theme(Theme theme);
static void theme_light_cb(Fl_Widget*, void*);
static void theme_dark_cb(Fl_Widget*, void*);

class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int W, int H, const char* L = 0)
        : Fl_Double_Window(W, H, L) {}

    void resize(int X, int Y, int W, int H) override {
        Fl_Double_Window::resize(X, Y, W, H);
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
        }
    }
};

class TreeResizer : public Fl_Box {
public:
    TreeResizer(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H) {
        box(FL_FLAT_BOX);
        color(fl_rgb_color(80,80,80));
    }
    int handle(int e) override {
        switch (e) {
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
};

// Style table used for syntax highlighting
static Fl_Text_Display::Style_Table_Entry style_table[] = {
    { fl_rgb_color(212,212,212), FL_COURIER,        14 }, // A - plain text
    { fl_rgb_color(106,153,85),  FL_COURIER_ITALIC, 14 }, // B - line comment
    { fl_rgb_color(106,153,85),  FL_COURIER_ITALIC, 14 }, // C - block comment
    { fl_rgb_color(206,145,120), FL_COURIER,        14 }, // D - string literal
    { fl_rgb_color( 86,156,214), FL_COURIER_BOLD,   14 }, // E - preprocessor
    { fl_rgb_color(197,134,192), FL_COURIER_BOLD,   14 }  // F - keyword
};

static const char *keywords[] = {
    "auto", "bool", "break", "case", "char", "class", "const", "continue",
    "default", "delete", "do", "double", "else", "enum", "extern", "float",
    "for", "goto", "if", "inline", "int", "long", "namespace", "new", "operator",
    "private", "protected", "public", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "template", "typedef", "typename", "union",
    "unsigned", "virtual", "void", "volatile", "while", NULL
};

static bool is_keyword(const char *s) {
    for (int i = 0; keywords[i]; ++i)
        if (strcmp(keywords[i], s) == 0) return true;
    return false;
}

static void style_parse(const char *text, char *style, int length) {
    char current = 'A';
    int col = 0;
    for (int i = 0; i < length; ++i) {
        char c = text[i];
        if (current == 'B') { // line comment
            style[i] = 'B';
            if (c == '\n') { current = 'A'; col = 0; }
            continue;
        }
        if (current == 'C') { // block comment
            style[i] = 'C';
            if (c == '*' && i + 1 < length && text[i+1] == '/') {
                style[++i] = 'C';
                current = 'A';
            }
            if (c == '\n') col = 0; else col++;
            continue;
        }
        if (current == 'D') { // string
            style[i] = 'D';
            if (c == '\\') {
                if (i + 1 < length) style[++i] = 'D';
                continue;
            } else if (c == '"') {
                current = 'A';
            }
            if (c == '\n') col = 0; else col++;
            continue;
        }

        if (c == '/' && i + 1 < length && text[i+1] == '/') {
            style[i] = style[i+1] = 'B';
            current = 'B';
            ++i;
            continue;
        } else if (c == '/' && i + 1 < length && text[i+1] == '*') {
            style[i] = style[i+1] = 'C';
            current = 'C';
            ++i;
            continue;
        } else if (c == '"') {
            style[i] = 'D';
            current = 'D';
            continue;
        } else if (col == 0 && c == '#') {
            style[i] = 'E';
            current = 'B';
            continue;
        } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            char buf[64];
            int j = 0;
            buf[j++] = c;
            while (i + j < length &&
                   (std::isalnum(static_cast<unsigned char>(text[i+j])) || text[i+j]=='_') &&
                   j < (int)sizeof(buf)-1)
                buf[j++] = text[i+j];
            buf[j] = '\0';
            if (is_keyword(buf)) {
                for (int k = 0; k < j; ++k) style[i+k] = 'F';
                i += j - 1;
                col += j;
                continue;
            }
        }

        style[i] = 'A';
        if (c == '\n') col = 0; else col++;
    }
    style[length] = '\0';
}

static void style_init() {
    char *text = buffer->text();
    int length = buffer->length();
    char *style = new char[length + 1];
    memset(style, 'A', length);
    style_parse(text, style, length);
    style_buffer->text(style);
    delete[] style;
    free(text);
}

static const char* font_size_path() {
    static char path[FL_PATH_MAX];
    const char* home = getenv("HOME");
    if (home) snprintf(path, sizeof(path), "%s/.lets_code_fontsize", home);
    else strncpy(path, ".lets_code_fontsize", sizeof(path));
    return path;
}

static void save_font_size(int sz) {
    FILE* fp = fopen(font_size_path(), "w");
    if (fp) {
        fprintf(fp, "%d", sz);
        fclose(fp);
    }
}

static int load_font_size() {
    int sz = font_size;
    FILE* fp = fopen(font_size_path(), "r");
    if (fp) {
        if (fscanf(fp, "%d", &sz) != 1) sz = font_size;
        fclose(fp);
    }
    return sz;
}

static void set_font_size(int sz) {
    font_size = sz;
    if (editor) {
        editor->textsize(sz);
    }
    for (unsigned i = 0; i < sizeof(style_table)/sizeof(style_table[0]); ++i) {
        style_table[i].size = sz;
    }
    save_font_size(sz);
    if (editor) editor->damage(FL_DAMAGE_ALL);
}

static const char* last_file_path() {
    static char path[FL_PATH_MAX];
    const char* home = getenv("HOME");
    if (home) snprintf(path, sizeof(path), "%s/.lets_code_last", home);
    else strncpy(path, ".lets_code_last", sizeof(path));
    return path;
}

static void save_last_file() {
    FILE* fp = fopen(last_file_path(), "w");
    if (fp) {
        fputs(current_file, fp);
        fclose(fp);
    }
}

static const char* last_folder_path() {
    static char path[FL_PATH_MAX];
    const char* home = getenv("HOME");
    if (home) snprintf(path, sizeof(path), "%s/.lets_code_last_folder", home);
    else strncpy(path, ".lets_code_last_folder", sizeof(path));
    return path;
}

static void save_last_folder() {
    FILE* fp = fopen(last_folder_path(), "w");
    if (fp) {
        fputs(current_folder, fp);
        fclose(fp);
    }
}

static void load_last_file_if_any() {
    FILE* fp = fopen(last_file_path(), "r");
    if (fp) {
        if (fgets(current_file, sizeof(current_file), fp)) {
            size_t len = strlen(current_file);
            if (len && current_file[len-1] == '\n') current_file[len-1] = '\0';
        }
        fclose(fp);
        if (current_file[0]) {
            buffer->loadfile(current_file);
            text_changed = false;
            update_title();
            style_init();
            last_save_time = 0;
            update_status();
        }
    }
}


static void update_title() {
    const char *name = current_file[0] ? fl_filename_name(current_file)
                                       : "Untitled";
    char title[FL_PATH_MAX + 20];
    snprintf(title, sizeof(title), "%s%s - Let‘s code",
             name, text_changed ? "*" : "");
    win->copy_label(title);
}

static void update_status() {
    if (!status_left || !status_right || !editor) return;
    int pos = editor->insert_position();
    int line = buffer->count_lines(0, pos) + 1;
    int col = pos - buffer->line_start(pos) + 1;
    char left[64];
    snprintf(left, sizeof(left), "Ln %d, Col %d", line, col);
    status_left->copy_label(left);

    char timebuf[64] = "Never";
    if (last_save_time) {
        std::tm *tm = std::localtime(&last_save_time);
        if (tm) strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm);
    }
    char right[128];
    snprintf(right, sizeof(right), "%s | Last: %s", text_changed ? "Modified" : "Saved", timebuf);
    status_right->copy_label(right);
    status_left->redraw();
    status_right->redraw();
}

static void changed_cb(int, int, int, int, const char*, void*) {
    text_changed = true;
    update_title();
    style_init();
    update_status();
}

static void new_cb(Fl_Widget*, void*) {
    if (text_changed) {
        int r = fl_choice("Discard changes?", "Cancel", "Discard", NULL);
        if (r == 0) return;
    }
    buffer->text("");
    current_file[0] = '\0';
    text_changed = false;
    update_title();
    style_init();
    last_save_time = 0;
    update_status();
}

static void load_file(const char *file) {
    if (buffer->loadfile(file) == 0) {
        strncpy(current_file, file, sizeof(current_file));
        text_changed = false;
        update_title();
        save_last_file();
        style_init();
        last_save_time = 0;
        update_status();
    } else {
        fl_alert("Cannot open '%s'", file);
    }
}

static void open_cb(Fl_Widget*, void*) {
    Fl_Native_File_Chooser fc;
    fc.title("Open File...");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (fc.show() == 0) load_file(fc.filename());
}

static void add_folder_items(const char* abs, const char* rel) {
    DIR* d = opendir(abs);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char abspath[FL_PATH_MAX];
        snprintf(abspath, sizeof(abspath), "%s/%s", abs, e->d_name);
        char relpath[FL_PATH_MAX];
        if (rel && *rel) snprintf(relpath, sizeof(relpath), "%s/%s", rel, e->d_name);
        else snprintf(relpath, sizeof(relpath), "%s", e->d_name);
        struct stat st;
        if (stat(abspath, &st) == 0) {
            file_tree->add(relpath);
            if (S_ISDIR(st.st_mode)) add_folder_items(abspath, relpath);
        }
    }
    closedir(d);
}

static void collapse_first_level() {
    Fl_Tree_Item* root = file_tree->root();
    if (!root) return;
    for (int i = 0; i < root->children(); ++i) {
        Fl_Tree_Item* child = root->child(i);
        if (child->has_children()) child->close();
    }
}

static void load_folder(const char* folder) {
    strncpy(current_folder, folder, sizeof(current_folder));
    file_tree->clear();
    file_tree->root_label("");
    file_tree->showroot(false);
    add_folder_items(folder, "");
    collapse_first_level();
    save_last_folder();
}

static void load_last_folder_if_any() {
    FILE* fp = fopen(last_folder_path(), "r");
    if (fp) {
        if (fgets(current_folder, sizeof(current_folder), fp)) {
            size_t len = strlen(current_folder);
            if (len && current_folder[len-1] == '\n') current_folder[len-1] = '\0';
        }
        fclose(fp);
        if (current_folder[0]) load_folder(current_folder);
    }
}

static void tree_cb(Fl_Widget* w, void*) {
    Fl_Tree* tr = static_cast<Fl_Tree*>(w);
    Fl_Tree_Item* it = tr->callback_item();
    if (!it) return;
    if (tr->callback_reason() == FL_TREE_REASON_SELECTED && !it->has_children()) {
        char rel[FL_PATH_MAX];
        tr->item_pathname(rel, sizeof(rel), it);
        const char* root_lbl = file_tree->root()->label();
        if (root_lbl && *root_lbl) {
            size_t len = strlen(root_lbl);
            if (!strncmp(rel, root_lbl, len) && rel[len] == '/')
                memmove(rel, rel + len + 1, strlen(rel + len + 1) + 1);
        }
        char path[FL_PATH_MAX * 2];
        snprintf(path, sizeof(path), "%s/%s", current_folder, rel);
        load_file(path);
    }
}

static void open_folder_cb(Fl_Widget*, void*) {
    Fl_Native_File_Chooser fc;
    fc.title("Open Folder...");
    fc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
    if (fc.show() == 0) load_folder(fc.filename());
}

static void save_to(const char *file) {
    if (buffer->savefile(file) == 0) {
        strncpy(current_file, file, sizeof(current_file));
        text_changed = false;
        update_title();
        save_last_file();
        last_save_time = std::time(nullptr);
        update_status();
    } else {
        fl_alert("Cannot save '%s'", file);
    }
}

static void save_cb(Fl_Widget*, void*) {
    if (current_file[0]) save_to(current_file);
    else {
        Fl_Native_File_Chooser fc;
        fc.title("Save File...");
        fc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
        if (fc.show() == 0) save_to(fc.filename());
    }
}

static void quit_cb(Fl_Widget*, void*) {
    if (text_changed) {
        int r = fl_choice("Save changes before quitting?", "Cancel", "Save", "Don't Save");
        if (r == 0) return;
        if (r == 1) save_cb(NULL, NULL);
    }
    save_last_file();
    win->hide();
}

static void apply_theme(Theme theme) {
    if (theme == THEME_DARK) {
        Fl::background(50, 50, 50);
        Fl::background2(60, 60, 60);
        Fl::foreground(230, 230, 230);
        if (menu) {
            menu->color(fl_rgb_color(50, 50, 50));
            menu->textcolor(fl_rgb_color(230, 230, 230));
            menu->selection_color(fl_rgb_color(80, 80, 80));
        }
        if (status_left && status_right) {
            status_left->color(fl_rgb_color(50, 50, 50));
            status_left->labelcolor(fl_rgb_color(230, 230, 230));
            status_right->color(fl_rgb_color(50, 50, 50));
            status_right->labelcolor(fl_rgb_color(230, 230, 230));
        }
        if (editor) {
            editor->color(fl_rgb_color(45,45,45), FL_DARK_BLUE);
            editor->textcolor(FL_WHITE);
            editor->cursor_color(FL_WHITE);
            editor->linenumber_bgcolor(fl_rgb_color(45,45,45));
            editor->linenumber_fgcolor(fl_rgb_color(120,120,120));
            Fl_Scrollbar* hsb = static_cast<Fl_Scrollbar*>(editor->child(0));
            Fl_Scrollbar* vsb = static_cast<Fl_Scrollbar*>(editor->child(1));
            if (hsb) hsb->color(fl_rgb_color(60,60,60), fl_rgb_color(120,120,120));
            if (vsb) vsb->color(fl_rgb_color(60,60,60), fl_rgb_color(120,120,120));
        }
    } else {
        Fl::background(240, 240, 240);
        Fl::background2(250, 250, 250);
        Fl::foreground(30, 30, 30);
        if (menu) {
            menu->color(fl_rgb_color(240, 240, 240));
            menu->textcolor(fl_rgb_color(30, 30, 30));
            menu->selection_color(fl_rgb_color(210, 210, 210));
        }
        if (status_left && status_right) {
            status_left->color(fl_rgb_color(240, 240, 240));
            status_left->labelcolor(fl_rgb_color(30, 30, 30));
            status_right->color(fl_rgb_color(240, 240, 240));
            status_right->labelcolor(fl_rgb_color(30, 30, 30));
        }
        if (editor) {
            editor->color(fl_rgb_color(255,255,255), FL_DARK_BLUE);
            editor->textcolor(FL_BLACK);
            editor->cursor_color(FL_BLACK);
            editor->linenumber_bgcolor(fl_rgb_color(235,235,235));
            editor->linenumber_fgcolor(fl_rgb_color(120,120,120));
            Fl_Scrollbar* hsb = static_cast<Fl_Scrollbar*>(editor->child(0));
            Fl_Scrollbar* vsb = static_cast<Fl_Scrollbar*>(editor->child(1));
            if (hsb) hsb->color(fl_rgb_color(220,220,220), fl_rgb_color(200,200,200));
            if (vsb) vsb->color(fl_rgb_color(220,220,220), fl_rgb_color(200,200,200));
        }
    }
    current_theme = theme;
    if (win) win->redraw();
}

static void theme_light_cb(Fl_Widget*, void*) { apply_theme(THEME_LIGHT); }
static void theme_dark_cb(Fl_Widget*, void*)  { apply_theme(THEME_DARK); }

int main(int argc, char **argv) {
    Fl::get_system_colors();
    // Ensure the editor uses the system's monospace font family
    Fl::set_font(FL_COURIER, "Monospace");
    Fl::set_color(FL_SELECTION_COLOR, fl_rgb_color(75, 110, 175));
    Fl::scheme("gtk+");
    Fl::scrollbar_size(16);              // wider scrollbars for a modern look

    win = new EditorWindow(1301, 887, "Let‘s code");
    win->callback(quit_cb);
    menu = new Fl_Menu_Bar(0, 0, win->w(), 25);
    menu->add("&File/New",  FL_COMMAND + 'n', new_cb);
    menu->add("&File/Open", FL_COMMAND + 'o', open_cb);
    menu->add("&File/Open Folder", 0, open_folder_cb);
    menu->add("&File/Save", FL_COMMAND + 's', save_cb);
    menu->add("&File/Quit", FL_COMMAND + 'q', quit_cb);
    menu->add("&View/Dark Theme", 0, theme_dark_cb);
    menu->add("&View/Light Theme", 0, theme_light_cb);

    const int status_h = 20;
    font_size = load_font_size();
    file_tree = new Fl_Tree(0, 25, tree_width, win->h() - 25 - status_h);
    file_tree->callback(tree_cb);
    file_tree->showroot(false);
    tree_resizer = new TreeResizer(tree_width, 25, 4, win->h() - 25 - status_h);
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
    Fl_Scrollbar* hsb = static_cast<Fl_Scrollbar*>(editor->child(0));
    Fl_Scrollbar* vsb = static_cast<Fl_Scrollbar*>(editor->child(1));
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
                           sizeof(style_table)/sizeof(style_table[0]),
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