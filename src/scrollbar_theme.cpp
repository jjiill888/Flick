#include "scrollbar_theme.hpp"
#include "globals.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>

// Clean modern scrollbars without arrow buttons
#if __has_include(<FL/fl_version.h>)
#  include <FL/fl_version.h>
#endif
#if __has_include(<FL/Fl_Style.H>)
#  include <FL/Fl_Style.H>
#  define HAVE_FL_STYLE 1
#endif

#if defined(FL_MAJOR_VERSION) && ((FL_MAJOR_VERSION>1) || (FL_MAJOR_VERSION==1 && FL_MINOR_VERSION>=5))
#  define BOXDRAW_HAS_WIDGET 1
#endif

static Fl_Boxtype track_box = FL_NO_BOX;
static Fl_Boxtype thumb_box = FL_NO_BOX;

#ifdef BOXDRAW_HAS_WIDGET
static void draw_track(const Fl_Widget*, int X, int Y, int W, int H, Fl_Color c)
#else
static void draw_track(int X, int Y, int W, int H, Fl_Color c)
#endif
{
    fl_color(c);
    fl_rectf(X, Y, W, H);
}

#ifdef BOXDRAW_HAS_WIDGET
static void draw_thumb(const Fl_Widget* w, int X, int Y, int W, int H, Fl_Color c)
#else
static void draw_thumb(int X, int Y, int W, int H, Fl_Color c)
#endif
{
    Fl_Color col = c;
#ifdef BOXDRAW_HAS_WIDGET
    if (Fl::belowmouse() == w)
        col = fl_darker(c);
#else
    // Older FLTK versions don't provide the widget pointer here,
    // so skip hover highlighting
#endif
    fl_color(col);
    fl_rectf(X, Y, W, H);
}

void apply_scrollbar_style() {
    if (track_box == FL_NO_BOX) {
#ifdef FL_FREE_BOXTYPE
        static int next_box = FL_FREE_BOXTYPE;
#else
        static int next_box = 100;
#endif
        track_box = static_cast<Fl_Boxtype>(next_box++);
        thumb_box = static_cast<Fl_Boxtype>(next_box++);
        Fl::set_boxtype(track_box, draw_track, 0, 0, 0, 0);
        Fl::set_boxtype(thumb_box, draw_thumb, 0, 0, 0, 0);
    }

    Fl::scrollbar_size(6);

#ifdef HAVE_FL_STYLE
    // apply to default style so new scrollbars inherit it
    Fl_Named_Style* s = Fl_Scrollbar::default_style;
    if (s) {
        s->box = track_box;
        s->slider = thumb_box;
        s->color = fl_rgb_color(245, 245, 245);
        s->selection_color = fl_rgb_color(160, 160, 160);
    }
#endif
}

Fl_Boxtype scrollbar_track_box() {
    return track_box;
}

Fl_Boxtype scrollbar_thumb_box() {
    return thumb_box;
}