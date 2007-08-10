self.description = "Sysupgrade with a replacement for a new package up to date"

sp = pmpkg("pkg2")
sp.replaces = ["pkg1"]

self.addpkg2db("sync", sp)

lp1 = pmpkg("pkg1")
lp2 = pmpkg("pkg2")

for p in lp1, lp2:
	self.addpkg2db("local", p)

self.args = "-Su"

self.addrule("PACMAN_RETCODE=0")
self.addrule("!PKG_EXIST=pkg1")
self.addrule("PKG_EXIST=pkg2")
