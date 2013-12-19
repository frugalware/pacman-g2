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

	def __str__(self):
		return "rule = %s" % self.rule

	def check(self, test):
		"""
		"""

		inverted = False
		self.result = SUCCESS

		[name, args] = self.rule.split("=")
		if name[0] == "!":
			inverted = True
			name = name.lstrip("!")
		[kind, case] = name.split("_")
		if "|" in args:
			[key, value] = args.split("|", 1)
		else:
			[key, value] = [args, None]

		if kind == "PACMAN":
			if case == "RETCODE":
				if test.returncode != int(key):
					self.result = FAILURE
			elif case == "OUTPUT":
				if not grep(os.path.join(test.root, LOGFILE), key):
					self.result = FAILURE
			else:
				self.result = SKIPPED
		elif kind == "PKG":
			localdb = test.db["local"]
			newpkg = localdb.db_read(key)
			if not newpkg:
				self.result = FAILURE
			else:
				dbg("newpkg.checksum : %s" % newpkg.checksum)
				dbg("newpkg.mtime    : %s" % newpkg.mtime)
				if case == "EXIST":
					self.result = 1
				elif case == "MODIFIED":
					if not localdb.ispkgmodified(newpkg):
						self.result = FAILURE
				elif case == "VERSION":
					if value != newpkg.version:
						self.result = FAILURE
				elif case == "GROUPS":
					if not value in newpkg.groups:
						self.result = FAILURE
				elif case == "DEPENDS":
					if not value in newpkg.depends:
						self.result = FAILURE
				elif case == "REQUIREDBY":
					if not value in newpkg.requiredby:
						self.result = FAILURE
				elif case == "REASON":
					if not newpkg.reason == int(value):
						self.result = FAILURE
				elif case == "FILES":
					if not value in newpkg.files:
						self.result = FAILURE
				elif case == "BACKUP":
					found = 0
					for f in newpkg.backup:
						name, md5sum = f.split("\t")
						if value == name:
							found = 1
					if not found:
						self.result = FAILURE
				else:
					self.result = SKIPPED
		elif kind == "FILE":
			filename = os.path.join(test.root, key)
			if case == "EXIST":
				if not os.path.isfile(filename):
					self.result = FAILURE
			else:
				if case == "MODIFIED":
					for f in test.files:
						if f.name == key:
							if not f.ismodified():
								self.result = FAILURE
				elif case == "PACNEW":
					if not os.path.isfile("%s%s" % (filename, PM_PACNEW)):
						self.result = FAILURE
				elif case == "PACORIG":
					if not os.path.isfile("%s%s" % (filename, PM_PACORIG)):
						self.result = FAILURE
				elif case == "PACSAVE":
					if not os.path.isfile("%s%s" % (filename, PM_PACSAVE)):
						self.result = FAILURE
				else:
					self.result = SKIPPED
		elif kind == "LINK":
			filename = os.path.join(test.root, key)
			if case == "EXIST":
				if not os.path.islink(filename):
					self.result = FAILURE
		else:
			self.result = SKIPPED

		if inverted:
			if self.result == SUCCESS:
				self.result = FAILURE
			elif self.result == FAILURE:
				self.result = SUCCESS
		return self.result

