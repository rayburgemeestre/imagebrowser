#include <fstream>
#include <iostream>
#include <regex>
#include <set>

#define CROW_JSON_USE_MAP
#include "crow.h"

uint64_t file_idx = 0;
std::map<uint64_t, std::string> files_lookup;
std::map<std::string, uint64_t> file_id_lookup;
struct file {
  uint64_t id;
  time_t epoch;
  char md5[33] = {0x00};
  int width = 0;
  int height = 0;
  short year = 0;
  short month = 0;
  short day = 0;
};

void get_thumb_width(int max_thumb_width, int &thumb_width, int &thumb_height);

void get_thumb_height(int max_thumb_height, int &thumb_width,
                      int &thumb_height);

std::string get_thumb_url(const std::string &input);

std::vector<file> all_files_sorted;
std::map<uint64_t, uint64_t> file_idx_lookup;

void load_all_files() {
  std::ifstream fi;
  fi.open("data_dir/meta.txt");
  std::string line;
  const std::string regex_str =
      R"XXX(^\/shadow_dir\/(.*) = (\d+):(\d+):(\d+) (\d+):(\d+):(\d+)(\+02:00|) (\S+)$)XXX";
  std::regex self_regex(regex_str);
  while (getline(fi, line, '\n')) {
    //	if (line.find("mp4") == std::string::npos) {
    //		continue;
    //	}
    std::smatch match;
    if (std::regex_match(line, match, self_regex)) {
      auto file_id = file_idx++;
      files_lookup[file_id] = match[1];
      file_id_lookup[match[1]] = file_id;
      file f;
      f.id = file_id;

      f.year = std::stoi(match[2]);
      f.month = std::stoi(match[3]);
      f.day = std::stoi(match[4]);

      struct tm t = {0}; // Initalize to all 0's
      t.tm_year = f.year - 1900;
      t.tm_mon = f.month - 1;
      t.tm_mday = f.day;
      t.tm_hour = std::stoi(match[5]);
      t.tm_min = std::stoi(match[6]);
      t.tm_sec = std::stoi(match[7]);
      f.epoch = mktime(&t);
      std::string(match[8]).copy(f.md5, 32);
      all_files_sorted.emplace_back(f);
    }
  }
  std::sort(all_files_sorted.begin(), all_files_sorted.end(),
            [](const file &a, const file &b) { return a.epoch < b.epoch; });
  size_t idx = 0;
  for (auto &f : all_files_sorted) {
    file_idx_lookup[f.id] = idx++;
  }
}

void load_file_dimensions() {
  std::ifstream fi;
  fi.open("data_dir/sizes.txt");
  std::string line;
  const std::string regex_str =
      R"XXX(^input_dir\/(.*) = (\d+)x(\d+)(\+0\+0|)$)XXX";
  std::regex self_regex(regex_str);
  while (getline(fi, line, '\n')) {
    //	if (line.find("mp4") == std::string::npos) {
    //		continue;
    //	}
    std::smatch match;
    if (std::regex_match(line, match, self_regex)) {
      auto file_id = file_id_lookup[match[1]];
      auto file_idx = file_idx_lookup[file_id];
      if (file_idx >= all_files_sorted.size()) {
        continue;
      }
      auto &file = all_files_sorted[file_idx];
      file.width = std::stoi(match[2]);
      file.height = std::stoi(match[3]);
    }
  }
}

int main() {
  std::cout << "Loading data... 1/2..." << std::endl;
  load_all_files();
  std::cout << "Loading data... 2/2..." << std::endl;
  load_file_dimensions();

  short year = 0;
  short month = 0;
  short day = 0;
  size_t index = 0;
  int64_t last_epoch = 0;

  std::vector<crow::json::wvalue> vec;

  for (auto &f : all_files_sorted) {
    if (f.year != year || f.month != month || f.day != day) {
      double perc = (double(index) / all_files_sorted.size()) * 100;
      vec.push_back(crow::json::wvalue({{"year", year},
                                        {"month", month},
                                        {"day", day},
                                        {"percentage", perc},
                                        {"index", index}}));
      year = f.year;
      month = f.month;
      day = f.day;
      last_epoch = f.epoch;
    }
    index++;
  }

  crow::SimpleApp app;
  crow::mustache::set_base(".");

  CROW_ROUTE(app, "/")
  ([] {
    crow::mustache::context ctx;
    return crow::mustache::load("index.html").render();
  });

  CROW_ROUTE(app, "/get_time_indexes")
  ([&] {
    crow::json::wvalue x(vec);
    return x;
  });

  CROW_ROUTE(app, "/num_images")
  ([&] {
    crow::json::wvalue x(all_files_sorted.size());
    return x;
  });

  CROW_ROUTE(app, "/next/<int>/<int>/<int>/<int>/<int>")
  ([&](int requested_index, int width, int height, int max_thumb_width,
       int max_thumb_height) {
    std::vector<crow::json::wvalue> vec;

    auto image_spacing = 2;
    auto x = 0;
    auto y = 0;
    auto index = requested_index;

    while (index < all_files_sorted.size()) {
      const auto &img = all_files_sorted[index];
      const auto s = files_lookup[img.id];
      auto thumb_width = img.width;
      auto thumb_height = img.height;

      get_thumb_width(max_thumb_width, thumb_width, thumb_height);
      get_thumb_height(max_thumb_height, thumb_width, thumb_height);

      if (x + thumb_width > width) {
        x = 0;
        y += max_thumb_height + image_spacing;
      }
      if (y + thumb_height > height) {
        index++;
        break;
      }

      vec.push_back(crow::json::wvalue({{"index", index},
                                        {"x", x},
                                        {"y", y},
                                        {"url", s},
                                        {"thumb_url", get_thumb_url(s)},
                                        {"width", thumb_width},
                                        {"height", thumb_height}}));

      // advance for next
      x += thumb_width + image_spacing;
      index++;
    }
    crow::json::wvalue xx(vec);
    return xx;
  });

  CROW_ROUTE(app, "/previous/<int>/<int>/<int>/<int>/<int>")
  ([&](int requested_index, int width, int height, int max_thumb_width,
       int max_thumb_height) {
    std::vector<crow::json::wvalue> vec;

    auto image_spacing = 2;
    auto x = width;
    auto y = height;
    auto index = requested_index;
    int last_x = 0;
    int last_y = 0;

    while (index >= 0 && index < all_files_sorted.size()) {
      const auto &img = all_files_sorted[index];
      const auto s = files_lookup[img.id];
      auto thumb_width = img.width;
      auto thumb_height = img.height;

      get_thumb_width(max_thumb_width, thumb_width, thumb_height);
      get_thumb_height(max_thumb_height, thumb_width, thumb_height);

      if (x - thumb_width < 0) {
        x = width;
        y -= max_thumb_height + image_spacing;
      }
      if (y - thumb_height < 0) {
        index--;
        break;
      }

      last_x = x - thumb_width;
      last_y = y - thumb_height;
      vec.push_back(crow::json::wvalue({{"index", index},
                                        {"x", x - thumb_width},
                                        {"y", y - thumb_height},
                                        {"url", s},
                                        {"thumb_url", get_thumb_url(s)},
                                        {"width", thumb_width},
                                        {"height", thumb_height}}));

      // advance for next
      x -= thumb_width + image_spacing;
      index--;
    }
    // reverse vector vec
    std::reverse(vec.begin(), vec.end());
    // no idea how to fix this. seems impossible, we need to store it
    // separately?
    //	or (auto& v : vec) {
    //		v["x"] = crow::json::rvalue(v["x"].nt()).operator int() +
    // last_x; 		v["y"] = crow::json::rvalue(v["y"].nt()).operator int()
    // + last_y;
    //
    crow::json::wvalue xx(vec);
    return xx;
  });

  app.port(18080).multithreaded().run();

  return 0;
}

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
