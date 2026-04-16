#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

#include "miniz.h"
#include "sol/sol.hpp"

namespace {

std::runtime_error zipError(const std::string& message, mz_zip_archive* archive) {
  return std::runtime_error(message + ": " +
                            std::string(mz_zip_get_error_string(
                                mz_zip_get_last_error(archive))));
}

std::string createZipArchive(sol::table files) {
  std::fprintf(stderr, "[libminiz] createZipArchive called\n");

  mz_zip_archive archive {};
  if (!mz_zip_writer_init_heap(&archive, 0, 0)) {
    throw zipError("mz_zip_writer_init_heap failed", &archive);
  }

  try {
    size_t entry_count = 0;
    for (const auto& pair : files) {
      const std::string name = pair.first.as<std::string>();
      const std::string data = pair.second.as<std::string>();

      std::fprintf(stderr, "[libminiz] adding %s (%zu bytes)\n",
                   name.c_str(), data.size());

      if (!mz_zip_writer_add_mem(&archive, name.c_str(), data.data(), data.size(),
                                 MZ_BEST_COMPRESSION)) {
        throw zipError("mz_zip_writer_add_mem failed", &archive);
      }
      ++entry_count;
    }

    void* output_ptr = nullptr;
    size_t output_size = 0;
    if (!mz_zip_writer_finalize_heap_archive(&archive, &output_ptr, &output_size)) {
      throw zipError("mz_zip_writer_finalize_heap_archive failed", &archive);
    }

    std::unique_ptr<void, decltype(&mz_free)> output_holder(output_ptr, &mz_free);
    std::string output(static_cast<const char*>(output_ptr), output_size);
    mz_zip_writer_end(&archive);

    std::fprintf(stderr, "[libminiz] createZipArchive wrote %zu entries, %zu bytes\n",
                 entry_count, output.size());
    return output;
  } catch (...) {
    mz_zip_writer_end(&archive);
    throw;
  }
}

sol::table extractZipArchive(sol::this_state state, const std::string& archive_data) {
  std::fprintf(stderr, "[libminiz] extractZipArchive called (%zu bytes)\n",
               archive_data.size());

  sol::state_view lua(state);
  mz_zip_archive archive {};
  if (!mz_zip_reader_init_mem(&archive, archive_data.data(), archive_data.size(), 0)) {
    throw zipError("mz_zip_reader_init_mem failed", &archive);
  }

  sol::table files = lua.create_table();

  try {
    const mz_uint count = mz_zip_reader_get_num_files(&archive);
    std::fprintf(stderr, "[libminiz] archive contains %u files\n", count);

    for (mz_uint i = 0; i < count; ++i) {
      mz_zip_archive_file_stat stat {};
      if (!mz_zip_reader_file_stat(&archive, i, &stat)) {
        throw zipError("mz_zip_reader_file_stat failed", &archive);
      }

      if (mz_zip_reader_is_file_a_directory(&archive, i)) {
        continue;
      }

      size_t extracted_size = 0;
      void* extracted = mz_zip_reader_extract_to_heap(&archive, i, &extracted_size, 0);
      if (extracted == nullptr) {
        throw zipError("mz_zip_reader_extract_to_heap failed", &archive);
      }

      std::unique_ptr<void, decltype(&mz_free)> extracted_holder(extracted, &mz_free);
      files[stat.m_filename] =
          std::string(static_cast<const char*>(extracted), extracted_size);

      std::fprintf(stderr, "[libminiz] extracted %s (%zu bytes)\n",
                   stat.m_filename, extracted_size);
    }

    mz_zip_reader_end(&archive);
    return files;
  } catch (...) {
    mz_zip_reader_end(&archive);
    throw;
  }
}

sol::table openLibrary(sol::this_state state) {
  std::fprintf(stderr, "[libminiz] openLibrary called\n");

  sol::state_view lua(state);
  sol::table library = lua.create_table();
  library["createZip"] = &createZipArchive;
  library["extractZip"] = &extractZipArchive;

  lua["_G"]["miniz"] = library;

  std::fprintf(stderr, "[libminiz] registered miniz global\n");
  return library;
}

}  // namespace

extern "C" __attribute__((visibility("default"))) int
luaopen_libminiz(lua_State* state) {
  std::fprintf(stderr, "[libminiz] luaopen_libminiz entry\n");
  return sol::stack::call_lua(state, 1, openLibrary);
}
