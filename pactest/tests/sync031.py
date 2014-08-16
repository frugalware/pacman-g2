self.description = "Install a specific package using sync db name"

sp1 = pmpkg("dummy", "1.0-1")
sp1.files = ["bin/dummy1"]
self.addpkg2db("sync1", sp1)

sp2 = pmpkg("dummy", "1.0-2")
sp2.files = ["bin/dummy2"]
self.addpkg2db("sync2", sp2)

self.args = "-S sync2/dummy"

self.addrule("PKG_EXIST=dummy")
self.addrule("PKG_VERSION=dummy|1.0-2")
for f in sp2.files:
	self.addrule("FILE_EXIST=%s" % f)
