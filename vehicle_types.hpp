#pragma once

#include <cstdint>

#include "sol/sol.hpp"

namespace vehicle_types {

void locate(std::uintptr_t game_base_address);
void install_lua_overrides(sol::state_view lua);

void load_sbv(int vehicle_type_index, const char *model_name);
void clear_custom_slots();
void setup_new(int vehicle_type_index, int initial_wheel_flags,
               float wheel_radius, float wheel_mass);
void setup_object_weight(int vehicle_type_index);

} // namespace vehicle_types
