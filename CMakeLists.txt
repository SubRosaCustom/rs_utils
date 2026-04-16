cmake_minimum_required(VERSION 3.8)

set(DEPS_ROOT "${CMAKE_CURRENT_LIST_DIR}/deps")
set(MOONJIT_ROOT "${DEPS_ROOT}/moonjit")
set(SOL2_ROOT "${DEPS_ROOT}/sol2")
set(MINIZ_ROOT "${DEPS_ROOT}/miniz")
set(ROSASERVER_ROOT "${DEPS_ROOT}/RosaServer/RosaServer")

# librosaserver_src_integration.so
add_library(rosaserver_src_integration SHARED
    librosaserver_src_integration.cpp
)

set_property(TARGET rosaserver_src_integration PROPERTY CXX_STANDARD 17)
set_property(TARGET rosaserver_src_integration PROPERTY C_VISIBILITY_PRESET default)
set_property(TARGET rosaserver_src_integration PROPERTY CXX_VISIBILITY_PRESET default)
set_property(TARGET rosaserver_src_integration PROPERTY VISIBILITY_INLINES_HIDDEN OFF)

target_compile_definitions(rosaserver_src_integration PRIVATE SOL_LUAJIT)
target_include_directories(rosaserver_src_integration PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${MOONJIT_ROOT}/src"
    "${SOL2_ROOT}/include"
    "${ROSASERVER_ROOT}"
)
target_link_libraries(rosaserver_src_integration "${MOONJIT_ROOT}/src/libluajit.so")


# libminiz.so
add_library(miniz SHARED
    libminiz.cpp
    "${MINIZ_ROOT}/miniz.c"
    "${MINIZ_ROOT}/miniz_tdef.c"
    "${MINIZ_ROOT}/miniz_tinfl.c"
    "${MINIZ_ROOT}/miniz_zip.c"
)

set_property(TARGET miniz PROPERTY CXX_STANDARD 17)
set_property(TARGET miniz PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET miniz PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET miniz PROPERTY VISIBILITY_INLINES_HIDDEN ON)

target_compile_definitions(miniz PRIVATE SOL_LUAJIT)
target_include_directories(miniz PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${MOONJIT_ROOT}/src"
    "${SOL2_ROOT}/include"
    "${MINIZ_ROOT}"
)
target_link_libraries(miniz "${MOONJIT_ROOT}/src/libluajit.so")
