self.description = "Upgrade a package (lesser version)"

lp = pmpkg("dummy", "1.0-2")
lp.files = ["bin/dummy",
            "usr/man/man1/dummy.1"]
self.addpkg2db("local", lp)

p = pmpkg("dummy")
p.files = ["bin/dummy",
           "usr/man/man1/dummy.1"]
self.addpkg(p)

self.args = "-U %s" % p.filename()

self.addrule("PKG_MODIFIED=dummy")
self.addrule("PKG_VERSION=dummy|1.0-1")
for f in lp.files:
	self.addrule("FILE_MODIFIED=%s" % f)
