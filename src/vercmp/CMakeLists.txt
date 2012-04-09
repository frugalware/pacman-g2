#include_directories (${PACMAN-G2_SOURCE_DIR})

add_executable(vercmp vercmp.c)

target_link_libraries(vercmp pacman)

install(TARGETS vercmp DESTINATION bin)
