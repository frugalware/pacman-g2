self.description = "Test absence of available package"

# DO NOT USE 'dummy' as package name, is reserved for internal in deptest code

sp = pmpkg("dummy1")
self.addpkg2db("sync", sp)

self.args = "-T dummy1"

self.addrule("PACMAN_RETCODE=127")
self.addrule("!PKG_EXIST=dummy1")
