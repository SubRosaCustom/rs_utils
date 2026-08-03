#include "item_types.hpp"

#include <cstring>
#include <stdexcept>

#include "game_layout.hpp"
#include "structs.h"

namespace item_types {
namespace {

using LoadItemModelFunction = void (*)(int, char *);

ItemType *item_types = nullptr;
LoadItemModelFunction load_itm_function = nullptr;
LoadItemModelFunction load_it3_function = nullptr;

bool is_valid(const ItemType &item_type) { return item_type.mass > 0.0f; }

int get_count() {
  int count = static_cast<int>(game_layout::stock_item_type_count);
  for (std::size_t index = game_layout::stock_item_type_count;
       index <= game_layout::maximum_item_type_index; ++index) {
    if (is_valid(item_types[index])) {
      ++count;
    }
  }
  return count;
}

sol::table get_all(sol::this_state state) {
  sol::state_view lua(state);
  sol::table result = lua.create_table();
  for (std::size_t index = 0; index < game_layout::stock_item_type_count;
       ++index) {
    result.add(&item_types[index]);
  }
  for (std::size_t index = game_layout::stock_item_type_count;
       index <= game_layout::maximum_item_type_index; ++index) {
    if (is_valid(item_types[index])) {
      result.add(&item_types[index]);
    }
  }
  return result;
}

ItemType *get_by_name(const char *name) {
  if (name == nullptr) {
    return nullptr;
  }
  for (std::size_t index = 0; index <= game_layout::maximum_item_type_index;
       ++index) {
    if (index >= game_layout::stock_item_type_count &&
        !is_valid(item_types[index])) {
      continue;
    }
    if (std::strcmp(item_types[index].name, name) == 0) {
      return &item_types[index];
    }
  }
  return nullptr;
}

ItemType *get_by_index(sol::table, unsigned int index) {
  if (index > game_layout::maximum_item_type_index) {
    throw std::invalid_argument("Index out of range");
  }
  if (index >= game_layout::stock_item_type_count &&
      !is_valid(item_types[index])) {
    throw std::invalid_argument("Index out of range");
  }
  return &item_types[index];
}

} // namespace

void locate(std::uintptr_t game_base_address) {
  item_types = reinterpret_cast<ItemType *>(game_base_address +
                                            game_layout::item_types_offset);
  load_itm_function = reinterpret_cast<LoadItemModelFunction>(
      game_base_address + game_layout::load_itm_offset);
  load_it3_function = reinterpret_cast<LoadItemModelFunction>(
      game_base_address + game_layout::load_it3_offset);
}

void install_lua_overrides(sol::state_view lua) {
  sol::table lua_item_types = lua["itemTypes"];
  if (!lua_item_types.valid()) {
    throw std::runtime_error("itemTypes table is unavailable");
  }
  sol::table metatable = lua_item_types[sol::metatable_key];
  if (!metatable.valid()) {
    throw std::runtime_error("itemTypes metatable is unavailable");
  }

  lua_item_types["getCount"] = &get_count;
  lua_item_types["getAll"] = &get_all;
  lua_item_types["getByName"] = &get_by_name;
  metatable["__len"] = &get_count;
  metatable["__index"] = &get_by_index;
}

void load_itm(int item_type_index, const char *model_path) {
  if (item_type_index < 0 ||
      item_type_index >
          static_cast<int>(game_layout::maximum_item_type_index)) {
    throw std::invalid_argument("item type index out of range");
  }
  if (model_path == nullptr || *model_path == '\0') {
    throw std::invalid_argument("ITM path must be non-empty");
  }

  load_itm_function(item_type_index, const_cast<char *>(model_path));
}

void load_it3(int item_type_index, const char *model_path) {
  if (item_type_index < 0 ||
      item_type_index >
          static_cast<int>(game_layout::maximum_item_type_index)) {
    throw std::invalid_argument("item type index out of range");
  }
  if (model_path == nullptr || *model_path == '\0') {
    throw std::invalid_argument("IT3 path must be non-empty");
  }

  load_it3_function(item_type_index, const_cast<char *>(model_path));
}

} // namespace item_types
