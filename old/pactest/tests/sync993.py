self.description = "System upgrade (Ignoring version on provides)"

sp1 = pmpkg("pkg1", "1.0-2")
sp1.depends = ["pkg3>=2.0"]
self.addpkg2db("sync", sp1);

lp1 = pmpkg("pkg1")
self.addpkg2db("local", lp1)

lp2 = pmpkg("pkg2", "1.0-1")
lp2.conflicts = ["pkg3"]
lp2.provides = ["pkg3"]
self.addpkg2db("sync", lp2)
self.addpkg2db("local", lp2)

self.args = "-Su"

self.addrule("PACMAN_RETCODE=0")
self.addrule("PKG_MODIFIED=pkg1")
self.addrule("PKG_EXIST=pkg2")
