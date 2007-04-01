AUTOMAKE_OPTIONS=foreign

mandir = @mandir@/hu
if HAS_PO4A
man_MANS = pacman-g2.8 makepkg.8 FrugalBuild.8
endif

$(man_MANS):

if HAS_PO4A
EXTRA_DIST = pacman-g2.8 makepkg.8 FrugalBuild.8
endif

distclean:
	rm -f hu/*.8
