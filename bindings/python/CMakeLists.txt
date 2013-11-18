include_directories (${PACMAN-G2_SOURCE_DIR}/lib/libpacman/)
SWIG_ADD_MODULE(pacman_python python ../pacman.i)
SET_SOURCE_FILES_PROPERTIES(pacman.i PROPERTIES SWIG_FLAGS "-includeall")
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
SWIG_LINK_LIBRARIES(pacman_python pacman)
add_custom_command(TARGET ${SWIG_MODULE_pacman_python_REAL_NAME}
                   POST_BUILD
                   COMMAND "${PYTHON_EXECUTABLE}" "-c" "import py_compile; py_compile.compile('${CMAKE_CURRENT_BINARY_DIR}/pacman.py', cfile='${CMAKE_CURRENT_BINARY_DIR}/pacman.pyc')"
                   VERBATIM)
install(TARGETS ${SWIG_MODULE_pacman_python_REAL_NAME} DESTINATION lib)
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c" "from distutils import sysconfig; print sysconfig.get_python_lib()"
                COMMAND "sed" "s|/usr|${CMAKE_INSTALL_PREFIX}|"
                OUTPUT_VARIABLE PYTHON_STD_PACKAGES_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE)
message("${PYTHON_STD_PACKAGES_PATH}")                
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pacman.py ${CMAKE_CURRENT_BINARY_DIR}/pacman.pyc 
        DESTINATION ${PYTHON_STD_PACKAGES_PATH})