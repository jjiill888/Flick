#include "SearchReplace.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace SearchReplace {

static bool is_text_file(const fs::path& p) {
    auto ext = p.extension().string();
    return ext == ".txt" || ext == ".cpp" || ext == ".hpp" ||
           ext == ".c"   || ext == ".h";
}

int findInBuffer(Fl_Text_Buffer* buffer, const std::string& keyword) {
    if (!buffer || keyword.empty()) return 0;
    char* raw = buffer->text();
    std::string text = raw ? raw : "";
    free(raw);
    int count = 0;
    size_t pos = 0;
    while ((pos = text.find(keyword, pos)) != std::string::npos) {
        ++count;
        pos += keyword.size();
    }
    return count;
}

int replaceInBuffer(Fl_Text_Buffer* buffer, const std::string& keyword,
                    const std::string& replacement) {
    if (!buffer || keyword.empty()) return 0;
    char* raw = buffer->text();
    std::string text = raw ? raw : "";
    free(raw);
    int count = 0;
    size_t pos = 0;
    while ((pos = text.find(keyword, pos)) != std::string::npos) {
        text.replace(pos, keyword.size(), replacement);
        pos += replacement.size();
        ++count;
    }
    if (count > 0) buffer->text(text.c_str());
    return count;
}

static int count_in_file(const fs::path& file, const std::string& keyword,
                         std::string* content_out = nullptr) {
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) return 0;
    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();
    if (content.find('\0') != std::string::npos) return 0; // skip binary
    if (content_out) *content_out = content;
    int count = 0;
    size_t pos = 0;
    while ((pos = content.find(keyword, pos)) != std::string::npos) {
        ++count;
        pos += keyword.size();
    }
    return count;
}

int findInFolder(const std::string& folderPath, const std::string& keyword,
                 std::string* firstPath) {
    if (keyword.empty()) return 0;
    int total = 0;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        if (!is_text_file(entry.path())) continue;
        int found = count_in_file(entry.path(), keyword);
        if (found) {
            if (firstPath && firstPath->empty())
                *firstPath = entry.path().string();
            total += found;
        }
    }
    return total;
}

int replaceInFolder(const std::string& folderPath, const std::string& keyword,
                    const std::string& replacement) {
    if (keyword.empty()) return 0;
    int total = 0;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) continue;
        if (!is_text_file(entry.path())) continue;
        std::string content;
        int found = count_in_file(entry.path(), keyword, &content);
        if (!found) continue;
        std::string replaced = content;
        size_t pos = 0;
        int cnt = 0;
        while ((pos = replaced.find(keyword, pos)) != std::string::npos) {
            replaced.replace(pos, keyword.size(), replacement);
            pos += replacement.size();
            ++cnt;
        }
        if (cnt) {
            std::ofstream ofs(entry.path(), std::ios::binary | std::ios::trunc);
            if (ofs) {
                ofs << replaced;
                total += cnt;
            }
        }
    }
    return total;
}

}