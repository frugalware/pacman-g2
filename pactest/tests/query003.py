self.description = "Query search an invalid package"

p = pmpkg("foobar")
p.files = ["bin/foobar"]
self.addpkg2db("local", p)

self.args = "-Qs foobaz"

self.addrule("!PACMAN_RETCODE=0")
