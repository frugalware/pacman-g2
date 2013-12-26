self.description = "Query search a package ignoring case"

p = pmpkg("foobar")
p.files = ["bin/foobar"]
self.addpkg2db("local", p)

self.args = "-Qs FoObAr"

self.addrule("PACMAN_RETCODE=0")
