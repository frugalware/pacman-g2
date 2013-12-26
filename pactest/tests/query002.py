self.description = "Query an invalid package"

p = pmpkg("foobar")
p.files = ["bin/foobar"]
self.addpkg2db("local", p)

self.args = "-Q foobaz"

self.addrule("!PACMAN_RETCODE=0")
