#include <sstream>
#include <vector>
#include <string>

// Function to split a string by a delimiter
void split(const std::string &s, const std::string &delimiter, std::vector<std::string> &result) {
    size_t start = 0;
    size_t end = s.find(delimiter);
    while (end != std::string::npos) {
        result.push_back(s.substr(start, end - start));
        start = end + delimiter.length();
        end = s.find(delimiter, start);
    }
    result.push_back(s.substr(start, end));
}
/*
std::string foo = "This is some string I want to split by spaces.";
std::vector<std::string> results;
split(foo, " ", results);
*/