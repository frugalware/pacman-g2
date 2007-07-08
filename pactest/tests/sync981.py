self.description = "Remove with provides"

lp1 = pmpkg("hunspell")
lp1.depends = ["hunspell-dict"]
lp1.requiredby = ['hunspell-hu', 'hunspell-de']
self.addpkg2db("local", lp1);

lp2 = pmpkg("hunspell-de")
lp2.provides = ["hunspell-dict"]
lp2.depends = ["hunspell"]
lp2.requiredby = ['hunspell']
self.addpkg2db("local", lp2)

lp3 = pmpkg("hunspell-hu")
lp3.provides = ["hunspell-dict"]
lp2.depends = ["hunspell"]
lp2.requiredby = ['hunspell']
self.addpkg2db("local", lp3)

self.args = "-R hunspell-de hunspell-hu"

self.addrule("PACMAN_RETCODE=1")
self.addrule("PKG_EXIST=hunspell")
self.addrule("PKG_EXIST=hunspell-de")
self.addrule("PKG_EXIST=hunspell-hu")
