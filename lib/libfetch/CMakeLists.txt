SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -DINET6 -DWITH_SSL -DNETBSD")
#add_definitions(-DFOO -DBAR ...)

add_custom_command(OUTPUT ftperr.h
		   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/errlist.sh "ftp_errlist" "FTP" ${CMAKE_CURRENT_SOURCE_DIR}/ftp.errors ">" "ftperr.h")
add_custom_command(OUTPUT httperr.h          
                   COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/errlist.sh "http_errlist" "HTTP" ${CMAKE_CURRENT_SOURCE_DIR}/http.errors > "httperr.h")

include_directories(${CMAKE_CURRENT_BINARY_DIR})
    
add_library(fetch SHARED common.c fetch.c file.c ftp.c http.c httperr.h ftperr.h)

install(TARGETS fetch DESTINATION lib)
