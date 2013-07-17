set (CARCH ${CMAKE_SYSTEM_PROCESSOR})
CONFIGURE_FILE (
  "${CMAKE_CURRENT_SOURCE_DIR}/frugalware-current.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/frugalware-current"
  )
CONFIGURE_FILE (
  "${CMAKE_CURRENT_SOURCE_DIR}/frugalware.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/frugalware"
  )

install(FILES frugalware-current frugalware
       DESTINATION /etc/pacman-g2/repos)
