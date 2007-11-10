man3_MANS = libpacman.3

man5_MANS = FrugalBuild.5

man8_MANS = makepkg.8 pacman-g2.8

localized_MANS = $(man5_MANS) $(man8_MANS)

localized_LANGS = $(shell grep '\[po4a_langs\]' po4a.cfg |sed 's/\[po4a_langs\] //')

man_MANS = $(localized_MANS) $(man3_MANS)

%.3: %.txt
	a2x -f manpage $^

%.5: %.txt
	a2x -f manpage $^

%.8: %.txt
	a2x -f manpage $^

all: $(man_MANS) po4a localized_mans

po4a:
	po4a -k 0 po4a.cfg

localized_mans: $(shell for i in $(localized_LANGS); do for j in $(localized_MANS); do echo $$i/$$j; done; done)

install-data-local: install-man5-local install-man8-local

install-man5-local:
	for i in $(localized_LANGS); do \
		test -z "$(mandir)/$$i/man5" || $(MKDIR_P) "$(DESTDIR)$(mandir)/$$i/man5"; \
		for j in $(man5_MANS); do \
			$(INSTALL_DATA) $$j "$(DESTDIR)$(mandir)/$$i/man5/$$j"; \
		done; \
	done

install-man8-local:
	for i in $(localized_LANGS); do \
		test -z "$(mandir)/$$i/man8" || $(MKDIR_P) "$(DESTDIR)$(mandir)/$$i/man8"; \
		for j in $(man8_MANS); do \
			$(INSTALL_DATA) $$j "$(DESTDIR)$(mandir)/$$i/man8/$$j"; \
		done; \
	done

man_MANS += man3/*

EXTRA_DIST = $(man_MANS)
