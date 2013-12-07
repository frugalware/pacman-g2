#pacman.pc
SET(prefix "/usr")
SET(libdir "\${exec_prefix}/lib")
SET(includedir "\${prefix}/include")
CONFIGURE_FILE("pacman.pc.in" "pacman.pc" @ONLY)

set(FLIB_SOURCES
	fstdlib/canonicalize.c
	hash/md5.c
	hash/md5driver.c
	hash/sha1.c
	io/archive.c
	io/ffilelock.c
	io/ftp.c
	util/list.c
	util/log.c
	util/stringlist.c
	util/time.c
	fstring.c
)

set(LIBPACMAN_SOURCES 
	${FLIB_SOURCES}
	db/fakedb.c
	db/localdb.c
	db/localdb_files.c
	db/syncdb.c
	package/pkginfo.c
	add.c
	be_files.c
	cache.c
	config_parser.c
	conflict.c
	db.c
	deps.c
	error.c
	group.c
	handle.c
	package.c
	packages_transaction.c
	pacman.c
	remove.c
	server.c
	sync.c
	trans.c
	trans_sysupgrade.c
	util.c
	versioncmp.c
)

include_directories (${PACMAN-G2_SOURCE_DIR})
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

find_package(LibArchive REQUIRED)
find_package(PacCURL)
if(NOT PACCURL_FOUND)
  find_package(CURL REQUIRED)
  include_directories("${CURL_INCLUDE_DIRS}/curl")
  message(WARNING "libpaccurl not found")
else()
  include_directories("${CURL_INCLUDE_DIRS}/paccurl")
endif(NOT PACCURL_FOUND)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE64_SOURCE")

add_library(pacman SHARED ${LIBPACMAN_SOURCES})
set_target_properties(pacman PROPERTIES SOVERSION 0.${PACMAN_G2_LIB_VERSION})
target_link_libraries(pacman ${LibArchive_LIBRARIES} ${CURL_LIBRARIES})


#static build
if(PACCURL_FOUND AND (NOT DISABLE_STATIC_BUILD))
  SET(CMAKE_FIND_LIBRARY_SUFFIXES .a)
  add_library(pacman-static STATIC ${LIBPACMAN_SOURCES})
  set_target_properties(pacman-static PROPERTIES OUTPUT_NAME "pacman")
  set_target_properties(pacman-static PROPERTIES LINK_SEARCH_START_STATIC ON)
  set_target_properties(pacman-static PROPERTIES LINK_SEARCH_END_STATIC ON)
  find_library(LIB_ARCHIVE_STATIC archive)
  find_library(LIB_PACCURL_STATIC paccurl)
  find_library(LIB_SSL_STATIC ssl)
  target_link_libraries(pacman-static ${LIB_ARCHIVE_STATIC} ${LIB_PACCURL_STATIC} ${LIB_SSL_STATIC})
  install(TARGETS pacman-static DESTINATION lib)
else()
  message(WARNING "Won't build pacman-g2.static")
  set(DISABLE_STATIC_BUILD TRUE  CACHE BOOL "Disable static build")
endif(PACCURL_FOUND AND (NOT DISABLE_STATIC_BUILD))

install(TARGETS pacman DESTINATION lib)
install(FILES pacman.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pacman.pc DESTINATION lib/pkgconfig)

