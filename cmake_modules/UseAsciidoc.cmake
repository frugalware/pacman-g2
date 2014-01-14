#  - macro_asciidoc2man(inputfile outputfile)
#
#  Create a manpage with asciidoc.
#  Example: macro_asciidoc2man(foo.txt foo.1)
#
# Copyright (c) 2006, Andreas Schneider, <mail@cynapses.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

macro(ASCIIDOC2MAN _a2m_input _a2m_output _a2m_install_dir)

   set(_manFile ${CMAKE_CURRENT_BINARY_DIR}/${_a2m_output})
   add_custom_command(OUTPUT ${_manFile}
       COMMAND ${ASCIIDOC_A2X_EXECUTABLE} -D ${CMAKE_CURRENT_BINARY_DIR} -f manpage ${CMAKE_CURRENT_SOURCE_DIR}/${_a2m_input}
       WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
       DEPENDS ${_a2m_input}
   )
   
   string(REGEX REPLACE "/" "." no_slash_dir ${_a2m_install_dir})
   
   add_custom_target(${no_slash_dir}${_a2m_output} ALL DEPENDS ${_manFile})

   install(FILES  ${CMAKE_CURRENT_BINARY_DIR}/${_a2m_output} DESTINATION ${_a2m_install_dir})

endmacro(ASCIIDOC2MAN _a2m_input _a2m_output _a2m_install_dir)


macro(ASCIILOCALDOC2MAN _a2m_input _a2m_output _a2m_install_dir _lang)

   set(_manFile ${CMAKE_CURRENT_BINARY_DIR}/${_lang}/${_a2m_output})
   add_custom_command(OUTPUT ${_manFile}
       COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${_lang}
       COMMAND ${ASCIIDOC_A2X_EXECUTABLE} -D ${CMAKE_CURRENT_BINARY_DIR}/${_lang} -f manpage ${CMAKE_CURRENT_SOURCE_DIR}/local_mans/${_lang}/${_a2m_input}
       WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
       DEPENDS ${_a2m_input}
   )
   
   string(REGEX REPLACE "/" "." no_slash_dir ${_a2m_install_dir})
   
   add_custom_target(${no_slash_dir}${_lang}${_a2m_output} ALL DEPENDS ${_manFile})

   install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_lang}/${_a2m_output} DESTINATION ${_a2m_install_dir})

endmacro(ASCIILOCALDOC2MAN _a2m_input _a2m_output _a2m_install_dir _lang)
