#translations
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LINGUAS")
  file (STRINGS "LINGUAS" languages)
    foreach(lang IN LISTS languages)
      GETTEXT_PROCESS_PO_FILES(${lang} ALL INSTALL_DESTINATION share/locale PO_FILES ${lang}/libpacman.po)
    endforeach(lang)
else(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LINGUAS")
  message(WARNING "file LINGUAS not present")
endif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LINGUAS")