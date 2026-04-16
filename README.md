# librosaserver_src_integration
Allows for custom ItemTypes via extending the default max count from 46 to the actual 255 (probably 256 just being safe) and checking for `mass > 0`.

Ships alongside `libminiz.so`, which exposes in-memory ZIP helpers used by `rs_integration` for faster syncing.

## Future
In the future we will add extending of `VehicleTypes` as well and maybe some stuff that will be more optimized in C++.

## Disclaimer
This is an integral server part of RosaServer integration for SR:C, you won't be able to make SR:C function on your server without this.

## Behavior
- `require("librosaserver_src_integration")` installs the Lua-side `itemTypes` overrides.
- The module extends `itemTypes` access past the stock RosaServer max of 46 up to an actual max of 255.
- Extended item types are treated as valid when `mass > 0`.
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
git clone --recursive https://github.com/SubRosaCustom/librosaserver_src_integration.git
cd librosaserver_src_integration
make -C deps/moonjit/src
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
