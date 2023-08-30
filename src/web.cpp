#include "web.h"

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
