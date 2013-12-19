#! /usr/bin/python
#
#  Copyright (c) 2006 by Aurelien Foret <orelien@chez.com>
# 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
#  USA.


from util import *


SKIPPED = -1
FAILURE = 0
SUCCESS = 1

class pmrule:
	"""Rule object
	"""

	def __init__(self, rule):
		self.rule = rule
		self.result = None

	def __str__(self):
		return "rule = %s" % self.rule

	def check(self, root, retcode, localdb, files):
		"""
		"""

		inverted = False
		success = SUCCESS

		[test, args] = self.rule.split("=")
		if test[0] == "!":
			inverted = True
			test = test.lstrip("!")
		[kind, case] = test.split("_")
		if "|" in args:
			[key, value] = args.split("|", 1)
		else:
			[key, value] = [args, None]

		if kind == "PACMAN":
			if case == "RETCODE":
				if retcode != int(key):
					success = FAILURE
			elif case == "OUTPUT":
				if not grep(os.path.join(root, LOGFILE), key):
					success = FAILURE
			else:
				success = SKIPPED
		elif kind == "PKG":
			newpkg = localdb.db_read(key)
			if not newpkg:
				success = FAILURE
			else:
				dbg("newpkg.checksum : %s" % newpkg.checksum)
				dbg("newpkg.mtime    : %s" % newpkg.mtime)
				if case == "EXIST":
					success = 1
				elif case == "MODIFIED":
					if not localdb.ispkgmodified(newpkg):
						success = FAILURE
				elif case == "VERSION":
					if value != newpkg.version:
						success = FAILURE
				elif case == "GROUPS":
					if not value in newpkg.groups:
						success = FAILURE
				elif case == "DEPENDS":
					if not value in newpkg.depends:
						success = FAILURE
				elif case == "REQUIREDBY":
					if not value in newpkg.requiredby:
						success = FAILURE
				elif case == "REASON":
					if not newpkg.reason == int(value):
						success = FAILURE
				elif case == "FILES":
					if not value in newpkg.files:
						success = FAILURE
				elif case == "BACKUP":
					found = 0
					for f in newpkg.backup:
						name, md5sum = f.split("\t")
						if value == name:
							found = 1
					if not found:
						success = FAILURE
				else:
					success = SKIPPED
		elif kind == "FILE":
			filename = os.path.join(root, key)
			if case == "EXIST":
				if not os.path.isfile(filename):
					success = FAILURE
			else:
				if case == "MODIFIED":
					for f in files:
						if f.name == key:
							if not f.ismodified():
								success = FAILURE
				elif case == "PACNEW":
					if not os.path.isfile("%s%s" % (filename, PM_PACNEW)):
						success = FAILURE
				elif case == "PACORIG":
					if not os.path.isfile("%s%s" % (filename, PM_PACORIG)):
						success = FAILURE
				elif case == "PACSAVE":
					if not os.path.isfile("%s%s" % (filename, PM_PACSAVE)):
						success = FAILURE
				else:
					success = SKIPPED
		elif kind == "LINK":
			filename = os.path.join(root, key)
			if case == "EXIST":
				if not os.path.islink(filename):
					success = FAILURE
		else:
			success = SKIPPED

		if inverted:
			if success == SUCCESS:
				success = FAILURE
			elif success == FAILURE:
				success = SUCCESS
		self.result = success
		return success


if __name__ != "__main__":
	rule = pmrule("PKG_EXIST=dummy")
