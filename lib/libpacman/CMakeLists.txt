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
	packages_transaction.c
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
  message(WARNING "libpaccurl.so not found, won't build pacman-g2.static")
else()
  include_directories("${CURL_INCLUDE_DIRS}/paccurl")
endif(NOT PACCURL_FOUND)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE64_SOURCE")

add_library(pacman SHARED ${LIBPACMAN_SOURCES})
set_target_properties(pacman PROPERTIES SOVERSION 0.${PACMAN_G2_LIB_VERSION})
target_link_libraries(pacman ${LibArchive_LIBRARIES} ${CURL_LIBRARIES})

#static build
if(PACCURL_FOUND)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  add_library(pacman-static STATIC ${LIBPACMAN_SOURCES})
  set_target_properties(pacman-static PROPERTIES OUTPUT_NAME "pacman")
  set_target_properties(pacman-static PROPERTIES LINK_SEARCH_START_STATIC ON)
  set_target_properties(pacman-static PROPERTIES LINK_SEARCH_END_STATIC ON)
  find_library(LIB_ARCHIVE_STATIC archive)
  find_library(LIB_PACCURL_STATIC paccurl)
  target_link_libraries(pacman-static ${LIB_ARCHIVE_STATIC} ${LIB_PACCURL_STATIC})
  install(TARGETS pacman-static DESTINATION lib)
endif(PACCURL_FOUND)

install(TARGETS pacman DESTINATION lib)
install(FILES pacman.h DESTINATION include)
