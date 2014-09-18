self.description = "Test absence of a package"

# DO NOT USE 'dummy' as package name, is reserved for internal in deptest code

self.args = "-T dummy1"

self.addrule("PACMAN_RETCODE=127")
self.addrule("!PKG_EXIST=dummy1")
