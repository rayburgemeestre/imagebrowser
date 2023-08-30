#include <string>

#define CROW_JSON_USE_MAP
#include "crow.h"

std::string get_content_type(const std::string &extension);

crow::response send_image(const std::string &file_path,
                          const std::string &img_name);
