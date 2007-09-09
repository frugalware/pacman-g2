self.description = "Explicit install of a pkg should affect the reason"

sp = pmpkg("pkg", "1.0-2")

self.addpkg2db("sync", sp)

lp = pmpkg("pkg")
lp.reason = 1
self.addpkg2db("local", lp)

self.args = "-S pkg"

self.addrule("PACMAN_RETCODE=0")
self.addrule("PKG_VERSION=pkg|1.0-2")
self.addrule("PKG_REASON=pkg|0")
