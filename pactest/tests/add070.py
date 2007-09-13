self.description = "Remove a file because of removes()"

p1 = pmpkg("p1")
p1.files = ["usr/lib/generated"]
self.addpkg2db("local", p1)

p2 = pmpkg("p2")
p2.removes = ["usr/lib/generated"]
self.addpkg2db("sync", p2)

self.args = "-S p2"

self.addrule("PKG_EXIST=p1")
self.addrule("PKG_EXIST=p2")
self.addrule("!FILE_EXIST=usr/lib/generated")
