#include "util.h"

#include <sstream>

void split(const std::string &s, char delimiter,
           std::vector<std::string> &out) {
  std::istringstream ss(s);
  std::string item;
  while (getline(ss, item, delimiter)) {
    out.push_back(item);
  }
}

std::string trim(const std::string &str) {
  size_t start = str.find_first_not_of(" \t\n\r\f\v");
  size_t end = str.find_last_not_of(" \t\n\r\f\v");

  if (start == std::string::npos) // Check if the string is all whitespace
    return "";

  return str.substr(start, end - start + 1);
}

std::string replace_perc20(const std::string &file_name) {
  std::string modifiedName = file_name;
  size_t pos = 0;

  while ((pos = modifiedName.find("%20", pos)) != std::string::npos) {
    modifiedName.replace(pos, 3, " "); // 3 characters in "%20"
    pos += 1; // Move forward by one character (to avoid infinite loops in case
              // of overlapping patterns, though not the case with %20)
  }

  return modifiedName;
}
