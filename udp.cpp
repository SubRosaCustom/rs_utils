#include "udp.hpp"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <random>
#include <stdexcept>
#include <sys/socket.h>

#include "game_layout.hpp"

namespace udp {
namespace {

std::uintptr_t game_base_address = 0;

int get_game_socket() {
  if (game_base_address == 0) {
    throw std::runtime_error("game base address is unavailable");
  }

  const int socket_index = *reinterpret_cast<const int *>(
      game_base_address + game_layout::game_socket_index_offset);
  if (socket_index < 0 || socket_index > 7) {
    throw std::runtime_error("game UDP socket index is invalid");
  }

  const int socket_descriptor = *reinterpret_cast<const int *>(
      game_base_address + game_layout::game_socket_descriptors_offset +
      (static_cast<std::uintptr_t>(socket_index) * sizeof(int)));
  if (socket_descriptor < 0) {
    throw std::runtime_error("game UDP socket is unavailable");
  }
  return socket_descriptor;
}

} // namespace

void locate(std::uintptr_t address) { game_base_address = address; }

std::string random_token() {
  static constexpr char hexadecimal_digits[] = "0123456789abcdef";

  std::random_device random;
  std::string token(32, '0');
  for (std::size_t index = 0; index < token.size(); index += 2) {
    const auto byte = static_cast<unsigned int>(random()) & 0xffU;
    token[index] = hexadecimal_digits[byte >> 4U];
    token[index + 1] = hexadecimal_digits[byte & 0x0fU];
  }
  return token;
}

int send_packet(std::string_view address, int port, std::string_view bytes) {
  if (port <= 0 || port > 65535) {
    throw std::invalid_argument("UDP destination port out of range");
  }
  if (bytes.empty() || bytes.size() > 1200U) {
    throw std::invalid_argument("UDP payload size out of range");
  }

  const int socket_descriptor = get_game_socket();

  sockaddr_in destination{};
  destination.sin_family = AF_INET;
  destination.sin_port = htons(static_cast<std::uint16_t>(port));
  if (::inet_pton(AF_INET, std::string(address).c_str(),
                  &destination.sin_addr) != 1) {
    throw std::invalid_argument("invalid IPv4 address");
  }

  const auto sent = ::sendto(socket_descriptor, bytes.data(), bytes.size(), 0,
                             reinterpret_cast<const sockaddr *>(&destination),
                             sizeof(destination));
  if (sent == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    throw std::runtime_error(std::strerror(errno));
  }
  return static_cast<int>(sent);
}

sol::table drain_src_packets(sol::this_state state) {
  static constexpr std::string_view src_magic = "7DFPSRCU";
  static constexpr std::size_t maximum_datagram_size = 1200;
  static constexpr int maximum_drain_count = 32;

  sol::state_view lua(state);
  sol::table result = lua.create_table();
  sol::table packets = lua.create_table();
  result["packets"] = packets;
  result["vanillaPending"] = false;

  const int socket_descriptor = get_game_socket();
  std::array<char, maximum_datagram_size> buffer{};
  int drained_count = 0;

  while (drained_count < maximum_drain_count) {
    sockaddr_in source{};
    socklen_t source_size = sizeof(source);
    const auto peeked_size =
        ::recvfrom(socket_descriptor, buffer.data(), buffer.size(),
                   MSG_PEEK | MSG_DONTWAIT | MSG_TRUNC,
                   reinterpret_cast<sockaddr *>(&source), &source_size);
    if (peeked_size == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      throw std::runtime_error(std::strerror(errno));
    }

    const bool is_src_packet =
        peeked_size >= static_cast<ssize_t>(src_magic.size()) &&
        std::memcmp(buffer.data(), src_magic.data(), src_magic.size()) == 0;
    if (!is_src_packet) {
      result["vanillaPending"] = true;
      break;
    }

    source_size = sizeof(source);
    const auto received_size = ::recvfrom(
        socket_descriptor, buffer.data(), buffer.size(), MSG_DONTWAIT,
        reinterpret_cast<sockaddr *>(&source), &source_size);
    if (received_size == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      throw std::runtime_error(std::strerror(errno));
    }
    ++drained_count;

    if (peeked_size > static_cast<ssize_t>(buffer.size()) ||
        received_size != peeked_size || source.sin_family != AF_INET) {
      continue;
    }

    std::array<char, INET_ADDRSTRLEN> address{};
    if (::inet_ntop(AF_INET, &source.sin_addr, address.data(),
                    address.size()) == nullptr) {
      continue;
    }

    sol::table packet = lua.create_table();
    packet["address"] = std::string(address.data());
    packet["port"] = static_cast<int>(ntohs(source.sin_port));
    packet["data"] =
        std::string(buffer.data(), static_cast<std::size_t>(received_size));
    packets.add(packet);
  }

  result["drained"] = drained_count;
  return result;
}

sol::table current_packet_endpoint(sol::this_state state) {
  if (game_base_address == 0) {
    throw std::runtime_error("game base address is unavailable");
  }

  const auto address = *reinterpret_cast<const std::uint32_t *>(
      game_base_address + game_layout::current_packet_address_offset);
  const auto port = *reinterpret_cast<const std::int32_t *>(
      game_base_address + game_layout::current_packet_port_offset);

  sol::state_view lua(state);
  sol::table endpoint = lua.create_table();
  endpoint["address"] = std::to_string((address >> 24U) & 0xffU) + "." +
                        std::to_string((address >> 16U) & 0xffU) + "." +
                        std::to_string((address >> 8U) & 0xffU) + "." +
                        std::to_string(address & 0xffU);
  endpoint["port"] = static_cast<int>(port);
  return endpoint;
}

} // namespace udp
