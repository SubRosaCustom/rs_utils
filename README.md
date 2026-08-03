[![Release](https://github.com/SubRosaCustom/rs_utils/actions/workflows/release.yml/badge.svg)](https://github.com/SubRosaCustom/rs_utils/actions/workflows/release.yml)
[![Test](https://github.com/SubRosaCustom/rs_utils/actions/workflows/test.yml/badge.svg)](https://github.com/SubRosaCustom/rs_utils/actions/workflows/test.yml)

# rs_utils
Native helper repo for SRC server-side RosaServer integration. It builds the two Lua-loadable shared libraries used by `rs_integration`:
- `librosaserver_src_integration.so` for custom item and vehicle types, model loaders, and game UDP access
- `libminiz.so` for in-memory ZIP archive helpers

## Disclaimer
This is an integral server part of RosaServer integration for SR:C, you won't be able to make SR:C function on your server without this.

## Behavior
- `require("librosaserver_src_integration")` installs the Lua-side `itemTypes` and `vehicleTypes` overrides.
- The native game layout is enabled only for Sub Rosa version `38e`, reported by RosaServer as `server.versionMajor == 38` and `server.versionMinor == 4`.
- `sendPacket` sends standalone datagrams through the game's existing UDP socket.
- `drainSrcPackets` drains bounded SRC datagrams without consuming pending vanilla game packets.
- `currentPacketEndpoint` reports the source endpoint of the current game packet.
- `randomToken` generates the server's UDP authentication tokens.
- The module extends `itemTypes` access past the stock RosaServer max of 46 up to an actual max of 255.
- The module extends `vehicleTypes` access past the stock RosaServer max of 17 up to an actual max of 127.
- Extended item types are treated as valid when `mass > 0`.
- Extended vehicle types are treated as valid when `mass > 0`.
- `loadITM`, `loadIT3`, and `loadSBV` load custom item and vehicle models.
- `clearCustomVehicleTypeSlots` clears vehicle type slots 17 through 127.
- `setupVehicleTypeNew` and `setupObjectTypeWeight` initialize custom vehicle type data.
- `require("libminiz")` registers the global `miniz` table with `createZip` / `extractZip`.

## Lua Usage
`LD_PRELOAD` only loads the shared libraries into the process. You still need to require each module from Lua.

```lua
require("librosaserver_src_integration")
require("libminiz")

print(#itemTypes)
print(itemTypes[47].name)

local archive = miniz.createZip({
  ["scripts/test.lua"] = "print('hello')",
  ["assets/test.txt"] = "abc"
})

local files = miniz.extractZip(archive)
print(files["assets/test.txt"])
```

## Building
Clone recursively so the vendored dependencies are present:

```bash
git clone --recursive https://github.com/SubRosaCustom/rs_utils.git
cd rs_utils
make -C deps/moonjit/src XCFLAGS+="-DLUAJIT_ENABLE_LUA52COMPAT -DLUAJIT_ENABLE_GC64"
cmake -S . -B build
cmake --build build --config Release
```

Build output:
- `build/librosaserver_src_integration.so`
- `build/libminiz.so`

This repo vendors these dependencies as submodules:
- `deps/moonjit`
- `deps/sol2`
- `deps/miniz`

## Testing

After building, run `cd test && ./test`. The suite loads the current build into
a RosaServer runtime and exercises the Lua modules and game UDP socket.

## Installation
- Download the latest release
- Unzip it
- Put `librosaserver_src_integration.so` and `libminiz.so` beside `librosaserver.so`
- Put `libluajit.so` beside your server binaries if it is not already present
- Edit or create a start script to preload both shared libraries

```bash
LD_PRELOAD="$(pwd)/libluajit.so $(pwd)/librosaserver_src_integration.so $(pwd)/libminiz.so $(pwd)/librosaserver.so" ./subrosadedicated.x64
```

Then require both modules from your RosaServer Lua entrypoint:

```lua
require("librosaserver_src_integration")
require("libminiz")
```

## Releases
- GitHub Actions currently builds Linux artifacts.
- The release artifact is a `.zip` containing `librosaserver_src_integration.so`, `libminiz.so`, and `libluajit.so`.
