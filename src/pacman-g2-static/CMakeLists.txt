set(PACMAN_SOURCES 
	../${CMAKE_PROJECT_NAME_LOWER}/add.c
	../${CMAKE_PROJECT_NAME_LOWER}/conf.c
	../${CMAKE_PROJECT_NAME_LOWER}/deptest.c
	../${CMAKE_PROJECT_NAME_LOWER}/download.c
	../${CMAKE_PROJECT_NAME_LOWER}/list.c
	../${CMAKE_PROJECT_NAME_LOWER}/log.c
	../${CMAKE_PROJECT_NAME_LOWER}/package.c
	../${CMAKE_PROJECT_NAME_LOWER}/pacman-g2.c
	../${CMAKE_PROJECT_NAME_LOWER}/ps.c
	../${CMAKE_PROJECT_NAME_LOWER}/query.c
	../${CMAKE_PROJECT_NAME_LOWER}/remove.c
	../${CMAKE_PROJECT_NAME_LOWER}/sync.c
	../${CMAKE_PROJECT_NAME_LOWER}/trans.c
	../${CMAKE_PROJECT_NAME_LOWER}/upgrade.c
	../${CMAKE_PROJECT_NAME_LOWER}/util.c
)

include_directories (${PACMAN-G2_SOURCE_DIR})
include_directories (${PACMAN-G2_SOURCE_DIR}/lib/libpacman/)

set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -pthread")
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
set(CMAKE_EXE_LINK_DYNAMIC_C_FLAGS)       # remove -Wl,-Bdynamic)
set(CMAKE_SHARED_LIBRARY_C_FLAGS)         # remove -fPIC
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)    # remove -rdynamic


add_executable("${CMAKE_PROJECT_NAME_LOWER}.static" ${PACMAN_SOURCES})
set_target_properties("${CMAKE_PROJECT_NAME_LOWER}.static" PROPERTIES LINK_SEARCH_START_STATIC ON)
set_target_properties("${CMAKE_PROJECT_NAME_LOWER}.static" PROPERTIES LINK_SEARCH_END_STATIC ON)
target_link_libraries("${CMAKE_PROJECT_NAME_LOWER}.static" pacman-static bz2 lzma m archive dl bz2 lzma m archive z crypto expat rt)

install(TARGETS "${CMAKE_PROJECT_NAME_LOWER}.static" DESTINATION bin)