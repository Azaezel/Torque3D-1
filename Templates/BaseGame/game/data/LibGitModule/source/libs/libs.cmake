set(USE_BUNDLED_ZLIB OFF CACHE STRING "We already build zlib" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Shared Library (OFF for Static)" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build Tests using the Clar suite" FORCE)
set(BUILD_CLI OFF CACHE BOOL "Build the command-line interface" FORCE)
add_subdirectory(${MODULE_SUBDIR}/libs/libgit2 ${TORQUE_LIB_TARG_DIRECTORY}/libgit2 EXCLUDE_FROM_ALL)

set(TORQUE_LINK_LIBRARIES ${TORQUE_LINK_LIBRARIES} libgit2)