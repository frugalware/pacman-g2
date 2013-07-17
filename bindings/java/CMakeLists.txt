SWIG_ADD_MODULE(pacman_java java ../pacman.i)
SET_SOURCE_FILES_PROPERTIES(pacman.i PROPERTIES SWIG_FLAGS "-includeall")
INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS})
SWIG_LINK_LIBRARIES(pacman_java pacman)
add_custom_command(TARGET ${SWIG_MODULE_pacman_java_REAL_NAME}
                   POST_BUILD
                   COMMAND "${Java_JAVAC_EXECUTABLE}" "*.java"
                   COMMAND "${Java_JAR_EXECUTABLE}" "-cf" "${CMAKE_CURRENT_BINARY_DIR}/pacman.jar" "*.class")
install(TARGETS ${SWIG_MODULE_pacman_java_REAL_NAME} DESTINATION lib)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pacman.jar DESTINATION share/java)


