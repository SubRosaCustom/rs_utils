cmake_minimum_required(VERSION 3.8)

set(DEPS_ROOT "${CMAKE_CURRENT_LIST_DIR}/deps")
set(MOONJIT_ROOT "${DEPS_ROOT}/moonjit")
set(SOL2_ROOT "${DEPS_ROOT}/sol2")
set(MINIZ_ROOT "${DEPS_ROOT}/miniz")

add_library(rosaserver_src_integration SHARED
    librosaserver_src_integration.cpp
    "${MINIZ_ROOT}/miniz.c"
    "${MINIZ_ROOT}/miniz_tdef.c"
    "${MINIZ_ROOT}/miniz_tinfl.c"
    "${MINIZ_ROOT}/miniz_zip.c"
)

set_property(TARGET rosaserver_src_integration PROPERTY CXX_STANDARD 17)
set_property(TARGET rosaserver_src_integration PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET rosaserver_src_integration PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET rosaserver_src_integration PROPERTY VISIBILITY_INLINES_HIDDEN ON)

target_compile_definitions(rosaserver_src_integration PRIVATE SOL_LUAJIT)
target_link_libraries(rosaserver_src_integration "${MOONJIT_ROOT}/src/libluajit.so")
include_directories("${MOONJIT_ROOT}/src")
include_directories("${SOL2_ROOT}/include")
include_directories("${MINIZ_ROOT}")
