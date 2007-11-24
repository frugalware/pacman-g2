self.description = "Remove a file because of removes()"

p = pmpkg("p")
p.removes = ["bin/dummy"]
self.addpkg(p)

self.filesystem = ["bin/dummy"]

self.args = "-A %s" % p.filename()

self.addrule("PKG_EXIST=p")
self.addrule("!FILE_EXIST=bin/dummy")
