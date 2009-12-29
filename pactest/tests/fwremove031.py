self.description = "Remove a package without depcheck"

p1 = pmpkg("kdelibs", "1.0-1")
p1.requiredby = ['foo']
self.addpkg2db("local", p1)

p2 = pmpkg("kfoo")
p2.depends = ["kdelibs"]
self.addpkg2db("local", p2)

self.args = "-Rd %s" % p2.name

self.addrule("PKG_REQUIREDBY=kdelibs|foo")
