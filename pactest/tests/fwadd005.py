self.description = "Install a package without depcheck"

p1 = pmpkg("kdelibs", "1.0-1")
p1.requiredby = ['foo']
self.addpkg2db("local", p1)

p2 = pmpkg("kfoo")
p2.depends = ["kdelibs"]
self.addpkg(p2)

self.args = "-Ud %s" % p2.filename()

self.addrule("PKG_REQUIREDBY=kdelibs|foo")
