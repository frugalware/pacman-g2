all: makepkg.8 pacman.8 po4a

if HAS_PO4A
SUBDIRS = hu
endif

if HAS_MAN2HTML
makepkg.8:
	man2html makepkg.8 > html/makepkg.8.html
endif

if HAS_MAN2HTML
pacman.8:
	man2html pacman.8 > html/pacman.8.html
endif

clean:
	rm -rf html/*
	rm -rf hu/*.8

man_MANS = pacman.8 makepkg.8 libalpm.3

if HAS_DOXYGEN
man_MANS += man3/*.3
endif

EXTRA_DIST = $(man_MANS)

po4a:
if HAS_PO4A
	@$(NORMAL_INSTALL)
	po4a -k 0 po4a.cfg
endif
