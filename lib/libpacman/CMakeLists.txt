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

find_package(LibArchive REQUIRED)

find_package(PacCURL)

if(NOT PACCURL_FOUND)
  find_package(CURL REQUIRED)
    include_directories("${CURL_INCLUDE_DIRS}/curl")
else()
  include_directories("${CURL_INCLUDE_DIRS}/paccurl")
endif(NOT PACCURL_FOUND)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE64_SOURCE")

message("paccurl include: ${CURL_INCLUDE_DIRS}")
add_library(pacman SHARED ${LIBPACMAN_SOURCES})
set_target_properties(pacman PROPERTIES SOVERSION 0.${PACMAN_G2_LIB_VERSION})
message("paccurl: ${CURL_LIBRARIES}")
target_link_libraries(pacman ${LibArchive_LIBRARIES} ${CURL_LIBRARIES})

install(TARGETS pacman DESTINATION lib)
install (FILES pacman.h DESTINATION include)
