#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <FL/Fl_Box.H>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

static Fl_Double_Window *win;
static Fl_Menu_Bar    *menu;
static Fl_Text_Editor  *editor;
static Fl_Text_Buffer  *buffer = new Fl_Text_Buffer();
static Fl_Text_Buffer  *style_buffer = new Fl_Text_Buffer();
static bool text_changed = false;
static char current_file[FL_PATH_MAX] = "";
static Fl_Box          *status_bar = nullptr;

class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int W, int H, const char* L = 0)
        : Fl_Double_Window(W, H, L) {}

    void resize(int X, int Y, int W, int H) override {
        Fl_Double_Window::resize(X, Y, W, H);
        if (menu && editor && status_bar) {
            const int status_h = status_bar->h();
            editor->size(W, H - menu->h() - status_h);
            menu->size(W, menu->h());
            status_bar->position(0, H - status_h);
            status_bar->size(W, status_h);
        }
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
            style_init();
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

static void changed_cb(int, int, int, int, const char*, void*) {
    text_changed = true;
    update_title();
    style_init();
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
}

static void load_file(const char *file) {
    if (buffer->loadfile(file) == 0) {
        strncpy(current_file, file, sizeof(current_file));
        text_changed = false;
        update_title();
        save_last_file();
        style_init();
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

static void save_to(const char *file) {
    if (buffer->savefile(file) == 0) {
        strncpy(current_file, file, sizeof(current_file));
        text_changed = false;
        update_title();
        save_last_file();
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

int main(int argc, char **argv) {
    // Use a consistent dark theme for the entire application
    Fl::get_system_colors();
    Fl::background(50, 50, 50);          // primary background
    Fl::background2(60, 60, 60);         // secondary background
    Fl::foreground(230, 230, 230);       // text color
    // Ensure the editor uses the system's monospace font family
    Fl::set_font(FL_COURIER, "Monospace");
    Fl::set_color(FL_SELECTION_COLOR, fl_rgb_color(75, 110, 175));
    Fl::scheme("gtk+");
    Fl::scrollbar_size(16);              // wider scrollbars for a modern look

    win = new EditorWindow(800, 600, "Let‘s code");
    menu = new Fl_Menu_Bar(0, 0, win->w(), 25);
    menu->color(fl_rgb_color(50, 50, 50));
    menu->textcolor(fl_rgb_color(230, 230, 230));
    menu->selection_color(fl_rgb_color(80, 80, 80));
    menu->add("&File/New",  FL_COMMAND + 'n', new_cb);
    menu->add("&File/Open", FL_COMMAND + 'o', open_cb);
    menu->add("&File/Save", FL_COMMAND + 's', save_cb);
    menu->add("&File/Quit", FL_COMMAND + 'q', quit_cb);

    const int status_h = 20;
    editor = new Fl_Text_Editor(0, 25, win->w(), win->h() - 25 - status_h);
    editor->buffer(buffer);
    editor->textfont(FL_COURIER);
    editor->textsize(14);
    editor->linenumber_width(30);
    editor->linenumber_bgcolor(fl_rgb_color(45,45,45));
    editor->linenumber_fgcolor(fl_rgb_color(120,120,120));
    editor->linenumber_align(FL_ALIGN_RIGHT);
    editor->scrollbar_width(Fl::scrollbar_size());
    editor->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    editor->color(fl_rgb_color(45,45,45), FL_DARK_BLUE);
    editor->textcolor(FL_WHITE);
    editor->cursor_color(FL_WHITE);
    status_bar = new Fl_Box(0, win->h() - status_h, win->w(), status_h);
    status_bar->box(FL_FLAT_BOX);
    status_bar->color(fl_rgb_color(50, 50, 50));
    status_bar->labelcolor(fl_rgb_color(230, 230, 230));
    status_bar->labelsize(12);
    status_bar->label("");
    buffer->add_modify_callback(changed_cb, nullptr);
    style_init();
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
    }

    return Fl::run();
}