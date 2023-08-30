#include "thumbs.h"

#include <set>

void get_thumb_height(int max_thumb_height, int &thumb_width,
                      int &thumb_height) {
  if (thumb_height > max_thumb_height) {
    thumb_width = (thumb_width * max_thumb_height) / thumb_height;
    thumb_height = max_thumb_height;
  }
}

void get_thumb_width(int max_thumb_width, int &thumb_width, int &thumb_height) {
  if (thumb_width > max_thumb_width) {
    thumb_height = (thumb_height * max_thumb_width) / thumb_width;
    thumb_width = max_thumb_width;
  }
}

std::string get_thumb_url(const std::string &input) {
  static std::set<std::string> extensions = {".mp4", ".mkv", ".mov", ".m2t",
                                             ".avi"};
  auto ext = input.substr(input.find_last_of("."));
  // convert ext to lower case
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  // if ext is in extensions, return input
  if (extensions.find(ext) != extensions.end()) {
    return input + ".gif";
  }
  return input;
}
