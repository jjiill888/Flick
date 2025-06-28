#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <fstream>

static Fl_Double_Window *win;
static Fl_Text_Editor  *editor;
static Fl_Text_Buffer  *buffer = new Fl_Text_Buffer();
static bool text_changed = false;
static char current_file[FL_PATH_MAX] = "";

static void update_title() {
    const char *name = current_file[0] ? fl_filename_name(current_file)
                                       : "Untitled";
    char title[FL_PATH_MAX + 20];
    snprintf(title, sizeof(title), "%s%s - FLTK Text Editor",
             name, text_changed ? "*" : "");
    win->copy_label(title);
}

static void changed_cb(int, int, int, int, const char*, void*) {
    text_changed = true;
    update_title();
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
}

static void load_file(const char *file) {
    if (buffer->loadfile(file) == 0) {
        strncpy(current_file, file, sizeof(current_file));
        text_changed = false;
        update_title();
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
    win->hide();
}

int main(int argc, char **argv) {
    Fl::background(38, 38, 38);
    Fl::foreground(220, 220, 220);

    win = new Fl_Double_Window(800, 600, "FLTK Text Editor");
    Fl_Menu_Bar *menu = new Fl_Menu_Bar(0, 0, win->w(), 25);
    menu->add("&File/New",  FL_COMMAND + 'n', new_cb);
    menu->add("&File/Open", FL_COMMAND + 'o', open_cb);
    menu->add("&File/Save", FL_COMMAND + 's', save_cb);
    menu->add("&File/Quit", FL_COMMAND + 'q', quit_cb);

    editor = new Fl_Text_Editor(0, 25, win->w(), win->h() - 25);
    editor->buffer(buffer);
    editor->textfont(FL_COURIER);
    editor->linenumber_width(40);
    editor->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    editor->color(fl_rgb_color(45,45,45));
    editor->textcolor(FL_WHITE);
    editor->cursor_color(FL_WHITE);
    buffer->add_modify_callback(changed_cb, nullptr);

    win->resizable(editor);
    win->end();
    win->show(argc, argv);

    if (argc > 1) load_file(argv[1]);
    else update_title();

    return Fl::run();
}
