# librosaserver_src_integration
Allows for custom ItemTypes via extending the default max count from 46 to the actual 255 (probably 256 just being safe) and checking for mass > 0.
Also adds miniz zip file creation for rs_integration to have faster syncing.

## Future
In the future we will add extending of VehicleTypes as well and maybe some stuff that will be more optimized in C++

## Disclaimer
This is a integral server part of Rosaserver integariton of SR:C, you won't be able to make SR:C function on your server without this.

## Installation
- Download the latest release
- Unzip it
- Put librosaserver_src_integration.so beside librosaserver.so
- Edit or create new start script to have librosaserver_src_integration.so
```
LD_PRELOAD="$(pwd)/libluajit.so $(pwd)/librosaserver_src_integration.so $(pwd)/librosaserver.so" ./subrosadedicated.x64
```
