#include <fstream>
#include <iostream>
#include <mutex>
#include <regex>
#include <set>

#define CROW_JSON_USE_MAP
#include "crow.h"

uint64_t file_idx = 0;
std::map<uint64_t, std::string> files_lookup;
std::map<std::string, uint64_t> file_id_lookup;
std::map<uint64_t, std::vector<std::string>> file_tags;
std::map<uint64_t, std::string> file_orientations;
// TODO: handy, for duplicates
// std::map<uint64_t, std::string> file_hashes;
// std::map<uint64_t, std::string<std::string>> hash_to_file;
std::set<std::string> unique_tags;
std::vector<std::string> selected_tags;

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

// utils

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

void get_thumb_width(int max_thumb_width, int &thumb_width, int &thumb_height);

void get_thumb_height(int max_thumb_height, int &thumb_width,
                      int &thumb_height);

std::string get_thumb_url(const std::string &input);

std::vector<file> all_files_sorted;
std::vector<file> filtered_files_sorted;
std::map<uint64_t, uint64_t> file_idx_lookup;

std::string get_tags_string(uint64_t id) {
  std::stringstream ss;
  auto pos = file_tags.find(id);
  if (pos != file_tags.end()) {
    for (auto &t : pos->second) {
      ss << t << ";";
    }
  }
  return ss.str();
}

void load_all_files() {
  std::ifstream fi;
  // fi.open("data_dir/meta.txt");
  fi.open("/mnt2/NAS/photos/photo_app/data_dir/meta.txt");
  std::string line;
  const std::string regex_str =
      // SKIPPING: /shadow_dir/Ray/2013-06-XX/2013-06-02 21.34.51.jpg =
      // 2013:06:02 21:34:51+02.00 1cf31383ab314411d68bf60e5ffa93fa
      //
      R"XXX(^\/shadow_dir\/(.*) = (\d+)[-:](\d+)[-:](\d+)[ T](\d+):(\d+):(\d+)(\+\d\d[.:]\d\d|) (\S+)$)XXX";
  std::regex self_regex(regex_str);
  while (getline(fi, line, '\n')) {
    //    if (line.find("-WA") != std::string::npos) {
    //      continue;
    //    }
    //    if (line.find("Screenshots") != std::string::npos) {
    //      continue;
    //    }
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
    } else {
      std::cerr << "Error, cannot parse date: " << line << std::endl;
    }
  }
  std::sort(all_files_sorted.begin(), all_files_sorted.end(),
            [](const file &a, const file &b) { return a.epoch < b.epoch; });
  size_t idx = 0;
  for (auto &f : all_files_sorted) {
    file_idx_lookup[f.id] = idx++;
    // std::cout << "Found something: " << f.year << "-" << f.month << "-" <<
    // f.day << std::endl;
  }
}

void load_file_dimensions() {
  std::ifstream fi;
  // fi.open("data_dir/sizes.txt");
  fi.open("/mnt2/NAS/photos/photo_app/data_dir/sizes.txt");
  std::string line;
  const std::string regex_str =
      R"XXX(^input_dir\/(.*) = (\d+)x(\d+)(\+0\+0|)$)XXX";
  std::regex self_regex(regex_str);
  while (getline(fi, line, '\n')) {
    //    if (line.find("-WA") != std::string::npos) {
    //      continue;
    //    }
    //    if (line.find("Screenshots") != std::string::npos) {
    //      continue;
    //    }
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

void load_file_orientations() {
  std::ifstream fi;
  fi.open("/mnt2/NAS/photos/photo_app/orientation.txt");
  std::string line;
  const std::string regex_str = R"XXX(^input_dir\/(.*) = (.*)$)XXX";
  std::regex self_regex(regex_str);
  while (getline(fi, line, '\n')) {
    //    if (line.find("-WA") != std::string::npos) {
    //      continue;
    //    }
    //    if (line.find("Screenshots") != std::string::npos) {
    //      continue;
    //    }
    //	if (line.find("mp4") == std::string::npos) {
    //		continue;
    //	}
    std::smatch match;
    if (std::regex_match(line, match, self_regex)) {
      auto file_id = file_id_lookup[match[1]];
      auto file_idx = file_idx_lookup[file_id];
      if (file_idx >= all_files_sorted.size()) {
          std::cout << "Skipping, something is off here" << std::endl;
        continue;
      }
      auto &file = all_files_sorted[file_idx];
      const auto orientation_str = trim(match[2]);
      // std::cout << "Inputting file orientation: " << file_id << std::endl;
      file_orientations.emplace(file_id, orientation_str);
    }
  }
}

void load_tags() {
  std::ifstream fi("db2.txt");
  if (!fi) {
    std::cerr << "Failed to open db2.txt" << std::endl;
    return;
  }

  std::string line;
  std::string current_file;
  bool is_gif = false;

  while (getline(fi, line)) {
    std::istringstream iss(line);
    std::string keyword;
    iss >> keyword;

    std::string rest_of_line;
    getline(iss, rest_of_line); // gets the rest of the line after the keyword

    if (keyword == "file:") {
      current_file =
          rest_of_line.substr(1); // excluding the space after "file:"
      if (current_file.rfind(".gif") == current_file.length() - 4) {
        is_gif = true;
        current_file = current_file.substr(0, current_file.length() - 4);
      } else {
        is_gif = false;
      }
      current_file = replace_perc20(current_file);
    } else if (keyword == "tags:") {
      std::vector<std::string> tags;
      split(rest_of_line, ';', tags);
      for (auto &t : tags) {
        t = trim(t);
      }

      auto pos = file_id_lookup.find(current_file);
      if (pos == file_id_lookup.end()) {
        std::cerr << "ERROR: cannot find file: " << current_file << " in lookup"
                  << std::endl;
        continue;
        // exit(1);
      }
      file_tags[pos->second] = tags;
      for (const auto &t : tags) {
        unique_tags.insert(t);
      }
    }
  }
}

std::string get_content_type(const std::string &extension) {
  static const std::unordered_map<std::string, std::string> mime_map = {
      {"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"png", "image/png"},
      {"gif", "image/gif"},  {"bmp", "image/bmp"},
  };

  auto it = mime_map.find(extension);
  if (it != mime_map.end()) {
    return it->second;
  } else {
    // default to octet-stream for unknown types
    return "application/octet-stream";
  }
}

crow::response send_image(const std::string &file_path,
                          const std::string &img_name) {
  std::string filepath =
      file_path + "/" +
      img_name; // Adjust as needed for your file system layout
  std::cout << "Filepath = " << filepath;
  std::ifstream img_file(filepath, std::ios::binary | std::ios::ate);

  if (!img_file) {
    // File not found or failed to open
    return crow::response(404);
  }

  std::streamsize size = img_file.tellg();
  img_file.seekg(0, std::ios::beg);

  std::vector<char> buffer(size);
  if (!img_file.read(buffer.data(), size)) {
    // Failed to read the file
    return crow::response(500);
  }

  std::string extension;
  size_t pos = img_name.rfind(".");
  if (pos != std::string::npos) {
    extension = img_name.substr(pos + 1);
  }

  crow::response resp;
  resp.set_header("Content-Type", get_content_type(extension));
  resp.body = std::string(buffer.begin(), buffer.end());
  return resp;
}

int main() {
  std::cout << "Loading data... 1/5..." << std::endl;
  load_all_files();
  filtered_files_sorted = all_files_sorted;
  std::cout << "Loading data... 2/5..." << std::endl;
  load_file_dimensions();
  std::cout << "Loading data... 3/5..." << std::endl;
  load_tags();
  std::cout << "Loading data... 4/5..." << std::endl;
  load_file_orientations();
  std::cout << "Loading data... 5/5..." << std::endl;

  std::cout << "filtered_files_sorted.size() =  "
            << filtered_files_sorted.size() << std::endl;

  std::vector<crow::json::wvalue> vec;
  std::mutex mut;

  const auto update_time_indexes = [&]() {
    std::unique_lock<std::mutex> lock(mut);

    short year = 0;
    short month = 0;
    short day = 0;
    size_t index = 0;
    int64_t last_epoch = 0;

    //	  for (auto& v: vec) {
    //		  v.reset();
    //	  }
    std::cout << "clearing vec" << std::endl;
    vec.clear();
    std::cout << "potential = " << filtered_files_sorted.size() << std::endl;
    for (auto &f : filtered_files_sorted) {
      if (f.year != year || f.month != month || f.day != day) {
        double perc = (double(index) / filtered_files_sorted.size()) * 100;

        if (year < 2000 && year > 2023) {
          // ignore for now
        } else {
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
      }
      index++;
    }
  };

  const auto update_filtered = [&]() {
    std::unique_lock<std::mutex> lock(mut);
    size_t index = 0;
    filtered_files_sorted.clear();
    for (auto &img : all_files_sorted) {
      std::vector<crow::json::wvalue> tags;
      auto pos = file_tags.find(img.id);
      if (pos != file_tags.end()) {
        for (const auto &st : pos->second) {
          tags.emplace_back(st);
        }
      }
      int matches = 0; // matching tags
      if (selected_tags.size()) {
        for (const auto &t : tags) {
          for (const auto &t2 : selected_tags) {
            auto t1 = t.dump();
            t1 = t1.substr(1, t1.size() - 2);
            if (t1 == t2) {
              matches++;
            }
          }
        }
      }
      if (matches == selected_tags.size()) {
        // std::cout << "INCLUDING!" << std::endl;
      } else {
        // std::cout << "SKIPPING!" << std::endl;
        index++;
        continue;
      }
      filtered_files_sorted.emplace_back(img);
    }
  };

  update_time_indexes();

  crow::SimpleApp app;
  crow::mustache::set_base(".");

  CROW_ROUTE(app, "/")
  ([] {
    crow::mustache::context ctx;
    return crow::mustache::load("index.html").render();
  });

  CROW_ROUTE(app, "/get_time_indexes")
  ([&] {
    update_time_indexes();
    std::unique_lock<std::mutex> lock(mut);
    crow::json::wvalue x(vec);
    return x;
  });

  CROW_ROUTE(app, "/get_tags")
  ([&] {
    std::vector<crow::json::wvalue> v;
    for (const auto &t : unique_tags) {
      v.emplace_back(t);
    }
    crow::json::wvalue x(v);
    return x;
  });

  CROW_ROUTE(app, "/get_selected_tags")
  ([&] {
    std::vector<crow::json::wvalue> v;
    for (const auto &t : selected_tags) {
      v.emplace_back(t);
    }
    crow::json::wvalue x(v);
    return x;
  });

  CROW_ROUTE(app, "/set_tags")
      .methods("POST"_method)([&](const crow::request &req) {
        auto x = crow::json::load(req.body);
        if (!x) {
          return crow::response(400, "Invalid JSON");
        }

        // if (!x.has("tags") || !x["tags"].is_array()) {
        //	if (!x.has("tags") || !x["tags"].t() != crow::json::type::Array)
        //{ 		return crow::response(400, "Missing tags array");
        //	}

        // Assuming unique_tags is a std::vector or similar container that you
        // want to update
        selected_tags.clear();
        for (const auto &tag : x["tags"]) {
          selected_tags.push_back(tag.s());
          std::cout << "got it: " << tag.s() << std::endl;
        }

        update_time_indexes();
        update_filtered();

        return crow::response(200, "Tags updated successfully");
      });

  CROW_ROUTE(app, "/num_images")
  ([&] {
    update_filtered();
    crow::json::wvalue x(filtered_files_sorted.size());
    return x;
  });

  CROW_ROUTE(app, "/next/<int>/<int>/<int>/<int>/<int>")
  ([&](int requested_index, int width, int height, int max_thumb_width,
       int max_thumb_height) {
    std::vector<crow::json::wvalue> vec;

    auto image_spacing = 2;
    auto x = 0;
    auto y = 0;
    auto max_thumb_height_for_y = 0;
    auto index = requested_index;

    while (index < filtered_files_sorted.size()) {
      const auto &img = filtered_files_sorted[index];
      const auto s = files_lookup[img.id];
      auto thumb_width = img.width;
      auto thumb_height = img.height;

        std::string orientation = std::to_string(img.id);
        auto pos = file_orientations.find(img.id);
        if (pos != file_orientations.end()) {
            orientation = pos->second;
        }
        if (orientation.find("RightTop") != std::string::npos) {
            std::swap(thumb_width, thumb_height);
        }

        get_thumb_width(max_thumb_width, thumb_width, thumb_height);
        get_thumb_height(max_thumb_height, thumb_width, thumb_height);


        if (thumb_height > max_thumb_height_for_y) {
        max_thumb_height_for_y = thumb_height;
      }

      if (x + thumb_width > width) {
        x = 0;
        y += max_thumb_height + image_spacing;
        max_thumb_height_for_y = 0;
      }
      if (y + thumb_height > height) {
        index++;
        break;
      }

      const auto tags = get_tags_string(img.id);

      vec.push_back(crow::json::wvalue({{"index", index},
                                        {"x", x},
                                        {"y", y},
                                        {"url", s},
                                        {"thumb_url", get_thumb_url(s)},
                                        {"width", thumb_width},
                                        {"height", thumb_height},
                                        {"tags", tags},
                                        {"orientation", orientation}}));

      std::cout << "pushing next: " << s << std::endl;

      // advance for next
      x += thumb_width + image_spacing;
      index++;
    }
    std::cout << "max thumb height for y= " << max_thumb_height_for_y
              << " height = " << height << " last y: " << y << std::endl;
    auto remaining = (height - y) / 2;
    for (auto &v : vec) {
      v["y"] = std::stoi(v["y"].dump()) + remaining;
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
    int min_last_y = -1;

    while (index >= 0 && index < filtered_files_sorted.size()) {
      const auto &img = filtered_files_sorted[index];
      const auto s = files_lookup[img.id];
      auto thumb_width = img.width;
      auto thumb_height = img.height;

        std::string orientation = std::to_string(img.id);
        auto pos = file_orientations.find(img.id);
        if (pos != file_orientations.end()) {
            orientation = pos->second;
        }
        if (orientation.find("RightTop") != std::string::npos) {
            std::swap(thumb_width, thumb_height);
        }

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
      if (min_last_y == -1)
        min_last_y = last_y;
      else if (last_y < min_last_y)
        min_last_y = last_y;

      const auto tags = get_tags_string(img.id);

      vec.push_back(crow::json::wvalue({{"index", index},
                                        {"x", x - thumb_width},
                                        {"y", y - thumb_height},
                                        {"url", s},
                                        {"thumb_url", get_thumb_url(s)},
                                        {"width", thumb_width},
                                        {"height", thumb_height},
                                        {"tags", tags},
                                        {"orientation", orientation}}));

      std::cout << "pushing previous: " << s << std::endl;
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
    std::cout << "remaining height =" << min_last_y << std::endl;
    min_last_y /= 2;
    for (auto &v : vec) {
      v["y"] = std::stoi(v["y"].dump()) - min_last_y;
    }
    crow::json::wvalue xx(vec);
    return xx;
  });

  CROW_ROUTE(app, "/img/<string>/<string>")
  ([&](const std::string &type, const std::string &img) {
    // source https://stackoverflow.com/questions/154536/encode-decode-urls-in-c
    const auto urlDecode = [](const std::string &SRC) -> std::string {
      std::string ret;
      char ch;
      int i, ii;
      for (i = 0; i < SRC.length(); i++) {
        if (SRC[i] == '%') {
          sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
          ch = static_cast<char>(ii);
          ret += ch;
          i = i + 2;
        } else {
          ret += SRC[i];
        }
      }
      return ret;
    };

    std::string path = "/mnt2/NAS/photos/DATA/2_shadow";
    if (type != "preview") {
      path = "/mnt2/NAS/photos/DATA/2";
    }

    std::cout << "type = " << type << " , img = " << urlDecode(img)
              << std::endl;
    return send_image(path, urlDecode(img));
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
