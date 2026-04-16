#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "sol/sol.hpp"
#include "structs.h"

namespace {

static constexpr unsigned int rsMaxNumberOfItemTypes = 46;
static constexpr unsigned int actualMaxNumberOfItemTypes = 255;

ItemType* itemTypes = nullptr;

bool isValidItemType(const ItemType& item_type) {
  return item_type.mass > 0.0f;
}

int getItemTypeCount() {
  int count = static_cast<int>(rsMaxNumberOfItemTypes);
  for (unsigned int i = rsMaxNumberOfItemTypes; i <= actualMaxNumberOfItemTypes; ++i) {
    if (isValidItemType(itemTypes[i])) {
      ++count;
    }
  }
  return count;
}

sol::table getAllItemTypes(sol::this_state state) {
  sol::state_view lua(state);
  sol::table arr = lua.create_table();
  for (unsigned int i = 0; i < rsMaxNumberOfItemTypes; ++i) {
    arr.add(&itemTypes[i]);
  }
  for (unsigned int i = rsMaxNumberOfItemTypes; i <= actualMaxNumberOfItemTypes; ++i) {
    if (isValidItemType(itemTypes[i])) {
      arr.add(&itemTypes[i]);
    }
  }
  return arr;
}

ItemType* getItemTypeByName(const char* name) {
  if (name == nullptr) {
    return nullptr;
  }
  for (unsigned int i = 0; i <= actualMaxNumberOfItemTypes; ++i) {
    if (i >= rsMaxNumberOfItemTypes && !isValidItemType(itemTypes[i])) {
      continue;
    }
    if (std::strcmp(itemTypes[i].name, name) == 0) {
      return &itemTypes[i];
    }
  }
  return nullptr;
}

ItemType* itemTypesIndex(sol::table, unsigned int idx) {
  if (idx > actualMaxNumberOfItemTypes) {
    throw std::invalid_argument("Index out of range");
  }
  if (idx >= rsMaxNumberOfItemTypes && !isValidItemType(itemTypes[idx])) {
    throw std::invalid_argument("Index out of range");
  }
  return &itemTypes[idx];
}

sol::table openLibrary(sol::this_state state) {
  sol::state_view lua(state);

  const uintptr_t base_address = lua["memory"]["getBaseAddress"]();
  itemTypes = reinterpret_cast<ItemType*>(base_address + 0x5a60d7c0);

  sol::table lua_item_types = lua["itemTypes"];
  if (!lua_item_types.valid()) {
    throw std::runtime_error("itemTypes table is unavailable");
  }
  sol::table meta = lua_item_types[sol::metatable_key];
  if (!meta.valid()) {
    throw std::runtime_error("itemTypes metatable is unavailable");
  }

  lua_item_types["getCount"] = &getItemTypeCount;
  lua_item_types["getAll"] = &getAllItemTypes;
  lua_item_types["getByName"] = &getItemTypeByName;

  meta["__len"] = &getItemTypeCount;
  meta["__index"] = &itemTypesIndex;

  sol::table library = lua.create_table();
  library["getCount"] = &getItemTypeCount;
  return library;
}

}  // namespace

extern "C" __attribute__((visibility("default"))) int
luaopen_librosaserver_src_integration(lua_State* state) {
  return sol::stack::call_lua(state, 1, openLibrary);
}
