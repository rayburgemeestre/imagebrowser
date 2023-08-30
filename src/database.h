
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#define CROW_JSON_USE_MAP
#include "crow.h"

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

class database {
public:
  void load();

private:
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

  std::vector<file> all_files_sorted;
  std::vector<file> filtered_files_sorted;
  std::map<uint64_t, uint64_t> file_idx_lookup;

  void load_all_files();
  void load_file_dimensions();
  void load_file_orientations();
  void load_tags();

private:
  std::vector<crow::json::wvalue> vec;
  std::mutex mut;

public:
  std::string get_tags_string(uint64_t id);
  std::vector<crow::json::wvalue> get_vec();
  void update_time_indexes();
  void update_filtered();
  std::set<std::string> &get_unique_tags();
  std::vector<std::string> &get_selected_tags();
  std::vector<file> &get_filtered_files_sorted();
  std::map<uint64_t, std::string> &get_files_lookup();
  std::map<uint64_t, std::string> &get_file_orientations();
};
