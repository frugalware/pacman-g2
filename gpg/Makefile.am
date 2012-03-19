install:
	mkdir -p $(DESTDIR)/etc/pacman-g2/keys
	chmod 0700 $(DESTDIR)/etc/pacman-g2/keys
	gpg --homedir $(DESTDIR)/etc/pacman-g2/keys --list-keys
	gpg --homedir $(DESTDIR)/etc/pacman-g2/keys --import $(top_srcdir)/src/pacman-g2.key
	ln -sf pubring.gpg $(DESTDIR)/etc/pacman-g2/keys/trustedkeys.gpg
	rm -f $(DESTDIR)/etc/pacman-g2/keys/*~
