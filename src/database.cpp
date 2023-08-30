#include "database.h"
#include "util.h"
#include <fstream>

#include <regex>

void database::load() {
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
}

void database::load_all_files() {
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

void database::load_file_dimensions() {
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

void database::load_file_orientations() {
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

void database::load_tags() {
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

std::string database::get_tags_string(uint64_t id) {
  std::stringstream ss;
  auto pos = file_tags.find(id);
  if (pos != file_tags.end()) {
    for (auto &t : pos->second) {
      ss << t << ";";
    }
  }
  return ss.str();
}

std::vector<crow::json::wvalue> database::get_vec() {
  std::unique_lock<std::mutex> lock(mut);
  return vec;
}

void database::update_time_indexes() {
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
}

void database::update_filtered() {
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
}

std::set<std::string> &database::get_unique_tags() { return unique_tags; }

std::vector<std::string> &database::get_selected_tags() {
  return selected_tags;
}

std::vector<file> &database::get_filtered_files_sorted() {
  return filtered_files_sorted;
}

std::map<uint64_t, std::string> &database::get_files_lookup() {
  return files_lookup;
}

std::map<uint64_t, std::string> &database::get_file_orientations() {
  return file_orientations;
}
