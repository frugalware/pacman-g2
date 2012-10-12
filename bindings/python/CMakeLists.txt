SWIG_ADD_MODULE(pacman_python python ../pacman.i)
SET_SOURCE_FILES_PROPERTIES(pacman.i PROPERTIES SWIG_FLAGS "-includeall")
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
SWIG_LINK_LIBRARIES(pacman_python pacman)
add_custom_command(TARGET ${SWIG_MODULE_pacman_python_REAL_NAME}
                   POST_BUILD
                   COMMAND "${PYTHON_EXECUTABLE}" "-c" "import py_compile; py_compile.compile('${CMAKE_CURRENT_BINARY_DIR}/pacman.py', cfile='${CMAKE_CURRENT_BINARY_DIR}/pacman.pyc')"
                   VERBATIM)
install(TARGETS ${SWIG_MODULE_pacman_python_REAL_NAME} DESTINATION lib)
