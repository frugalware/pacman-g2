set(LIBPACMAN_SOURCES
	hash/md5.c
	hash/md5driver.c
	hash/sha1.c
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
	package.c
	packages_transaction.c
	pacman.c
	provide.c
	remove.c
	server.c
	sync.c
	trans.c
	trans_sysupgrade.c
	util.c
	versioncmp.c
)

include_directories (${PACMAN-G2_SOURCE_DIR})

include_directories (${PACMAN-G2_SOURCE_DIR}/lib/libftp)

find_library(ARCHIVE_LIB archive)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE64_SOURCE")

add_library(pacman SHARED ${LIBPACMAN_SOURCES})

target_link_libraries(pacman ftp archive)

install(TARGETS pacman DESTINATION lib)
