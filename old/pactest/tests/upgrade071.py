self.description = "Upgrade with wrong architecture and --noarch"

sp = pmpkg("dummy")
sp.arch = "i586"
self.addpkg2db("sync", sp)

self.args = "-S --noarch dummy"

self.addrule("PACMAN_RETCODE=0")
self.addrule("PKG_EXIST=dummy")
