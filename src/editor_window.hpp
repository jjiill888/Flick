#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Text_Buffer.H>
#include <ctime>

enum Theme { THEME_DARK, THEME_LIGHT };

class My_Text_Editor : public Fl_Text_Editor {
public:
    using Fl_Text_Editor::Fl_Text_Editor;
    int handle(int e) override;
};

class EditorWindow : public Fl_Double_Window {
public:
    EditorWindow(int W, int H, const char* L = 0);
    void resize(int X, int Y, int W, int H) override;
};

class TreeResizer : public Fl_Box {
public:
    TreeResizer(int X, int Y, int W, int H);
    int handle(int e) override;
};

int run_editor(int argc, char** argv);