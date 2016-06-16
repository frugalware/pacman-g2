self.description = "Upgrade a package with a filesystem conflict (--force)"

sp = pmpkg("dummy")
sp.files = ["bin/dummy", "usr/man/man1/dummy.1"]
self.addpkg(sp)

lp = pmpkg("dummy")
lp.files = ["usr/man/man1/dummy.1"]
self.addpkg2db("local", lp)

self.filesystem = ["bin/dummy"]

self.args = "-Uf %s" % sp.filename()

self.addrule("PACMAN_RETCODE=0")
self.addrule("PKG_EXIST=dummy")
self.addrule("FILE_MODIFIED=bin/dummy")
self.addrule("FILE_EXIST=usr/man/man1/dummy.1")
