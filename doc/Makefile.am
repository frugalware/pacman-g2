SUBDIRS = hu

all: makepkg.8 pacman.8 po4a

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

man_MANS = pacman.8 makepkg.8 libalpm.3

if HAS_DOXYGEN
man_MANS += man3/*.3
endif

EXTRA_DIST = $(man_MANS)

po4a:
	@$(NORMAL_INSTALL)
	po4a -k 0 --variable "srcdir=$(srcdir)/" $(srcdir)/po4a.cfg
