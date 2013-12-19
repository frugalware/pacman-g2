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


import os
import time

import pmtest

class pmenv:
	"""Environment object
	"""

	def __init__(self, root = "root"):
		self.root = os.path.abspath(root)
		self.pacman = {
			"bin": "pacman-g2",
			"debug": 0,
			"gdb": 0,
			"valgrind": 0
		}
		self.testcases = []

	def __str__(self):
		return "root = %s\n" \
		       "pacman = %s" \
		       % (self.root, self.pacman)

	def addtest(self, testcase):
		"""
		"""

		if not os.path.isfile(testcase):
			err("file %s not found" % testcase)
			return
		test = pmtest.pmtest(testcase, self.root)
		self.testcases.append(test)

	def run(self):
		"""
		"""

		for t in self.testcases:
			print "=========="*8
			print "Running '%s'" % t.name.strip(".py")

			t.load()
			print t.description
			print "----------"*8

			t.generate()
			# Hack for mtimes consistency
			modified = 0
			for i in t.rules:
				if i.rule.find("MODIFIED") != -1:
					modified = 1
			if modified:
				time.sleep(3)

			t.run(self.pacman)

			t.check()
			print "==> Test result"
			if t.failures == 0:
				print "\tPASSED"
			else:
				print "\tFAILED"
			print

	def results(self):
		"""
		"""

		self.successes = 0
		self.failures = 0
		self.total = len(self.testcases)

		print "=========="*8
		print "Results"
		print "----------"*8
		for test in self.testcases:
			if test.failures == 0:
				print "[PASSED]",
				self.successes += 1
			else:
				print "[FAILED]",
				self.failures += 1
			print test.name.strip(".py").ljust(38),
			print "Rules:",
			print "OK = %2u KO = %2u SKIP = %2u" % (test.successes, test.failures, test.skippeds)
		print "----------"*8
		print "TOTAL  = %3u" % self.total
		if self.total:
			print "PASSED = %3u (%6.2f%%)" % (self.successes, float(self.failures)*100/self.total)
			print "FAILED = %3u (%6.2f%%)" % (self.failures, float(self.failures)*100/self.total)
		print
		return self


if __name__ == "__main__":
	env = pmenv("/tmp")
	print env
