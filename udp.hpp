#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "sol/sol.hpp"

namespace udp {

void locate(std::uintptr_t game_base_address);

std::string random_token();
int send_packet(std::string_view address, int port, std::string_view bytes);
sol::table drain_src_packets(sol::this_state state);
sol::table current_packet_endpoint(sol::this_state state);

} // namespace udp
