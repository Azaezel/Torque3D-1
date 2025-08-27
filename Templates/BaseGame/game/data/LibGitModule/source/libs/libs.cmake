include(FetchContent)

FetchContent_Declare(
    libgit2
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libgit2
    BINARY_DIR ${TORQUE_LIB_TARG_DIRECTORY}/libgit2
)

set(USE_BUNDLED_ZLIB OFF CACHE BOOL "")
set(USE_BUNDLED_PCRE OFF CACHE BOOL "")
SET(BUILD_CLI OFF CACHE BOOL "")
set(BUILD_TESTS OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(CMAKE_SKIP_INSTALL_RULES ON CACHE BOOL "Skip install rules")
# Fetch libgit2 source
FetchContent_MakeAvailable(libgit2)

set(TORQUE_LINK_LIBRARIES ${TORQUE_LINK_LIBRARIES} libgit2package libgit2)
