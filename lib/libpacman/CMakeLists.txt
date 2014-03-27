#pacman.pc
SET(prefix "/usr")
SET(libdir "\${exec_prefix}/lib")
SET(includedir "\${prefix}/include")
CONFIGURE_FILE("pacman.pc.in" "pacman.pc" @ONLY)

set(FLIB_SOURCES
	fstdlib/canonicalize.c
	fstring/basename.c
	fstring/case.c
	fstring/dirname.c
	fstring/matcher.c
	hash/md5.c
	hash/md5driver.c
	hash/sha1.c
	io/archive.c
	io/ffilelock.c
	io/ftp.c
	util/fcallback.h
	util/fdispatchlogger.c
	util/fdispatchlogger.h
	util/flist.c
	util/flist.h
	util/ffilelogger.c
	util/ffilelogger.h
	util/flogger.c
	util/flogger.h
	util/fptrlist.c
	util/fptrlist.h
	util/fsyslogger.c
	util/fsyslogger.h
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
	package/fpmpackage.c
	package/packagecache.c
	add.c
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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE64_SOURCE")

add_library(pacman ${BUILD_TYPE} ${LIBPACMAN_SOURCES})
target_link_libraries(pacman ${LIBPACMAN_LIBRARIES})
if(BUILD_SHARED_LIBS)
  set_target_properties(pacman PROPERTIES SOVERSION 0.${PACMAN_G2_LIB_VERSION})
else(BUILD_SHARED_LIBS)
  set_target_properties(pacman PROPERTIES LINK_SEARCH_START_STATIC ON)
  set_target_properties(pacman PROPERTIES LINK_SEARCH_END_STATIC ON)
endif(BUILD_SHARED_LIBS)

#translations
if(HAS_ASCIIDOC)
  add_subdirectory(po)
endif(HAS_ASCIIDOC)

if(HAS_DOXYGEN)
  add_custom_command(TARGET pacman
                   POST_BUILD
                   COMMAND doxygen Doxyfile
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  install(DIRECTORY ../../doc/man3/ DESTINATION share/man/man3/)
endif(HAS_DOXYGEN)

install(TARGETS pacman DESTINATION lib)
install(FILES pacman.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pacman.pc DESTINATION lib/pkgconfig)
