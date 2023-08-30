#include "database.h"
#include "thumbs.h"
#include "util.h"
#include "web.h"

#include <iostream>
#include <set>

#define CROW_JSON_USE_MAP
#include "crow.h"

int main() {
  database db;
  db.load();

  db.update_time_indexes();

  crow::SimpleApp app;
  crow::mustache::set_base(".");

  CROW_ROUTE(app, "/")
  ([] {
    crow::mustache::context ctx;
    return crow::mustache::load("index.html").render();
  });

  CROW_ROUTE(app, "/get_time_indexes")
  ([&] {
    db.update_time_indexes();
    crow::json::wvalue x(db.get_vec());
    return x;
  });

  CROW_ROUTE(app, "/get_tags")
  ([&] {
    std::vector<crow::json::wvalue> v;
    for (const auto &t : db.get_unique_tags()) {
      v.emplace_back(t);
    }
    crow::json::wvalue x(v);
    return x;
  });

  CROW_ROUTE(app, "/get_selected_tags")
  ([&] {
    std::vector<crow::json::wvalue> v;
    for (const auto &t : db.get_selected_tags()) {
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
        db.get_selected_tags().clear();
        for (const auto &tag : x["tags"]) {
          db.get_selected_tags().push_back(tag.s());
          std::cout << "got it: " << tag.s() << std::endl;
        }

        db.update_time_indexes();
        db.update_filtered();

        return crow::response(200, "Tags updated successfully");
      });

  CROW_ROUTE(app, "/num_images")
  ([&] {
    db.update_filtered();
    crow::json::wvalue x(db.get_filtered_files_sorted().size());
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

    while (index < db.get_filtered_files_sorted().size()) {
      const auto &img = db.get_filtered_files_sorted()[index];
      const auto s = db.get_files_lookup()[img.id];
      auto thumb_width = img.width;
      auto thumb_height = img.height;

      std::string orientation = std::to_string(img.id);
      auto pos = db.get_file_orientations().find(img.id);
      if (pos != db.get_file_orientations().end()) {
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

      const auto tags = db.get_tags_string(img.id);

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

    while (index >= 0 && index < db.get_filtered_files_sorted().size()) {
      const auto &img = db.get_filtered_files_sorted()[index];
      const auto s = db.get_files_lookup()[img.id];
      auto thumb_width = img.width;
      auto thumb_height = img.height;

      std::string orientation = std::to_string(img.id);
      auto pos = db.get_file_orientations().find(img.id);
      if (pos != db.get_file_orientations().end()) {
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

      const auto tags = db.get_tags_string(img.id);

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
