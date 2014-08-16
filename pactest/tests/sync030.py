self.description = "Install a package using sync db name"

sp = pmpkg("dummy")
sp.files = ["bin/dummy",
            "usr/man/man1/dummy.1"]
self.addpkg2db("sync", sp)

self.args = "-S sync/dummy"

self.addrule("PKG_EXIST=dummy")
for f in sp.files:
	self.addrule("FILE_EXIST=%s" % f)
