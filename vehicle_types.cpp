#include "vehicle_types.hpp"

#include <cstring>
#include <stdexcept>

#include "game_layout.hpp"
#include "structs.h"

namespace vehicle_types {
namespace {

static_assert(sizeof(VehicleType) == game_layout::vehicle_type_size);

using LoadSbvFunction = void (*)(int, const char *);
using SetupVehicleTypeFunction = void (*)(int, float, float, int);
using SetupObjectWeightFunction = void (*)(int);

VehicleType *vehicle_types = nullptr;
LoadSbvFunction load_sbv_function = nullptr;
SetupVehicleTypeFunction setup_vehicle_type_function = nullptr;
SetupObjectWeightFunction setup_object_weight_function = nullptr;

bool is_valid(const VehicleType &vehicle_type) {
  return vehicle_type.mass > 0.0f;
}

int get_count() {
  int count = static_cast<int>(game_layout::stock_vehicle_type_count);
  for (std::size_t index = game_layout::stock_vehicle_type_count;
       index <= game_layout::maximum_vehicle_type_index; ++index) {
    if (is_valid(vehicle_types[index])) {
      ++count;
    }
  }
  return count;
}

sol::table get_all(sol::this_state state) {
  sol::state_view lua(state);
  sol::table result = lua.create_table();
  for (std::size_t index = 0; index < game_layout::stock_vehicle_type_count;
       ++index) {
    result.add(&vehicle_types[index]);
  }
  for (std::size_t index = game_layout::stock_vehicle_type_count;
       index <= game_layout::maximum_vehicle_type_index; ++index) {
    if (is_valid(vehicle_types[index])) {
      result.add(&vehicle_types[index]);
    }
  }
  return result;
}

VehicleType *get_by_name(const char *name) {
  if (name == nullptr) {
    return nullptr;
  }
  for (std::size_t index = 0; index <= game_layout::maximum_vehicle_type_index;
       ++index) {
    if (index >= game_layout::stock_vehicle_type_count &&
        !is_valid(vehicle_types[index])) {
      continue;
    }
    if (std::strcmp(vehicle_types[index].name, name) == 0) {
      return &vehicle_types[index];
    }
  }
  return nullptr;
}

VehicleType *get_by_index(sol::table, unsigned int index) {
  if (index > game_layout::maximum_vehicle_type_index) {
    throw std::invalid_argument("Index out of range");
  }
  if (index >= game_layout::stock_vehicle_type_count &&
      !is_valid(vehicle_types[index])) {
    throw std::invalid_argument("Index out of range");
  }
  return &vehicle_types[index];
}

void require_valid_index(int vehicle_type_index) {
  if (vehicle_type_index < 0 ||
      vehicle_type_index >
          static_cast<int>(game_layout::maximum_vehicle_type_index)) {
    throw std::invalid_argument("vehicle type index out of range");
  }
}

} // namespace

void locate(std::uintptr_t game_base_address) {
  vehicle_types = reinterpret_cast<VehicleType *>(
      game_base_address + game_layout::vehicle_types_offset);
  load_sbv_function = reinterpret_cast<LoadSbvFunction>(
      game_base_address + game_layout::load_sbv_offset);
  setup_vehicle_type_function = reinterpret_cast<SetupVehicleTypeFunction>(
      game_base_address + game_layout::setup_vehicle_type_offset);
  setup_object_weight_function = reinterpret_cast<SetupObjectWeightFunction>(
      game_base_address + game_layout::setup_object_weight_offset);
}

void install_lua_overrides(sol::state_view lua) {
  sol::table lua_vehicle_types = lua["vehicleTypes"];
  if (!lua_vehicle_types.valid()) {
    throw std::runtime_error("vehicleTypes table is unavailable");
  }
  sol::table metatable = lua_vehicle_types[sol::metatable_key];
  if (!metatable.valid()) {
    throw std::runtime_error("vehicleTypes metatable is unavailable");
  }

  lua_vehicle_types["getCount"] = &get_count;
  lua_vehicle_types["getAll"] = &get_all;
  lua_vehicle_types["getByName"] = &get_by_name;
  metatable["__len"] = &get_count;
  metatable["__index"] = &get_by_index;
}

void load_sbv(int vehicle_type_index, const char *model_name) {
  require_valid_index(vehicle_type_index);
  if (model_name == nullptr || *model_name == '\0') {
    throw std::invalid_argument("model name must be non-empty");
  }

  load_sbv_function(vehicle_type_index, model_name);
}

void clear_custom_slots() {
  for (std::size_t index = game_layout::stock_vehicle_type_count;
       index <= game_layout::maximum_vehicle_type_index; ++index) {
    std::memset(&vehicle_types[index], 0, sizeof(VehicleType));
  }
}

void setup_new(int vehicle_type_index, int initial_wheel_flags,
               float wheel_radius, float wheel_mass) {
  require_valid_index(vehicle_type_index);
  setup_vehicle_type_function(vehicle_type_index, wheel_radius, wheel_mass,
                              initial_wheel_flags);
}

void setup_object_weight(int vehicle_type_index) {
  require_valid_index(vehicle_type_index);
  setup_object_weight_function(vehicle_type_index);
}

} // namespace vehicle_types
