cmake_minimum_required (VERSION 2.8)
project (PACMAN-G2)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

include (CheckIncludeFile)
include (CheckFunctionExists)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

string(TOLOWER ${CMAKE_PROJECT_NAME} CMAKE_PROJECT_NAME_LOWER)

set (PACMAN_G2_VERSION 3.8.9)

SET(ENABLE_NLS TRUE CACHE BOOL "Translation program messages to the user's native language ?")

find_package(Doxygen QUIET)
message("searched for doxy")
IF(DOXYGEN_FOUND)
  message("found doxy")
  SET(HAS_DOXYGEN TRUE CACHE BOOL "Use Doxygen")
ENDIF(DOXYGEN_FOUND)

find_package(Asciidoc QUIET)
message("searched for ascii doc")
if(ASCIIDOC_FOUND)
  message("found asciidoc")
  SET(HAS_ASCIIDOC TRUE CACHE BOOL "Use Asciidoc")
ENDIF(ASCIIDOC_FOUND)

find_package(Mono QUIET)
message("searched for Mono")
if(MONO_FOUND)
  message("found Mono")
  SET(HAS_CSHARP TRUE CACHE BOOL "C# bindings")
ENDIF(MONO_FOUND)

find_package(PythonInterp QUIET)
message("searched for Python")
if(PYTHONINTERP_FOUND)
  message("found Python")
  SET(HAS_PYTHON TRUE CACHE BOOL "Python bindings")
ENDIF(PYTHONINTERP_FOUND)

CHECK_INCLUDE_FILE(dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILE(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILE(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILE(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILE(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H)

CHECK_FUNCTION_EXISTS(dcgettext HAVE_DCGETTEXT)
CHECK_FUNCTION_EXISTS(gettext HAVE_GETTEXT)
CHECK_FUNCTION_EXISTS(iconv HAVE_ICONV)
CHECK_FUNCTION_EXISTS(strverscmp HAVE_STRVERSCMP)

configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.cmake"
  "${PROJECT_SOURCE_DIR}/config.h"
  )

add_subdirectory (lib/libftp)
add_subdirectory (lib/libpacman)
add_subdirectory (src/versort)
add_subdirectory (src/vercmp)
add_subdirectory (src/pacman-g2)
