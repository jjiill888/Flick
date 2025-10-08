#pragma once
#include <FL/Fl.H>
#include <FL/fl_draw.H>

// Modern Color System - Tokyo Night Theme
// Low saturation, high contrast dark theme

namespace Colors {
    // Base colors - Foundation (layered for depth, optimized contrast)
    constexpr unsigned char WINDOW_BG[3]        = {15, 17, 21};   // #0F1115 - Window background
    constexpr unsigned char EDITOR_BG[3]        = {27, 30, 36};   // #1B1E24 - Editor background (slightly deeper)
    constexpr unsigned char PANEL_BG[3]         = {22, 24, 29};   // #16181D - File tree (darker)
    constexpr unsigned char TAB_BAR_BG[3]       = {35, 40, 50};   // #232832 - Tab bar (lightest)
    constexpr unsigned char BORDER[3]           = {28, 34, 48};   // #1C2230 - Borders and separators

    // Text colors - Typography (brighter for readability)
    constexpr unsigned char TEXT_PRIMARY[3]     = {227, 230, 238}; // #E3E6EE - Main text (brighter blue-white)
    constexpr unsigned char TEXT_SECONDARY[3]   = {166, 173, 187}; // #A6ADBB - Secondary text
    constexpr unsigned char TEXT_DISABLED[3]    = {107, 114, 128}; // #6B7280 - Disabled text

    // Accent colors - Tokyo Night inspired
    constexpr unsigned char ACCENT_BLUE[3]      = {122, 162, 247}; // #7AA2F7 - Primary accent
    constexpr unsigned char ACCENT_CYAN[3]      = {42, 195, 222};  // #2AC3DE - Secondary accent
    constexpr unsigned char ACCENT_PURPLE[3]    = {192, 202, 245}; // #C0CAF5 - Tertiary accent

    // Semantic colors
    constexpr unsigned char SUCCESS[3]          = {158, 206, 106}; // #9ECE6A - Success/green
    constexpr unsigned char WARNING[3]          = {224, 175, 104}; // #E0AF68 - Warning/orange
    constexpr unsigned char ERROR[3]            = {247, 118, 142}; // #F7768E - Error/red

    // Syntax highlighting - Optimized contrast (visible but not harsh)
    constexpr unsigned char SYNTAX_COMMENT[3]   = {108, 122, 142}; // #6C7A8E - Comments (deeper, less distracting)
    constexpr unsigned char SYNTAX_STRING[3]    = {229, 183, 122}; // #E5B77A - Strings (warm)
    constexpr unsigned char SYNTAX_NUMBER[3]    = {231, 185, 116}; // #E7B974 - Numbers/constants (warm accent)
    constexpr unsigned char SYNTAX_KEYWORD[3]   = {92, 130, 230};  // #5C82E6 - Keywords (deeper pure blue, "骨感")
    constexpr unsigned char SYNTAX_FUNCTION[3]  = {139, 217, 141}; // #8BD98D - Functions (soft contrast green)
    constexpr unsigned char SYNTAX_OPERATOR[3]  = {187, 154, 247}; // #BB9AF7 - Operators
    constexpr unsigned char SYNTAX_TYPE[3]      = {102, 153, 255}; // #6699FF - Types (slightly brighter than keywords)
    constexpr unsigned char SYNTAX_VARIABLE[3]  = {227, 230, 238}; // #E3E6EE - Variables (same as primary text)

    // Interactive elements - matching brackets, cursor column, etc.
    constexpr unsigned char BRACKET_MATCH_BG[3] = {40, 46, 60};    // #282E3C - Matching bracket background
    constexpr unsigned char CURSOR_COLUMN[3]    = {16, 19, 25};    // #101319 - Cursor column highlight
    constexpr unsigned char INVISIBLE_CHAR[3]   = {60, 66, 82};    // #3C4252 - Invisible characters (faded)

    // Interactive states - light accent colors
    constexpr unsigned char SELECTION_BG[3]     = {40, 48, 65};    // #283041 - Selection (blue tint)
    constexpr unsigned char CURRENT_LINE[3]     = {29, 33, 41};    // #1D2129 - Current line (subtle blue, ~10% opacity)
    constexpr unsigned char CURSOR_LINE[3]      = {122, 162, 247}; // #7AA2F7 - Cursor line highlight
    constexpr unsigned char ACTIVE_TAB_LINE[3]  = {122, 162, 247}; // #7AA2F7 - Active tab indicator
    constexpr unsigned char HOVER_BG[3]         = {35, 41, 52};    // #232934 - Hover state

    // UI elements
    constexpr unsigned char STATUSBAR_BG[3]     = {15, 17, 21};    // #0F1115 - Status bar
    constexpr unsigned char TITLEBAR_BG[3]      = {15, 17, 21};    // #0F1115 - Title bar
    constexpr unsigned char MENUBAR_BG[3]       = {17, 21, 28};    // #11151C - Menu bar

    // Helper function to create Fl_Color from RGB array
    inline Fl_Color rgb(const unsigned char color[3]) {
        return fl_rgb_color(color[0], color[1], color[2]);
    }

    // Brightness adjustment helpers
    inline Fl_Color brighten(const unsigned char base[3], float factor) {
        unsigned char r = static_cast<unsigned char>(base[0] * factor);
        unsigned char g = static_cast<unsigned char>(base[1] * factor);
        unsigned char b = static_cast<unsigned char>(base[2] * factor);
        return fl_rgb_color(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
    }

    // Alpha blend helper (for overlays)
    inline Fl_Color blend(const unsigned char fg[3], const unsigned char bg[3], float alpha) {
        unsigned char r = static_cast<unsigned char>(fg[0] * alpha + bg[0] * (1 - alpha));
        unsigned char g = static_cast<unsigned char>(fg[1] * alpha + bg[1] * (1 - alpha));
        unsigned char b = static_cast<unsigned char>(fg[2] * alpha + bg[2] * (1 - alpha));
        return fl_rgb_color(r, g, b);
    }
}
