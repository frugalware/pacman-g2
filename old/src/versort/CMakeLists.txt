include_directories (${PACMAN-G2_SOURCE_DIR})

include_directories (${PACMAN-G2_SOURCE_DIR}/lib/libpacman)

add_executable(versort versort.c)

target_link_libraries(versort pacman)

install(TARGETS versort DESTINATION bin)
