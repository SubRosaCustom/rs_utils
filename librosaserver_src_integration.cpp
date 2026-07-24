#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>

#include "sol/sol.hpp"
#include "structs.h"

namespace {

static constexpr unsigned int rsMaxNumberOfItemTypes = 46;
static constexpr unsigned int actualMaxNumberOfItemTypes = 255;
static constexpr unsigned int rsMaxNumberOfVehicleTypes = 17;
static constexpr unsigned int actualMaxNumberOfVehicleTypes = 127;

ItemType *itemTypes = nullptr;
VehicleType *vehicleTypes = nullptr;
uintptr_t gameBaseAddress = 0;

using LoadSBVFunction = void (*)(int, const char *);
using LoadItemModelFunction = void (*)(int, char *);
using SetupVehicleTypeNewFunction = void (*)(int, float, float, int);
using SetupObjectTypeWeightFunction = void (*)(int);

LoadSBVFunction loadSBVFn = nullptr;
LoadItemModelFunction loadITMFn = nullptr;
LoadItemModelFunction loadIT3Fn = nullptr;
SetupVehicleTypeNewFunction setupVehicleTypeNewFn = nullptr;
SetupObjectTypeWeightFunction setupObjectTypeWeightFn = nullptr;

std::string randomToken() {
  static constexpr char HEX[] = "0123456789abcdef";
  std::random_device random;
  std::string token(32, '0');
  for (std::size_t i = 0; i < token.size(); i += 2) {
    const auto byte = static_cast<unsigned int>(random()) & 0xffU;
    token[i] = HEX[byte >> 4U];
    token[i + 1] = HEX[byte & 0x0fU];
  }
  return token;
}

int gameSocketFd() {
  if (gameBaseAddress == 0) {
    throw std::runtime_error("game base address is unavailable");
  }

  const int socket_index =
      *reinterpret_cast<const int *>(gameBaseAddress + 0x39075c20);
  if (socket_index < 0 || socket_index > 7) {
    throw std::runtime_error("game UDP socket index is invalid");
  }
  const int socket_fd = *reinterpret_cast<const int *>(
      gameBaseAddress + 0x39075c44 + (static_cast<uintptr_t>(socket_index) * 4U));
  if (socket_fd < 0) {
    throw std::runtime_error("game UDP socket is unavailable");
  }
  return socket_fd;
}

int sendPacket(std::string_view address, int port, std::string_view bytes) {
  if (port <= 0 || port > 65535) {
    throw std::invalid_argument("UDP destination port out of range");
  }
  if (bytes.empty() || bytes.size() > 1200U) {
    throw std::invalid_argument("UDP payload size out of range");
  }

  const int socket_fd = gameSocketFd();

  sockaddr_in destination{};
  destination.sin_family = AF_INET;
  destination.sin_port = htons(static_cast<std::uint16_t>(port));
  if (::inet_pton(AF_INET, std::string(address).c_str(),
                  &destination.sin_addr) != 1) {
    throw std::invalid_argument("invalid IPv4 address");
  }

  const auto sent =
      ::sendto(socket_fd, bytes.data(), bytes.size(), 0,
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

sol::table drainSrcPackets(sol::this_state state) {
  static constexpr std::string_view MAGIC = "7DFPSRCU";
  static constexpr std::size_t MAX_DATAGRAM_SIZE = 1200;
  static constexpr int MAX_DRAIN_COUNT = 32;

  sol::state_view lua(state);
  sol::table result = lua.create_table();
  sol::table packets = lua.create_table();
  result["packets"] = packets;
  result["vanillaPending"] = false;

  const int socket_fd = gameSocketFd();
  std::array<char, MAX_DATAGRAM_SIZE> buffer{};
  int drained = 0;

  while (drained < MAX_DRAIN_COUNT) {
    sockaddr_in source{};
    socklen_t source_size = sizeof(source);
    const auto peeked =
        ::recvfrom(socket_fd, buffer.data(), buffer.size(),
                   MSG_PEEK | MSG_DONTWAIT | MSG_TRUNC,
                   reinterpret_cast<sockaddr *>(&source), &source_size);
    if (peeked == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      throw std::runtime_error(std::strerror(errno));
    }

    const bool is_src =
        peeked >= static_cast<ssize_t>(MAGIC.size()) &&
        std::memcmp(buffer.data(), MAGIC.data(), MAGIC.size()) == 0;
    if (!is_src) {
      result["vanillaPending"] = true;
      break;
    }

    source_size = sizeof(source);
    const auto received =
        ::recvfrom(socket_fd, buffer.data(), buffer.size(), MSG_DONTWAIT,
                   reinterpret_cast<sockaddr *>(&source), &source_size);
    if (received == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      throw std::runtime_error(std::strerror(errno));
    }
    ++drained;

    if (peeked > static_cast<ssize_t>(buffer.size()) ||
        received != peeked || source.sin_family != AF_INET) {
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
        std::string(buffer.data(), static_cast<std::size_t>(received));
    packets.add(packet);
  }

  result["drained"] = drained;
  return result;
}

sol::table currentPacketEndpoint(sol::this_state state) {
  if (gameBaseAddress == 0) {
    throw std::runtime_error("game base address is unavailable");
  }

  const auto address =
      *reinterpret_cast<const std::uint32_t *>(gameBaseAddress + 0x39085C84);
  const auto port =
      *reinterpret_cast<const std::int32_t *>(gameBaseAddress + 0x39085C88);

  sol::state_view lua(state);
  sol::table endpoint = lua.create_table();
  endpoint["address"] =
      std::to_string((address >> 24U) & 0xffU) + "." +
      std::to_string((address >> 16U) & 0xffU) + "." +
      std::to_string((address >> 8U) & 0xffU) + "." +
      std::to_string(address & 0xffU);
  endpoint["port"] = static_cast<int>(port);
  return endpoint;
}

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

  gameBaseAddress = lua["memory"]["getBaseAddress"]();
  itemTypes = reinterpret_cast<ItemType *>(gameBaseAddress + 0x5a60d7c0);
  vehicleTypes = reinterpret_cast<VehicleType *>(gameBaseAddress + 0x4d03560);
  loadITMFn = reinterpret_cast<LoadItemModelFunction>(gameBaseAddress + 0x41740);
  loadIT3Fn = reinterpret_cast<LoadItemModelFunction>(gameBaseAddress + 0x42c40);
  loadSBVFn = reinterpret_cast<LoadSBVFunction>(gameBaseAddress + 0xaf0e0);
  setupVehicleTypeNewFn = reinterpret_cast<SetupVehicleTypeNewFunction>(
      gameBaseAddress + 0xac890);
  setupObjectTypeWeightFn = reinterpret_cast<SetupObjectTypeWeightFunction>(
      gameBaseAddress + 0xabec0);

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
  library["randomToken"] = &randomToken;
  library["sendPacket"] = &sendPacket;
  library["drainSrcPackets"] = &drainSrcPackets;
  library["currentPacketEndpoint"] = &currentPacketEndpoint;
  lua["srcIntegrationNative"] = library;
  return library;
}

} // namespace

extern "C" __attribute__((visibility("default"))) int
luaopen_librosaserver_src_integration(lua_State *state) {
  return sol::stack::call_lua(state, 1, openLibrary);
}
