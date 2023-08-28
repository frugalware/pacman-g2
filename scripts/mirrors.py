from xml.dom import minidom
import sys

class Mirror (object):
	def __init__(self):
		self.types = []
	def dumpattr(self, what, dest):
		dest.write("# ")
		for i in range(len(what)+4):
			dest.write("-")
		dest.write("\n")
		dest.write("# - %s -\n" % what.upper())
		dest.write("# ")
		for i in range(len(self.country)+4):
			dest.write("-")
		dest.write("\n")

mirrors = {}
try:
	ver = sys.argv[1]
	repo = sys.argv[2]
	input = sys.argv[3]
	out = sys.argv[4]
except IndexError:
	raise Exception("usage example: python %s frugalware-stable frugalware input.xml output" % sys.argv[0])

xmldoc = minidom.parse(input)
for i in xmldoc.getElementsByTagName('mirror'):
	if not i.getElementsByTagName('id'):
		continue
	m = Mirror()
	for j in i.getElementsByTagName('type'):
		m.types.append(j.firstChild.toxml())
	for j in ['id', 'path', 'ftp_path', 'http_path', 'rsync_path', 'country', 'supplier', 'bandwidth']:
		if i.getElementsByTagName(j):
			if i.getElementsByTagName(j)[0].firstChild:
				m.__setattr__(j, i.getElementsByTagName(j)[0].firstChild.toxml())
			else:
				m.__setattr__(j, "")
	try:
		mirrors[m.country]
	except KeyError:
		mirrors[m.country] = []
	mirrors[m.country].append(m)
countries = list(mirrors.keys())
countries.sort()
sock = open(out, "w")
sock.write("""#
# %s repository
#

[%s]
""" % (ver, repo))
for i in countries:
	mirrors[i][0].dumpattr(mirrors[i][0].country, sock)
	for j in mirrors[i]:
		sock.write("# %s (%s)\n" % (j.supplier, j.bandwidth))
		try:
			if "ftp" in j.types:
				proto = "ftp"
				domain = "ftp"
				path = j.ftp_path
			else:
				proto = "http"
				domain = "www"
				path = j.http_path
		except AttributeError:
			path = j.path
		if len(path):
			path = "/%s" % path
		sock.write("Server = %s://%s%s.frugalware.org%s/%s/frugalware-@CARCH@\n" %
				(proto, domain, j.id, path, ver))
sock.close()
