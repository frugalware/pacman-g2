self.description = "Test presence of a package"

# DO NOT USE 'dummy' as package name, is reserved for internal in deptest code

lp = pmpkg("dummy1")
self.addpkg2db("local", lp)

self.args = "-T dummy1"

self.addrule("PACMAN_RETCODE=0")
self.addrule("!PKG_MODIFIED=dummy1")
self.addrule("PKG_VERSION=dummy1|1.0-1")
