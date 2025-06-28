#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <cstdio>
#include <cstdlib>

static Fl_Double_Window *win;
static Fl_Text_Editor  *editor;
static Fl_Text_Buffer  *buffer = new Fl_Text_Buffer();
static bool text_changed = false;
static char current_file[FL_PATH_MAX] = "";

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
        if (current_file[0])
            buffer->loadfile(current_file);
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
        save_last_file();
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
    // Use a dark color palette similar to the IntelliJ IDEA theme
    Fl::background(43, 43, 43);          // primary background
    Fl::background2(60, 63, 65);         // secondary background
    Fl::foreground(220, 220, 220);       // text color
    Fl::set_color(FL_SELECTION_COLOR, fl_rgb_color(75, 110, 175));
    Fl::scheme("gtk+");
    Fl::scrollbar_size(16);              // wider scrollbars for a modern look

    win = new Fl_Double_Window(800, 600, "Let‘s code");
    Fl_Menu_Bar *menu = new Fl_Menu_Bar(0, 0, win->w(), 25);
    menu->add("&File/New",  FL_COMMAND + 'n', new_cb);
    menu->add("&File/Open", FL_COMMAND + 'o', open_cb);
    menu->add("&File/Save", FL_COMMAND + 's', save_cb);
    menu->add("&File/Quit", FL_COMMAND + 'q', quit_cb);

    editor = new Fl_Text_Editor(0, 25, win->w(), win->h() - 25);
    editor->buffer(buffer);
    editor->textfont(FL_COURIER);
    editor->linenumber_width(40);
    editor->scrollbar_width(Fl::scrollbar_size());
    editor->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    editor->color(fl_rgb_color(45,45,45));
    editor->textcolor(FL_WHITE);
    editor->cursor_color(FL_WHITE);
    buffer->add_modify_callback(changed_cb, nullptr);

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
