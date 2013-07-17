install(DIRECTORY bash_completion.d DESTINATION /etc)
set (PKG_EXT "fpm")
set (PKG_COMP "xz")
CONFIGURE_FILE (
  "makepkg.conf.in"
  "makepkg.conf"
  )
CONFIGURE_FILE (
  "pacman-g2.conf.in"
  "pacman-g2.conf"
  )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/makepkg.conf
	      ${CMAKE_CURRENT_BINARY_DIR}/pacman-g2.conf
	      DESTINATION /etc)
add_subdirectory(pacman-g2)
