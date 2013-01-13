# - Find curl
# Find the native PacCURL headers and libraries.
#
#  PACCURL_INCLUDE_DIRS   - where to find paccurl/curl.h, etc.
#  PACCURL_LIBRARIES      - List of libraries when using curl.

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2012 Rolf Eike Beer <eike@sf-mail.de>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# Look for the header file.
find_path(PACCURL_INCLUDE_DIR NAMES paccurl/curl.h)
mark_as_advanced(PACCURL_INCLUDE_DIR)

# Look for the library (sorted from most current/relevant entry to least).
find_library(PACCURL_LIBRARY NAMES
    paccurl
)
mark_as_advanced(PACCURL_LIBRARY)

if(PACCURL_INCLUDE_DIR)
  foreach(_curl_version_header curlver.h curl.h)
    if(EXISTS "${CURL_INCLUDE_DIR}/paccurl/${_curl_version_header}")
      file(STRINGS "${CURL_INCLUDE_DIR}/paccurl/${_curl_version_header}" curl_version_str REGEX "^#define[\t ]+LIBCURL_VERSION[\t ]+\".*\"")

      string(REGEX REPLACE "^#define[\t ]+LIBCURL_VERSION[\t ]+\"([^\"]*)\".*" "\\1" PACCURL_VERSION_STRING "${curl_version_str}")
      unset(curl_version_str)
      break()
    endif()
  endforeach()
endif()

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PACCURL
                                  REQUIRED_VARS PACCURL_LIBRARY PACCURL_INCLUDE_DIR
                                  VERSION_VAR PACCURL_VERSION_STRING)

if(PACCURL_LIBRARY AND PACCURL_INCLUDE_DIR)
  set(CURL_LIBRARIES ${PACCURL_LIBRARY})
  set(CURL_INCLUDE_DIRS ${PACCURL_INCLUDE_DIR})
  set(PACCURL_FOUND TRUE)
endif(PACCURL_LIBRARY AND PACCURL_INCLUDE_DIR)
