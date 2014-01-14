  ASCIIDOC2MAN(makepkg.txt makepkg.8 share/man/man8)
  ASCIIDOC2MAN(pacman-g2.txt pacman-g2.8 share/man/man8)
  ASCIIDOC2MAN(FrugalBuild.txt FrugalBuild.5 share/man/man5)
  ASCIIDOC2MAN(libpacman.txt libpacman.3 share/man/man3)

if(HAS_PO4A)
  execute_process(
      COMMAND ${PO4A_EXECUTABLE} po4a.cfg
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  file (STRINGS "po/LINGUAS" languages)
  foreach(lang IN LISTS languages)
    ASCIILOCALDOC2MAN(makepkg.txt makepkg.8 share/man/${lang}/man8 ${lang})
    ASCIILOCALDOC2MAN(pacman-g2.txt pacman-g2.8 share/man/${lang}/man8 ${lang})
    ASCIILOCALDOC2MAN(FrugalBuild.txt FrugalBuild.5 share/man/${lang}/man5 ${lang})
  endforeach(lang)  
endif(HAS_PO4A)
