SWIG_ADD_MODULE(pacman_csharp csharp ../pacman.i)
SET_SOURCE_FILES_PROPERTIES(pacman.i PROPERTIES SWIG_FLAGS "-includeall")
SWIG_LINK_LIBRARIES(pacman_csharp pacman)
add_custom_command(TARGET ${SWIG_MODULE_pacman_csharp_REAL_NAME}
                   POST_BUILD
                   COMMAND "${SN_EXECUTABLE}" "-quiet" "-k" "1024" "pacman-csharp.snk"
                   COMMAND "${MCS_EXECUTABLE}" "/unsafe" "/out:pacman-csharp.dll" "/target:library" "/keyfile:pacman-csharp.snk" "*.cs")
install(TARGETS ${SWIG_MODULE_pacman_csharp_REAL_NAME} DESTINATION lib)
