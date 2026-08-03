#pragma once

#include <cstddef>
#include <cstdint>

namespace game_layout {

inline constexpr unsigned int supported_version_major = 38;
inline constexpr unsigned int supported_version_minor = 4;

inline constexpr std::size_t stock_item_type_count = 46;
inline constexpr std::size_t maximum_item_type_index = 255;
inline constexpr std::uintptr_t item_types_offset = 0x5a60d7c0;
inline constexpr std::uintptr_t load_itm_offset = 0x41740;
inline constexpr std::uintptr_t load_it3_offset = 0x42c40;

inline constexpr std::size_t stock_vehicle_type_count = 17;
inline constexpr std::size_t maximum_vehicle_type_index = 127;
inline constexpr std::size_t vehicle_type_size = 0x185c0;
inline constexpr std::uintptr_t vehicle_types_offset = 0x4d03560;
inline constexpr std::uintptr_t load_sbv_offset = 0xaf0e0;
inline constexpr std::uintptr_t setup_vehicle_type_offset = 0xac890;
inline constexpr std::uintptr_t setup_object_weight_offset = 0xabec0;

inline constexpr std::uintptr_t game_socket_index_offset = 0x39075c20;
inline constexpr std::uintptr_t game_socket_descriptors_offset = 0x39075c44;
inline constexpr std::uintptr_t current_packet_address_offset = 0x39085c84;
inline constexpr std::uintptr_t current_packet_port_offset = 0x39085c88;

} // namespace game_layout
