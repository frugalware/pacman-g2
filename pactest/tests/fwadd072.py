self.description = "dir should not overwrite a symlink"

p1 = pmpkg("perl")
p1.files = ["usr/lib/perl5/current/",
	    "usr/lib/perl5/5.10.0 -> current"]
self.addpkg(p1)

p2 = pmpkg("module")
p2.files = ["usr/lib/perl5/5.10.0/"]
self.addpkg(p2)

self.args = "-U %s %s" % (p1.filename(), p2.filename())
self.addrule("PACMAN_RETCODE=0")
self.addrule("PKG_EXIST=perl")
self.addrule("PKG_EXIST=module")
self.addrule("LINK_EXIST=usr/lib/perl5/5.10.0")
