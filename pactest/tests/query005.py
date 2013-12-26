self.description = "Query search a package ignoring case with a partial match"

p = pmpkg("foobar")
p.files = ["bin/foobar"]
self.addpkg2db("local", p)

self.args = "-Qs Oba"

self.addrule("PACMAN_RETCODE=0")
