EXTRA_DIST = frugalware-current

install-data-hook:
	mkdir -p $(DESTDIR)$(sysconfdir)/pacman-g2/repos ; \
	for j in $(EXTRA_DIST); do \
	    $(INSTALL) -c -m 644 $(srcdir)/$$j $(DESTDIR)$(sysconfdir)/pacman-g2/repos/$$j ; \
	done

uninstall-hook:
	for j in $(EXTRA_DIST); do \
	    rm -f $(DESTDIR)$(sysconfdir)/pacman-g2/repos/$$j ; \
	done
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(sysconfdir)/pacman-g2/repos
