add_library(ftp SHARED ftplib.c)

install(TARGETS ftp DESTINATION lib)
