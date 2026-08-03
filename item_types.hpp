#pragma once

#include <cstdint>

#include "sol/sol.hpp"

namespace item_types {

void locate(std::uintptr_t game_base_address);
void install_lua_overrides(sol::state_view lua);

void load_itm(int item_type_index, const char *model_path);
void load_it3(int item_type_index, const char *model_path);

} // namespace item_types
