all: po4a makepkg.8 FrugalBuild.8 pacman-g2.8

po4a:
if HAS_PO4A
	@$(NORMAL_INSTALL)
	po4a -k 0 po4a.cfg
endif

clean:
	rm -f man3/*.3

man_MANS = pacman-g2.8 makepkg.8 FrugalBuild.8 libpacman.3

if HAS_DOXYGEN
man_MANS += man3/*.3
endif

EXTRA_DIST = $(man_MANS)
