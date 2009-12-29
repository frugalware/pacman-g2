self.description = "Remove a file because of removes()"

self.filesystem = ["bin/dummy"]

p = pmpkg("p")
p.removes = ["bin/dummy"]
p.files = ["bin/dummy"]
self.addpkg2db("sync", p)

self.args = "-S p"

self.addrule("PKG_EXIST=p")
self.addrule("FILE_EXIST=bin/dummy")
