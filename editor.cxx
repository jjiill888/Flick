#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Menu_Bar.H>
#include <fstream>

Fl_Text_Editor* editor;
Fl_Text_Buffer* buffer = new Fl_Text_Buffer();

void open_cb(Fl_Widget*, void*) {
    const char* filename = fl_file_chooser("Open File", "*", NULL);
    if (filename) {
        std::ifstream ifs(filename);
        std::string content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
        buffer->text(content.c_str());
    }
}

int main() {
    Fl_Window win(800, 600, "FLTK Text Editor");
    Fl_Menu_Bar menu(0, 0, 800, 25);
    menu.add("&File/Open", FL_CTRL + 'o', open_cb);

    editor = new Fl_Text_Editor(0, 25, 800, 575);
    editor->buffer(buffer);

    win.end();
    win.show();
    return Fl::run();
}
