#include <cstdint>
#include <stdexcept>
#include <string>

#include "game_layout.hpp"
#include "item_types.hpp"
#include "sol/sol.hpp"
#include "udp.hpp"
#include "vehicle_types.hpp"

namespace {

void require_supported_game_version(sol::state_view lua) {
  sol::object server_object = lua["server"];
  if (!server_object.is<sol::userdata>()) {
    throw std::runtime_error("server version is unavailable");
  }

  sol::userdata server = server_object.as<sol::userdata>();
  const unsigned int version_major = server["versionMajor"];
  const unsigned int version_minor = server["versionMinor"];
  if (version_major != game_layout::supported_version_major ||
      version_minor != game_layout::supported_version_minor) {
    throw std::runtime_error(
        "unsupported game version " + std::to_string(version_major) + "." +
        std::to_string(version_minor) + "; expected " +
        std::to_string(game_layout::supported_version_major) + "." +
        std::to_string(game_layout::supported_version_minor));
  }
}

std::uintptr_t get_game_base_address(sol::state_view lua) {
  const std::uintptr_t game_base_address = lua["memory"]["getBaseAddress"]();
  if (game_base_address == 0) {
    throw std::runtime_error("game base address is unavailable");
  }
  return game_base_address;
}

sol::table open_library(sol::this_state state) {
  sol::state_view lua(state);
  require_supported_game_version(lua);

  const std::uintptr_t game_base_address = get_game_base_address(lua);
  item_types::locate(game_base_address);
  vehicle_types::locate(game_base_address);
  udp::locate(game_base_address);

  item_types::install_lua_overrides(lua);
  vehicle_types::install_lua_overrides(lua);

  sol::table library = lua.create_table();
  library["loadITM"] = &item_types::load_itm;
  library["loadIT3"] = &item_types::load_it3;
  library["loadSBV"] = &vehicle_types::load_sbv;
  library["clearCustomVehicleTypeSlots"] = &vehicle_types::clear_custom_slots;
  library["setupVehicleTypeNew"] = &vehicle_types::setup_new;
  library["setupObjectTypeWeight"] = &vehicle_types::setup_object_weight;
  library["randomToken"] = &udp::random_token;
  library["sendPacket"] = &udp::send_packet;
  library["drainSrcPackets"] = &udp::drain_src_packets;
  library["currentPacketEndpoint"] = &udp::current_packet_endpoint;

  lua["srcIntegrationNative"] = library;
  return library;
}

} // namespace

extern "C" __attribute__((visibility("default"))) int
luaopen_librosaserver_src_integration(lua_State *state) {
  return sol::stack::call_lua(state, 1, open_library);
}
