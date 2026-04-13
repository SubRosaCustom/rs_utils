#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

#include "miniz.h"
#include "sol/sol.hpp"

namespace {

static constexpr unsigned int rsMaxNumberOfItemTypes = 46;
static constexpr unsigned int actualMaxNumberOfItemTypes = 255;

using padding = uint8_t;

#define STRINGFY(a, b) a##b
#define STRINGFY2(a, b) STRINGFY(a, b)
#define PAD(size) padding STRINGFY2(unk, __LINE__)[size];

struct Vector {
  float x, y, z;
};

struct ItemType {
  int unk0;
  int price;           // 04
  float mass;          // 08
  int canCollide;      // 0c
  int isGun;           // 10
  int isOneHanded;     // 14
  int fireRate;        // 18
  int bulletType;      // 1c
  int unk2;            // 20
  int magazineAmmo;    // 24
  float bulletVelocity;  // 28
  float bulletSpread;    // 2c
  char name[64];         // 30
  PAD(0x7c - 0x30 - 64);
  int numHands;         // 7c
  Vector rightHandPos;  // 80
  Vector leftHandPos;   // 8c
  PAD(0xb0 - 0x8c - 12);
  Vector primaryGripRotAxis;    // b0
  float primaryGripRotation;    // bc
  Vector secondaryGripRotAxis;  // c0
  float secondaryGripRotation;  // cc
  PAD(0x104 - 0xcc - 4);
  Vector boundsCenter;  // 104
  PAD(0x11c - 0x104 - 12);
  int canMountTo[rsMaxNumberOfItemTypes];  // 11c
  PAD(0x1394 - 0x11c - (4 * rsMaxNumberOfItemTypes));
  Vector gunHoldingPos;  // 1394
  PAD(0x13D0 - 0x1394 - 12);
};

#undef PAD
#undef STRINGFY2
#undef STRINGFY

bool isValidItemType(const ItemType& item_type) {
  return item_type.mass > 0.0f;
}

ItemType* getItemTypeBase(sol::state_view lua) {
  sol::table item_types = lua["itemTypes"];
  if (!item_types.valid()) {
    throw std::runtime_error("itemTypes table is unavailable");
  }

  sol::table meta = item_types[sol::metatable_key];
  if (!meta.valid()) {
    throw std::runtime_error("itemTypes metatable is unavailable");
  }

  sol::protected_function index_fn = meta["__index"];
  if (!index_fn.valid()) {
    throw std::runtime_error("itemTypes.__index is unavailable");
  }

  sol::protected_function_result result = index_fn(item_types, 0u);
  if (!result.valid()) {
    sol::error err = result;
    throw std::runtime_error(err.what());
  }

  return result.get<ItemType*>();
}

int getItemTypeCount(ItemType* base) {
  int count = static_cast<int>(rsMaxNumberOfItemTypes);
  for (unsigned int i = rsMaxNumberOfItemTypes; i <= actualMaxNumberOfItemTypes; ++i) {
    if (isValidItemType(base[i])) {
      ++count;
    }
  }
  return count;
}

sol::table getAllItemTypes(sol::this_state state, ItemType* base) {
  sol::state_view lua(state);
  sol::table arr = lua.create_table();
  for (unsigned int i = 0; i < rsMaxNumberOfItemTypes; ++i) {
    arr.add(&base[i]);
  }

  for (unsigned int i = rsMaxNumberOfItemTypes; i <= actualMaxNumberOfItemTypes; ++i) {
    if (isValidItemType(base[i])) {
      arr.add(&base[i]);
    }
  }
  return arr;
}

std::runtime_error zipError(const std::string& message, mz_zip_archive* archive) {
  return std::runtime_error(message + ": " +
                            std::string(mz_zip_get_error_string(
                                mz_zip_get_last_error(archive))));
}

std::string createZipArchive(sol::table files) {
  mz_zip_archive archive {};
  if (!mz_zip_writer_init_heap(&archive, 0, 0)) {
    throw zipError("mz_zip_writer_init_heap failed", &archive);
  }

  try {
    for (const auto& pair : files) {
      const std::string name = pair.first.as<std::string>();
      const std::string data = pair.second.as<std::string>();

      if (!mz_zip_writer_add_mem(&archive, name.c_str(), data.data(), data.size(),
                                 MZ_BEST_COMPRESSION)) {
        throw zipError("mz_zip_writer_add_mem failed", &archive);
      }
    }

    void* output_ptr = nullptr;
    size_t output_size = 0;
    if (!mz_zip_writer_finalize_heap_archive(&archive, &output_ptr, &output_size)) {
      throw zipError("mz_zip_writer_finalize_heap_archive failed", &archive);
    }

    std::unique_ptr<void, decltype(&mz_free)> output_holder(output_ptr, &mz_free);
    std::string output(static_cast<const char*>(output_ptr), output_size);
    mz_zip_writer_end(&archive);
    return output;
  } catch (...) {
    mz_zip_writer_end(&archive);
    throw;
  }
}

sol::table extractZipArchive(sol::this_state state, const std::string& archive_data) {
  sol::state_view lua(state);
  mz_zip_archive archive {};
  if (!mz_zip_reader_init_mem(&archive, archive_data.data(), archive_data.size(), 0)) {
    throw zipError("mz_zip_reader_init_mem failed", &archive);
  }

  sol::table files = lua.create_table();

  try {
    const mz_uint count = mz_zip_reader_get_num_files(&archive);
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
    }

    mz_zip_reader_end(&archive);
    return files;
  } catch (...) {
    mz_zip_reader_end(&archive);
    throw;
  }
}

sol::table openLibrary(sol::this_state state) {
  sol::state_view lua(state);
  ItemType* base = getItemTypeBase(lua);
  if (base == nullptr) {
    throw std::runtime_error("itemTypes base pointer is null");
  }

  sol::table item_types = lua["itemTypes"];
  sol::table meta = item_types[sol::metatable_key];

  item_types["getCount"] = sol::as_function([base]() -> int {
    return getItemTypeCount(base);
  });

  item_types["getAll"] = sol::as_function([base](sol::this_state current_state) -> sol::table {
    return getAllItemTypes(current_state, base);
  });

  item_types["getByName"] = sol::as_function([base](const char* name) -> ItemType* {
    if (name == nullptr) {
      return nullptr;
    }

    for (unsigned int i = 0; i <= actualMaxNumberOfItemTypes; ++i) {
      if (isValidItemType(base[i]) && std::strcmp(base[i].name, name) == 0) {
        return &base[i];
      }
    }

    return nullptr;
  });

  meta["__len"] = sol::as_function([base]() -> int {
    return getItemTypeCount(base);
  });

  meta["__index"] = sol::as_function([base](sol::table, unsigned int idx) -> ItemType* {
    if (idx > actualMaxNumberOfItemTypes) {
      throw std::invalid_argument("Index out of range");
    }

    if (idx >= rsMaxNumberOfItemTypes && !isValidItemType(base[idx])) {
      throw std::invalid_argument("Index out of range");
    }

    return &base[idx];
  });

  sol::table library = lua.create_table();
  library["getCount"] = sol::as_function([base]() -> int {
    return getItemTypeCount(base);
  });
  library["createZip"] = sol::as_function([](sol::table files) -> std::string {
    return createZipArchive(files);
  });
  library["extractZip"] = sol::as_function([](sol::this_state current_state,
                                              const std::string& archive_data) -> sol::table {
    return extractZipArchive(current_state, archive_data);
  });
  return library;
}

}  // namespace

extern "C" __attribute__((visibility("default"))) int
luaopen_librosaserver_src_integration(lua_State* state) {
  return sol::stack::call_lua(state, 1, openLibrary);
}
