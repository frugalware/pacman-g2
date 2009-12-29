self.description = "Detect file conflict in case two symlinks conflicts."

lp = pmpkg("pkg")
lp.files = ["bin/dummy",
	"bin/symlink -> dummy"]
self.addpkg2db("local", lp)

p = pmpkg("pkg", "2.0")
p.files = ["bin/dummy",
	"bin/symlink -> dummy",
	"bin/symlink2 -> symlink"]
self.addpkg(p)

self.filesystem = ["bin/file",
	"bin/symlink2 -> dummy"]

self.args = "-U %s" % p.filename()

self.addrule("!PACMAN_RETCODE=0")
