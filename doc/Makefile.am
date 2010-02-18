if HAS_ASCIIDOC

man3 = libpacman.3
man5 = FrugalBuild.5

man8 = makepkg.8 pacman-g2.8
endif

localized_MANS = $(man5) $(man8) $(man3)

localized_LANGS = $(shell grep '\[po4a_langs\]' po4a.cfg |sed 's/\[po4a_langs\] //')

convert_MAN = \
	      if echo $(1)|grep -q /; then \
	      iconv -t $(shell locale -a |grep "$(shell dirname $(1))_.*\."|sed -n 's/.*\.//;1 p') -f UTF8 < $(1) > tmp; mv tmp $(1); \
	      fi

if HAS_ASCIIDOC
man_MANS = $(localized_MANS) $(man3_MANS)
if HAS_DOXYGEN
man_MANS += libpacman.3 man3/*
endif
else
man_MANS =
endif

if HAS_ASCIIDOC
%.3: %.txt
	a2x -a lang=$(shell dirname $@|tr -d .|sed 's/^$$/en/') -f manpage $^
	$(call convert_MAN,$@)

%.5: %.txt
	a2x -a lang=$(shell dirname $@|tr -d .|sed 's/^$$/en/') -f manpage $^
	$(call convert_MAN,$@)

%.8: %.txt
	a2x -a lang=$(shell dirname $@|tr -d .|sed 's/^$$/en/') -f manpage $^
	$(call convert_MAN,$@)

all: $(man_MANS) localized_mans

if HAS_PO4A
localized_mans: $(shell po4a po4a.cfg >&2; for i in $(localized_LANGS); do for j in $(localized_MANS); do if [ -e $$i/`echo $$j|sed 's/\..//'`.txt ]; then echo $$i/$$j; fi; done; done)
install-data-local: install-man5-local install-man8-local
else
localized_mans:
endif
else
localized_mans:
endif


install-man5-local:
	for i in $(localized_LANGS); do \
		for j in $(man5); do \
			if [ -e $$i/$$j ]; then \
				test -z "$(mandir)/$$i/man5" || $(MKDIR_P) "$(DESTDIR)$(mandir)/$$i/man5"; \
				$(INSTALL_DATA) $$i/$$j "$(DESTDIR)$(mandir)/$$i/man5/$$j"; \
			fi; \
		done; \
	done

install-man8-local:
	for i in $(localized_LANGS); do \
		for j in $(man8); do \
			if [ -e $$i/$$j ]; then \
			test -z "$(mandir)/$$i/man8" || $(MKDIR_P) "$(DESTDIR)$(mandir)/$$i/man8"; \
				$(INSTALL_DATA) $$i/$$j "$(DESTDIR)$(mandir)/$$i/man8/$$j"; \
			fi; \
		done; \
	done
