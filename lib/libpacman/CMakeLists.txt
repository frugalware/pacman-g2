set(LIBPACMAN_SOURCES 
	add.c
	backup.c
	be_files.c
	cache.c
	conflict.c
	db.c
	deps.c
	error.c
	group.c
	handle.c
	list.c
	log.c
	md5.c
	md5driver.c
	package.c
	pacman.c
	provide.c
	remove.c
	server.c
	sha1.c
	sync.c
	trans.c
	trans_sysupgrade.c
	util.c
	versioncmp.c
)

include_directories (${PACMAN-G2_SOURCE_DIR})

include_directories (${PACMAN-G2_SOURCE_DIR}/lib/libftp)

find_package(LibArchive REQUIRED)
message("${LibArchive_LIBRARIES}")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE64_SOURCE")

add_library(pacman SHARED ${LIBPACMAN_SOURCES})

target_link_libraries(pacman ftp ${LibArchive_LIBRARIES})

install(TARGETS pacman DESTINATION lib)
install (FILES pacman.h DESTINATION include)
