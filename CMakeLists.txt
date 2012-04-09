cmake_minimum_required (VERSION 2.8)
project (PACMAN-G2)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

add_subdirectory (lib/libftp)
add_subdirectory (lib/libpacman)
add_subdirectory (src/versort)
add_subdirectory (src/vercmp)
add_subdirectory (src/pacman-g2)
