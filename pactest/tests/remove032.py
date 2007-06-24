self.description = "Remove a group from the local db"

sp1 = pmpkg("pkg1")
sp1.groups = ["grp"]

sp2 = pmpkg("pkg2")
sp2.groups = ["grp"]

sp3 = pmpkg("pkg3")
sp3.groups = ["grp"]

for p in sp1, sp2, sp3:
	self.addpkg2db("local", p);

self.args = "-R grp"

self.addrule("PACMAN_RETCODE=0")
for p in sp1, sp2, sp3:
	self.addrule("!PKG_EXIST=%s" % p.name)
