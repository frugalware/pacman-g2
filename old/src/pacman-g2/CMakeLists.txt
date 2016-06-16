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

add_executable(pacman-g2 ${PACMAN_SOURCES})

target_link_libraries(pacman-g2 pacman ${M_LIB})

install(TARGETS pacman-g2 DESTINATION bin)
