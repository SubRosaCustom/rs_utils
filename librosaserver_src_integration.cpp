#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "sol/sol.hpp"
#include "structs.h"

namespace {

static constexpr unsigned int rsMaxNumberOfItemTypes = 46;
static constexpr unsigned int actualMaxNumberOfItemTypes = 255;
static constexpr unsigned int rsMaxNumberOfVehicleTypes = 17;
static constexpr unsigned int actualMaxNumberOfVehicleTypes = 127;

ItemType *itemTypes = nullptr;
VehicleType *vehicleTypes = nullptr;

using LoadSBVFunction = void (*)(int, const char *);
using LoadItemModelFunction = void (*)(int, char *);
using SetupVehicleTypeNewFunction = void (*)(int, float, float, int);
using SetupObjectTypeWeightFunction = void (*)(int);

LoadSBVFunction loadSBVFn = nullptr;
LoadItemModelFunction loadITMFn = nullptr;
LoadItemModelFunction loadIT3Fn = nullptr;
SetupVehicleTypeNewFunction setupVehicleTypeNewFn = nullptr;
SetupObjectTypeWeightFunction setupObjectTypeWeightFn = nullptr;

bool isValidItemType(const ItemType &item_type) {
  return item_type.mass > 0.0f;
}

bool isValidVehicleType(const VehicleType &vehicle_type) {
  return vehicle_type.mass > 0.0f;
}

int getItemTypeCount() {
  int count = static_cast<int>(rsMaxNumberOfItemTypes);
  for (unsigned int i = rsMaxNumberOfItemTypes; i <= actualMaxNumberOfItemTypes;
       ++i) {
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
  for (unsigned int i = rsMaxNumberOfItemTypes; i <= actualMaxNumberOfItemTypes;
       ++i) {
    if (isValidItemType(itemTypes[i])) {
      arr.add(&itemTypes[i]);
    }
  }
  return arr;
}

ItemType *getItemTypeByName(const char *name) {
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

ItemType *itemTypesIndex(sol::table, unsigned int idx) {
  if (idx > actualMaxNumberOfItemTypes) {
    throw std::invalid_argument("Index out of range");
  }
  if (idx >= rsMaxNumberOfItemTypes && !isValidItemType(itemTypes[idx])) {
    throw std::invalid_argument("Index out of range");
  }
  return &itemTypes[idx];
}

int getVehicleTypeCount() {
  int count = static_cast<int>(rsMaxNumberOfVehicleTypes);
  for (unsigned int i = rsMaxNumberOfVehicleTypes;
       i <= actualMaxNumberOfVehicleTypes; ++i) {
    if (isValidVehicleType(vehicleTypes[i])) {
      ++count;
    }
  }
  return count;
}

sol::table getAllVehicleTypes(sol::this_state state) {
  sol::state_view lua(state);
  sol::table arr = lua.create_table();
  for (unsigned int i = 0; i < rsMaxNumberOfVehicleTypes; ++i) {
    arr.add(&vehicleTypes[i]);
  }
  for (unsigned int i = rsMaxNumberOfVehicleTypes;
       i <= actualMaxNumberOfVehicleTypes; ++i) {
    if (isValidVehicleType(vehicleTypes[i])) {
      arr.add(&vehicleTypes[i]);
    }
  }
  return arr;
}

VehicleType *getVehicleTypeByName(const char *name) {
  if (name == nullptr) {
    return nullptr;
  }
  for (unsigned int i = 0; i <= actualMaxNumberOfVehicleTypes; ++i) {
    if (i >= rsMaxNumberOfVehicleTypes &&
        !isValidVehicleType(vehicleTypes[i])) {
      continue;
    }
    if (std::strcmp(vehicleTypes[i].name, name) == 0) {
      return &vehicleTypes[i];
    }
  }
  return nullptr;
}

VehicleType *vehicleTypesIndex(sol::table, unsigned int idx) {
  if (idx > actualMaxNumberOfVehicleTypes) {
    throw std::invalid_argument("Index out of range");
  }
  if (idx >= rsMaxNumberOfVehicleTypes &&
      !isValidVehicleType(vehicleTypes[idx])) {
    throw std::invalid_argument("Index out of range");
  }
  return &vehicleTypes[idx];
}

void loadSBV(int vehicle_type_index, const char *model_name) {
  const int normalized_index = static_cast<int>(vehicle_type_index);
  if (normalized_index < 0 ||
      normalized_index > static_cast<int>(actualMaxNumberOfVehicleTypes)) {
    throw std::invalid_argument("vehicle type index out of range");
  }
  if (model_name == nullptr || *model_name == '\0') {
    throw std::invalid_argument("model name must be non-empty");
  }

  loadSBVFn(normalized_index, model_name);
}

void loadITM(int item_type_index, const char *model_path) {
  if (item_type_index < 0 ||
      item_type_index > static_cast<int>(actualMaxNumberOfItemTypes)) {
    throw std::invalid_argument("item type index out of range");
  }
  if (model_path == nullptr || *model_path == '\0') {
    throw std::invalid_argument("ITM path must be non-empty");
  }

  loadITMFn(item_type_index, const_cast<char *>(model_path));
}

void loadIT3(int item_type_index, const char *model_path) {
  if (item_type_index < 0 ||
      item_type_index > static_cast<int>(actualMaxNumberOfItemTypes)) {
    throw std::invalid_argument("item type index out of range");
  }
  if (model_path == nullptr || *model_path == '\0') {
    throw std::invalid_argument("IT3 path must be non-empty");
  }

  loadIT3Fn(item_type_index, const_cast<char *>(model_path));
}

void setupVehicleTypeNew(int vehicle_type_index, int initial_wheel_flags,
                         float wheel_radius, float wheel_mass) {
  const int normalized_index = static_cast<int>(vehicle_type_index);
  if (normalized_index < 0 ||
      normalized_index > static_cast<int>(actualMaxNumberOfVehicleTypes)) {
    throw std::invalid_argument("vehicle type index out of range");
  }

  setupVehicleTypeNewFn(normalized_index, wheel_radius, wheel_mass,
                        initial_wheel_flags);
}

void setupObjectTypeWeight(int vehicle_type_index) {
  const int normalized_index = static_cast<int>(vehicle_type_index);
  if (normalized_index < 0 ||
      normalized_index > static_cast<int>(actualMaxNumberOfVehicleTypes)) {
    throw std::invalid_argument("vehicle type index out of range");
  }

  setupObjectTypeWeightFn(normalized_index);
}

sol::table openLibrary(sol::this_state state) {
  sol::state_view lua(state);

  const uintptr_t base_address = lua["memory"]["getBaseAddress"]();
  itemTypes = reinterpret_cast<ItemType *>(base_address + 0x5a60d7c0);
  vehicleTypes = reinterpret_cast<VehicleType *>(base_address + 0x4d03560);
  loadITMFn = reinterpret_cast<LoadItemModelFunction>(base_address + 0x41740);
  loadIT3Fn = reinterpret_cast<LoadItemModelFunction>(base_address + 0x42c40);
  loadSBVFn = reinterpret_cast<LoadSBVFunction>(base_address + 0xaf0e0);
  setupVehicleTypeNewFn = reinterpret_cast<SetupVehicleTypeNewFunction>(
      base_address + 0xac890);
  setupObjectTypeWeightFn = reinterpret_cast<SetupObjectTypeWeightFunction>(
      base_address + 0xabec0);

  {
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
  }

  {
    sol::table lua_vehicle_types = lua["vehicleTypes"];
    if (!lua_vehicle_types.valid()) {
      throw std::runtime_error("vehicleTypes table is unavailable");
    }
    sol::table meta = lua_vehicle_types[sol::metatable_key];
    if (!meta.valid()) {
      throw std::runtime_error("vehicleTypes metatable is unavailable");
    }

    lua_vehicle_types["getCount"] = &getVehicleTypeCount;
    lua_vehicle_types["getAll"] = &getAllVehicleTypes;
    lua_vehicle_types["getByName"] = &getVehicleTypeByName;

    meta["__len"] = &getVehicleTypeCount;
    meta["__index"] = &vehicleTypesIndex;
  }

  sol::table library = lua.create_table();
  library["loadITM"] = &loadITM;
  library["loadIT3"] = &loadIT3;
  library["loadSBV"] = &loadSBV;
  library["setupVehicleTypeNew"] = &setupVehicleTypeNew;
  library["setupObjectTypeWeight"] = &setupObjectTypeWeight;
  lua["srcIntegrationNative"] = library;
  return library;
}

} // namespace

extern "C" __attribute__((visibility("default"))) int
luaopen_librosaserver_src_integration(lua_State *state) {
  return sol::stack::call_lua(state, 1, openLibrary);
}
