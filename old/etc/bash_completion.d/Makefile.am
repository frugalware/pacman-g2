EXTRA_DIST = pacman-g2

install-data-hook:
	mkdir -p $(DESTDIR)$(sysconfdir)/bash_completion.d ; \
	for j in $(EXTRA_DIST); do \
	    $(INSTALL) -c -m 644 $(srcdir)/$$j $(DESTDIR)$(sysconfdir)/bash_completion.d/$$j ; \
	done

uninstall-hook:
	for j in $(EXTRA_DIST); do \
	    rm -f $(DESTDIR)$(sysconfdir)/bash_completion.d/$$j ; \
	done
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(sysconfdir)/bash_completion.d
