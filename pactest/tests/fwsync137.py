self.description = "Sysupgrade with a sync package featuring the stick property"

lp = pmpkg("dummy", "1.0-1")

self.addpkg2db("local", lp)

sp = pmpkg("dummy", "1.0-2")
sp.stick = 1

self.addpkg2db("sync", sp)

self.args = "-Su"

self.addrule("PACMAN_RETCODE=0")
self.addrule("PKG_VERSION=dummy|1.0-1")
