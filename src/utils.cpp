#include "utils.hpp"
#include "file_tree.hpp"
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>
#include <FL/Fl_Scrollbar.H>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <string>

// Style table used for syntax highlighting
Fl_Text_Display::Style_Table_Entry style_table[] = {
    { fl_rgb_color(212,212,212), FL_COURIER,        14 }, // A - plain text
    { fl_rgb_color(106,153,85),  FL_COURIER_ITALIC, 14 }, // B - line comment
    { fl_rgb_color(106,153,85),  FL_COURIER_ITALIC, 14 }, // C - block comment
    { fl_rgb_color(206,145,120), FL_COURIER,        14 }, // D - string literal
    { fl_rgb_color( 86,156,214), FL_COURIER_BOLD,   14 }, // E - preprocessor
    { fl_rgb_color(197,134,192), FL_COURIER_BOLD,   14 }, // F - keyword
    { fl_rgb_color(255,255,0),   FL_COURIER,        14 }  // G - search highlight
};
const int style_table_size = sizeof(style_table) / sizeof(style_table[0]);

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

void style_init() {
    char *text = buffer->text();
    int length = buffer->length();
    char *style = new char[length + 1];
    memset(style, 'A', length);
    style_parse(text, style, length);
    style_buffer->text(style);
    delete[] style;
    free(text);
    update_linenumber_width();
}

const char* font_size_path() {
    static char path[FL_PATH_MAX];
    const char* home = getenv("HOME");
    if (home) snprintf(path, sizeof(path), "%s/.lets_code_fontsize", home);
    else strncpy(path, ".lets_code_fontsize", sizeof(path));
    return path;
}

void save_font_size(int sz) {
    FILE* fp = fopen(font_size_path(), "w");
    if (fp) {
        fprintf(fp, "%d", sz);
        fclose(fp);
    }
}

int load_font_size() {
    int sz = font_size;
    FILE* fp = fopen(font_size_path(), "r");
    if (fp) {
        if (fscanf(fp, "%d", &sz) != 1) sz = font_size;
        fclose(fp);
    }
    return sz;
}

void update_linenumber_width() {
    if (!editor || !buffer) return;

    int lines = buffer->count_lines(0, buffer->length()) + 1;
    int digits = 1;
    for (int temp = lines; temp >= 10; temp /= 10) digits++;

    fl_font(editor->textfont(), editor->textsize());
    int char_width = fl_width('0');
    int width = digits * char_width + 6;  // 可调节 padding
    if (width < 20) width = 20;

    editor->linenumber_width(width);
}

void set_font_size(int sz) {
    font_size = sz;
    if (editor) {
        editor->textsize(sz);
    }
    for (unsigned i = 0; i < sizeof(style_table)/sizeof(style_table[0]); ++i) {
        style_table[i].size = sz;
    }
    save_font_size(sz);
    if (editor) {
        editor->damage(FL_DAMAGE_ALL);
        update_linenumber_width();
    }
}

const char* last_file_path() {
    static char path[FL_PATH_MAX];
    const char* home = getenv("HOME");
    if (home) snprintf(path, sizeof(path), "%s/.lets_code_last", home);
    else strncpy(path, ".lets_code_last", sizeof(path));
    return path;
}

void save_last_file() {
    FILE* fp = fopen(last_file_path(), "w");
    if (fp) {
        fputs(current_file, fp);
        fclose(fp);
    }
}

const char* last_folder_path() {
    static char path[FL_PATH_MAX];
    const char* home = getenv("HOME");
    if (home) snprintf(path, sizeof(path), "%s/.lets_code_last_folder", home);
    else strncpy(path, ".lets_code_last_folder", sizeof(path));
    return path;
}

void save_last_folder() {
    FILE* fp = fopen(last_folder_path(), "w");
    if (fp) {
        fputs(current_folder, fp);
        fclose(fp);
    }
}

void load_last_file_if_any() {
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

void update_title() {
    const char *name = current_file[0] ? fl_filename_name(current_file)
                                       : "Untitled";
    char title[FL_PATH_MAX + 20];
    snprintf(title, sizeof(title), "%s%s - Let‘s code",
             name, text_changed ? "*" : "");
    win->copy_label(title);
}

void update_status() {
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

void changed_cb(int, int, int, int, const char*, void*) {
    text_changed = true;
    update_title();
    style_init();
    update_linenumber_width();
    update_status();
}

void new_cb(Fl_Widget*, void*) {
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

void load_file(const char *file) {
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

void open_cb(Fl_Widget*, void*) {
    Fl_Native_File_Chooser fc;
    fc.title("Open File...");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (fc.show() == 0) load_file(fc.filename());
}

void open_folder_cb(Fl_Widget*, void*) {
    Fl_Native_File_Chooser fc;
    fc.title("Open Folder...");
    fc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
    if (fc.show() == 0) load_folder(fc.filename());
}

void refresh_folder_cb(Fl_Widget*, void*) {
    if (current_folder[0])
        load_folder(current_folder);
}

void save_to(const char *file) {
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

void save_cb(Fl_Widget*, void*) {
    if (current_file[0]) save_to(current_file);
    else {
        Fl_Native_File_Chooser fc;
        fc.title("Save File...");
        fc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
        if (fc.show() == 0) save_to(fc.filename());
    }
}

void quit_cb(Fl_Widget*, void*) {
    if (text_changed) {
        int r = fl_choice("Save changes before quitting?", "Cancel", "Save", "Don't Save");
        if (r == 0) return;
        if (r == 1) save_cb(NULL, NULL);
    }
    save_last_file();
    win->hide();
}

void apply_theme(Theme theme) {
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

void theme_light_cb(Fl_Widget*, void*) { apply_theme(THEME_LIGHT); }
void theme_dark_cb(Fl_Widget*, void*)  { apply_theme(THEME_DARK); }

void cut_cb(Fl_Widget*, void*)        { Fl_Text_Editor::kf_cut(0, editor); }
void copy_cb(Fl_Widget*, void*)       { Fl_Text_Editor::kf_copy(0, editor); }
void paste_cb(Fl_Widget*, void*)      { Fl_Text_Editor::kf_paste(0, editor); }
void select_all_cb(Fl_Widget*, void*) { Fl_Text_Editor::kf_select_all(0, editor); }

static bool replace_all(std::string& data, const std::string& search,
                        const std::string& replace) {
    bool changed = false;
    size_t pos = 0;
    while ((pos = data.find(search, pos)) != std::string::npos) {
        data.replace(pos, search.length(), replace);
        pos += replace.length();
        changed = true;
    }
    return changed;
}

void count_in_file(const char* file, const char* search, int* count) {
    FILE* fp = fopen(file, "rb");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::string data(len, '\0');
    fread(data.data(), 1, len, fp);
    fclose(fp);
    size_t pos = 0;
    while ((pos = data.find(search, pos)) != std::string::npos) {
        ++*count;
        pos += strlen(search);
    }
}

void replace_in_file(const char* file, const char* search,
                     const char* replace) {
    FILE* fp = fopen(file, "rb");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::string data(len, '\0');
    fread(data.data(), 1, len, fp);
    fclose(fp);

    if (replace_all(data, search, replace)) {
        fp = fopen(file, "wb");
        if (fp) {
            fwrite(data.data(), 1, data.size(), fp);
            fclose(fp);
        }
        if (strcmp(file, current_file) == 0) {
            buffer->text(data.c_str());
            text_changed = false;
            update_title();
            style_init();
            last_save_time = std::time(nullptr);
            update_status();
        }
    }
}

int highlight_in_buffer(const char* search, int* first_pos) {
    style_init();
    char* text = buffer->text();
    char* style = style_buffer->text();
    int len = buffer->length();
    size_t slen = strlen(search);
    int count = 0;
    if (first_pos) *first_pos = -1;
    for (int i = 0; i <= len - (int)slen; ) {
        if (strncmp(text + i, search, slen) == 0) {
            if (first_pos && *first_pos == -1) *first_pos = i;
            for (size_t k = 0; k < slen && i + (int)k < len; ++k)
                style[i + k] = 'G';
            i += slen;
            ++count;
        } else {
            ++i;
        }
    }
    style_buffer->text(style);
    free(style);
    free(text);
    editor->damage(FL_DAMAGE_ALL);
    return count;
}

void count_in_folder(const char* folder, const char* search, int* count) {
    DIR* d = opendir(folder);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char path[FL_PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", folder, e->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode))
            count_in_folder(path, search, count);
        else if (S_ISREG(st.st_mode))
            count_in_file(path, search, count);
    }
    closedir(d);
}

void replace_in_folder(const char* folder, const char* search,
                       const char* replace) {
    DIR* d = opendir(folder);
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char path[FL_PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", folder, e->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode))
            replace_in_folder(path, search, replace);
        else if (S_ISREG(st.st_mode))
            replace_in_file(path, search, replace);
    }
    closedir(d);
}

void find_cb(Fl_Widget*, void*) {
    if (!current_folder[0]) {
        fl_alert("No folder opened");
        return;
    }
    const char* term = fl_input("Find:", "");
    if (!term || !*term) return;
    int total = 0;
    count_in_folder(current_folder, term, &total);
    int first_pos = -1;
    int current = highlight_in_buffer(term, &first_pos);
    if (first_pos >= 0) {
        buffer->select(first_pos, first_pos + strlen(term));
        editor->insert_position(first_pos);
        int line = buffer->count_lines(0, first_pos);
        int lines_vis = editor->h() / (editor->textsize() + 4);
        int top = line - lines_vis/2;
        if (top < 0) top = 0;
        editor->scroll(top, 0);
        editor->show_insert_position();
    }
    char msg[128];
    snprintf(msg, sizeof(msg), "Found %d matches (%d in current file)", total, current);
    fl_message("%s", msg);
}

void replace_cb(Fl_Widget*, void*) {
    if (!current_folder[0] && !current_file[0]) {
        fl_alert("No file opened");
        return;
    }
    const char* find = fl_input("Find:", "");
    if (!find || !*find) return;
    const char* repl = fl_input("Replace with:", "");
    if (!repl) return;
    int total = 0;
    if (current_folder[0])
        count_in_folder(current_folder, find, &total);
    else
        count_in_file(current_file, find, &total);
    if (total == 0) {
        fl_message("No matches found");
        return;
    }
    char msg[128];
    snprintf(msg, sizeof(msg), "Replace %d occurrences?", total);
    if (fl_choice("%s", "Cancel", "OK", NULL, msg) != 1) return;
    if (current_folder[0])
        replace_in_folder(current_folder, find, repl);
    else
        replace_in_file(current_file, find, repl);
    int first_pos = -1;
    highlight_in_buffer(repl, &first_pos);
    if (first_pos >= 0) {
        buffer->select(first_pos, first_pos + strlen(repl));
        editor->insert_position(first_pos);
        int line = buffer->count_lines(0, first_pos);
        int lines_vis = editor->h() / (editor->textsize() + 4);
        int top = line - lines_vis/2;
        if (top < 0) top = 0;
        editor->scroll(top, 0);
        editor->show_insert_position();
    }
    fl_message("Replace complete");
}