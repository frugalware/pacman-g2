set(PACMAN_SOURCES 
	add.c
	conf.c
	deptest.c
	download.c
	list.c
	log.c
	package.c
	pacman-g2.c
	ps.c
	query.c
	remove.c
	sync.c
	trans.c
	upgrade.c
	util.c
)

include_directories (${PACMAN-G2_SOURCE_DIR})
find_library(M_LIB m)
add_executable(${CMAKE_PROJECT_NAME_LOWER} ${PACMAN_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME_LOWER} pacman ${M_LIB})

install(TARGETS "${CMAKE_PROJECT_NAME_LOWER}"  DESTINATION bin)