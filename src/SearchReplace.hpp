#pragma once
#include <string>
#include <FL/Fl_Text_Buffer.H>

namespace SearchReplace {
    // Search in current buffer
    int findInBuffer(Fl_Text_Buffer* buffer, const std::string& keyword);

    // Replace in current buffer
    int replaceInBuffer(Fl_Text_Buffer* buffer, const std::string& keyword, const std::string& replacement);

    // Search recursively in all text files under a folder
    int findInFolder(const std::string& folderPath, const std::string& keyword,
                     std::string* firstPath = nullptr);

    // Replace in all text files under a folder
    int replaceInFolder(const std::string& folderPath, const std::string& keyword, const std::string& replacement);
}