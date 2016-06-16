self.description = "Upgrade with wrong architecture"

sp = pmpkg("dummy")
sp.arch = "i586"
self.addpkg2db("sync", sp)

self.args = "-S dummy"

self.addrule("PACMAN_RETCODE=1")
self.addrule("!PKG_EXIST=dummy")
