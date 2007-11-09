all: po4a makepkg.8 FrugalBuild.8 pacman-g2.8

po4a:
if HAS_PO4A
	@$(NORMAL_INSTALL)
	po4a -k 0 po4a.cfg
endif

if HAS_MAN2HTML
makepkg.8:
	man2html makepkg.8 > html/makepkg.8.html

FrugalBuild.8:
	man2html FrugalBuild.8 > html/FrugalBuild.8.html

pacman-g2.8:
	man2html pacman-g2.8 > html/pacman-g2.8.html
endif

clean:
	rm -f html/*.html
	rm -f man3/*.3

man_MANS = pacman-g2.8 makepkg.8 FrugalBuild.8 libpacman.3

if HAS_DOXYGEN
man_MANS += man3/*.3
endif

EXTRA_DIST = $(man_MANS)
