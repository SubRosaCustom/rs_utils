#include <cstdint>
#include <cstdio>
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

// Resolved once in openLibrary, then used by all registered free functions.
// Safe because RosaServer hosts a single Lua state in-process.
ItemType* g_itemTypeBase = nullptr;

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

int getItemTypeCount() {
  int count = static_cast<int>(rsMaxNumberOfItemTypes);
  for (unsigned int i = rsMaxNumberOfItemTypes; i <= actualMaxNumberOfItemTypes; ++i) {
    if (isValidItemType(g_itemTypeBase[i])) {
      ++count;
    }
  }
  return count;
}

sol::table getAllItemTypes(sol::this_state state) {
  sol::state_view lua(state);
  sol::table arr = lua.create_table();
  for (unsigned int i = 0; i < rsMaxNumberOfItemTypes; ++i) {
    arr.add(&g_itemTypeBase[i]);
  }

  for (unsigned int i = rsMaxNumberOfItemTypes; i <= actualMaxNumberOfItemTypes; ++i) {
    if (isValidItemType(g_itemTypeBase[i])) {
      arr.add(&g_itemTypeBase[i]);
    }
  }
  return arr;
}

ItemType* getItemTypeByName(const char* name) {
  if (name == nullptr) {
    return nullptr;
  }

  for (unsigned int i = 0; i <= actualMaxNumberOfItemTypes; ++i) {
    if (isValidItemType(g_itemTypeBase[i]) &&
        std::strcmp(g_itemTypeBase[i].name, name) == 0) {
      return &g_itemTypeBase[i];
    }
  }

  return nullptr;
}

ItemType* itemTypesIndex(sol::table, unsigned int idx) {
  if (idx > actualMaxNumberOfItemTypes) {
    throw std::invalid_argument("Index out of range");
  }

  if (idx >= rsMaxNumberOfItemTypes && !isValidItemType(g_itemTypeBase[idx])) {
    throw std::invalid_argument("Index out of range");
  }

  return &g_itemTypeBase[idx];
}

sol::table openLibrary(sol::this_state state) {
  std::fprintf(stderr, "[rs_integration] openLibrary called\n");

  sol::state_view lua(state);
  g_itemTypeBase = getItemTypeBase(lua);
  if (g_itemTypeBase == nullptr) {
    throw std::runtime_error("itemTypes base pointer is null");
  }

  std::fprintf(stderr, "[rs_integration] itemTypes base resolved to %p\n",
               static_cast<void*>(g_itemTypeBase));

  sol::table item_types = lua["itemTypes"];
  sol::table meta = item_types[sol::metatable_key];

  item_types["getCount"] = &getItemTypeCount;
  item_types["getAll"] = &getAllItemTypes;
  item_types["getByName"] = &getItemTypeByName;

  meta["__len"] = &getItemTypeCount;
  meta["__index"] = &itemTypesIndex;

  const int initial_count = getItemTypeCount();
  std::fprintf(stderr, "[rs_integration] installed overrides, initial count = %d\n",
               initial_count);

  sol::table library = lua.create_table();
  library["getCount"] = &getItemTypeCount;
  return library;
}

}  // namespace

extern "C" __attribute__((visibility("default"))) int
luaopen_librosaserver_src_integration(lua_State* state) {
  std::fprintf(stderr, "[rs_integration] luaopen_librosaserver_src_integration entry\n");
  return sol::stack::call_lua(state, 1, openLibrary);
}
