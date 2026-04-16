#include <cstdint>
#include <cstring>
#include <stdexcept>

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
  int price;             // 04
  float mass;            // 08
  int canCollide;        // 0c
  int isGun;             // 10
  int isOneHanded;       // 14
  int fireRate;          // 18
  int bulletType;        // 1c
  int unk2;              // 20
  int magazineAmmo;      // 24
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

ItemType* itemTypes = nullptr;
sol::protected_function originalGetAddress;

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

uintptr_t getAddress(sol::object obj) {
  ItemType* ptr = obj.as<ItemType*>();
  if (ptr != nullptr) {
    return reinterpret_cast<uintptr_t>(ptr);
  }
  sol::protected_function_result result = originalGetAddress(obj);
  if (!result.valid()) {
    sol::error err = result;
    throw std::runtime_error(std::string("memory.getAddress failed: ") + err.what());
  }
  return result.get<uintptr_t>();
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

  sol::table lua_memory = lua["memory"];
  if (!lua_memory.valid()) {
    throw std::runtime_error("memory table is unavailable");
  }

  originalGetAddress = lua_memory["getAddress"];
  lua_memory["getAddress"] = &getAddress;

  sol::table library = lua.create_table();
  library["getCount"] = &getItemTypeCount;
  return library;
}

}  // namespace

extern "C" __attribute__((visibility("default"))) int
luaopen_librosaserver_src_integration(lua_State* state) {
  return sol::stack::call_lua(state, 1, openLibrary);
}
