#translations
if(EXISTS "${ROOT}/configuration/${customer}/configuration.${project_name}.xml")
  file (STRINGS "LINGUAS" languages)
  foreach(lang IN LISTS languages)
    GETTEXT_PROCESS_PO_FILES(${lang} ALL INSTALL_DESTINATION share/locale PO_FILES ${lang}/pacman-g2.po)
  endforeach(lang)
else()
  message(WARNING "file LINGUAS not present")
endif()


