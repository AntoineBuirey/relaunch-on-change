#include "scanner.hpp"

#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

// Function to scan source files based on a given pattern
// a pattern is a string that can be used to match file names, for example "*.cpp" to match all C++ source files
// it support double wildcard patterns, for example "src/**/*.cpp" to match all C++ source files in the src directory and its subdirectories
std::set<std::string> scanSources(const std::string& pattern){
    std::string normalizedPattern = pattern;
    for (char& character : normalizedPattern) {
        if (character == '\\') {
            character = '/';
        }
    }

    while (normalizedPattern.compare(0, 2, "./") == 0) {
        normalizedPattern.erase(0, 2);
    }

    const bool matchPath = normalizedPattern.find('/') != std::string::npos;
    std::string regexPattern = "^";
    for (std::size_t i = 0; i < normalizedPattern.size(); ++i) {
        const char character = normalizedPattern[i];
        if (character == '*' && i + 1 < normalizedPattern.size() &&
            normalizedPattern[i + 1] == '*') {
            i++;
            if (i + 1 < normalizedPattern.size() && normalizedPattern[i + 1] == '/') {
                i++;
                regexPattern += "(.*/)?";
            } else {
                regexPattern += ".*";
            }
        } else if (character == '*') {
            regexPattern += "[^/]*";
        } else if (character == '?') {
            regexPattern += "[^/]";
        } else {
            if (std::string(".^$|()[]{}+\\").find(character) != std::string::npos) {
                regexPattern += '\\';
            }
            regexPattern += character;
        }
    }
    regexPattern += "$";

    const std::regex matcher(regexPattern);
    const fs::path root = fs::current_path();
    std::set<std::string> sources;

    std::error_code error;
    fs::recursive_directory_iterator entries(
        root, fs::directory_options::skip_permission_denied, error);
    const fs::recursive_directory_iterator end;
    for (; entries != end; entries.increment(error)) {
        if (error) {
            error.clear();
            continue;
        }
        if (!entries->is_regular_file(error) || error) {
            error.clear();
            continue;
        }

        const fs::path relativePath = entries->path().lexically_relative(root);
        const std::string candidate = matchPath
            ? relativePath.generic_string()
            : relativePath.filename().generic_string();
        if (std::regex_match(candidate, matcher)) {
            sources.insert(entries->path().string());
        }
    }

    return sources;
}