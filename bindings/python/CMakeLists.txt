
EXECUTE_PROCESS(COMMAND "${PYTHON_EXECUTABLE}" "-c" "from distutils import sysconfig; print sysconfig.get_python_lib()"
                COMMAND "sed" "s|/usr|${CMAKE_INSTALL_PREFIX}|"
                OUTPUT_VARIABLE PYTHON_STD_PACKAGES_PATH
                OUTPUT_STRIP_TRAILING_WHITESPACE)

INCLUDE_DIRECTORIES(${PACMAN-G2_SOURCE_DIR}/lib/libpacman/)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

ADD_LIBRARY(pacman_python_binding
	pacman_wrap.c pacman.py pacman.pyc)
SET_TARGET_PROPERTIES(pacman_python_binding PROPERTIES
	PREFIX "_"
	OUTPUT_NAME "pacman")
TARGET_LINK_LIBRARIES(pacman_python_binding pacman)

ADD_CUSTOM_COMMAND(OUTPUT pacman.pyc
	DEPENDS pacman.py
        COMMAND "${PYTHON_EXECUTABLE}" "-c" "import py_compile; py_compile.compile('${CMAKE_CURRENT_BINARY_DIR}/pacman.py', cfile='${CMAKE_CURRENT_BINARY_DIR}/pacman.pyc')"
        VERBATIM)

ADD_CUSTOM_COMMAND(OUTPUT pacman_wrap.c pacman.py
	DEPENDS "${PACMAN_SWIG_SRC}"
	COMMAND "swig" "-python" ${CMAKE_SWIG_FLAGS} "-outcurrentdir" "${PACMAN_SWIG_SRC}")

INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/_pacman.so" "${CMAKE_CURRENT_BINARY_DIR}/pacman.py" "${CMAKE_CURRENT_BINARY_DIR}/pacman.pyc"
	DESTINATION "${PYTHON_STD_PACKAGES_PATH}")

