self.description = "Install a group from a sync db while there are other group-like packages"

sp1 = pmpkg("pkg1")
sp1.groups = ["grp"]

sp2 = pmpkg("pkg2")
sp2.groups = ["grp"]

sp3 = pmpkg("pkg3")
sp3.groups = ["grp"]

sp4 = pmpkg("foo-grp")

for p in sp1, sp2, sp3, sp4:
	self.addpkg2db("sync", p);

self.args = "-S grp"

self.addrule("PACMAN_RETCODE=0")
for p in sp1, sp2, sp3:
	self.addrule("PKG_EXIST=%s" % p.name)
self.addrule("!PKG_EXIST=%s" % sp4.name)
