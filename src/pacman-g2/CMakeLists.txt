set(PACMAN_SOURCES 
	conf.c
	deptest.c
	download.c
	list.c
	log.c
	package.c
	pacman-g2.c
	ps.c
	query.c
	sync.c
	trans.c
	util.c
	vercmp.c
)

include_directories (${PACMAN-G2_SOURCE_DIR})
include_directories (${PACMAN-G2_SOURCE_DIR}/lib/libpacman/)
find_library(M_LIB m)
add_executable(${CMAKE_PROJECT_NAME_LOWER} ${PACMAN_SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME_LOWER} pacman ${M_LIB})

execute_process(COMMAND ln -sf ./${CMAKE_PROJECT_NAME_LOWER} ./pacman
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

#translations
if(HAS_ASCIIDOC)
  ADD_SUBDIRECTORY(po)
endif(HAS_ASCIIDOC)
		
install(TARGETS "${CMAKE_PROJECT_NAME_LOWER}"  DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/pacman DESTINATION bin)
