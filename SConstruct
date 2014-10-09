#!/usr/bin/env python

# Custom test for finding pkg-config
def CheckPKGConfig(ctx, version):
	ctx.Message('Checking for pkg-config... ')
	rv = ctx.TryAction('pkg-config --atleast-pkgconfig-version=\'%s\'' % version)[0]
	ctx.Result(rv)
	return rv

# Custom test for finding a pkg-config package
def CheckPKG(ctx, name):
	ctx.Message('Checking for %s... ' % name)
	rv = ctx.TryAction('pkg-config --exists \'%s\'' % name)[0]
	ctx.Result(rv)
	return rv

# Custom test for finding the computer platform
def CheckPlatform(ctx):
	from platform import system, machine
	ctx.Message('Checking platform... ')
	rv = 1
	if system() == 'Linux':
		ctx.env['HOST_ARCH']   = machine()
		ctx.env['HOST_OS']     = system()
		ctx.env['TARGET_ARCH'] = machine()
		ctx.env['TARGET_OS']   = system()
	else:
		rv = 0
	ctx.Result(rv)
	return rv

# Enable shared builds
AddOption(
	'--enable-shared',
	dest='shared',
	action="store_true",
	default=True
)

# Disable shared builds
AddOption(
	'--disable-shared',
	dest='shared',
	action="store_false",
	default=True
)

# Enable static builds
AddOption(
	'--enable-static',
	dest='static',
	action="store_true",
	default=False
)

# Disable static builds
AddOption(
	'--disable-static',
	dest='static',
	action="store_false",
	default=False
)

# Enable debugging
AddOption(
	'--enable-debug',
	dest='debug',
	action="store_true",
	default=True
)

# Disable debugging
AddOption(
	'--disable-debug',
	dest='debug',
	action="store_false",
	default=True
)

# Get the build environment
env = Environment()

# Start the system testing phase
cfg = Configure(
	env,
	custom_tests = {
		'CheckPKGConfig' : CheckPKGConfig,
		'CheckPKG'       : CheckPKG,
		'CheckPlatform'  : CheckPlatform,
	},
	config_h = 'config.h'
)

# Check for pkg-config
if not cfg.CheckPKGConfig('0.15.0'):
	print('pkg-config >= 0.15.0 not found.')
	Exit(1)

# Check for libarchive
if not cfg.CheckPKG('libarchive'):
	print('libarchive not found.')
	Exit(1)

# Check for libcurl
if not cfg.CheckPKG('libcurl'):
	print('libcurl not found.')
	Exit(1)

# Check for supported platform
if not cfg.CheckPlatform():
	print('Unsupported computer platform.')
	Exit(1)

# Check for supported CPU architecture
if not cfg.env['HOST_ARCH'] in [ 'i686', 'x86_64' ]:
	print('Unsupported CPU architecture (%s).' % cfg.env['HOST_ARCH'])
	Exit(1)

# Check that certain headers exist
cfg.CheckHeader('dlfcn.h')
cfg.CheckHeader('inttypes.h')
cfg.CheckHeader('memory.h')
cfg.CheckHeader('stdint.h')
cfg.CheckHeader('stdlib.h')
cfg.CheckHeader('strings.h')
cfg.CheckHeader('string.h')
cfg.CheckHeader('sys/stat.h')
cfg.CheckHeader('sys/types.h')
cfg.CheckHeader('unistd.h')

# Check that certain functions exist
cfg.CheckFunc('gettext')
cfg.CheckFunc('dcgettext')
cfg.CheckFunc('iconv')
cfg.CheckFunc('strverscmp')

# Finish the system testing phase
env = cfg.Finish()

# Set C compiler only flags
env.Append(CFLAGS = '-std=c99')

# Set C++ compiler only flags
env.Append(CXXFLAGS = '-std=c++11')

# Set debugging or no debugging flags
if GetOption('debug'):
	env.Append(CCFLAGS = '-g')
else:
	env.Append(CCFLAGS = '-DNDEBUG')

# Set flags for libarchive
env.ParseConfig('pkg-config --cflags --libs libarchive')

# Set flags for libcurl
env.ParseConfig('pkg-config --cflags --libs libcurl')

# Set local include directories
env.Append(CPPPATH = 'lib/libpacman')

# flib source files
flib_sources = [
	'lib/libpacman/kernel/fobject.cpp',
	'lib/libpacman/kernel/fobject.h',
	'lib/libpacman/kernel/fsignal.h',
	'lib/libpacman/kernel/fstr.cpp',
	'lib/libpacman/kernel/fstr.h',
	'lib/libpacman/fstdlib/canonicalize.c',
	'lib/libpacman/fstring/basename.c',
	'lib/libpacman/fstring/case.c',
	'lib/libpacman/fstring/dirname.c',
	'lib/libpacman/hash/md5.c',
	'lib/libpacman/hash/md5driver.c',
	'lib/libpacman/hash/sha1.c',
	'lib/libpacman/io/archive.c',
	'lib/libpacman/io/ffilelock.c',
	'lib/libpacman/io/ftp.c',
	'lib/libpacman/util/fabstractlogger.cpp',
	'lib/libpacman/util/fabstractlogger.h',
	'lib/libpacman/util/falgorithm.h',
	'lib/libpacman/util/fdispatchlogger.cpp',
	'lib/libpacman/util/fdispatchlogger.h',
	'lib/libpacman/util/flist.h',
	'lib/libpacman/util/ffilelogger.cpp',
	'lib/libpacman/util/ffilelogger.h',
	'lib/libpacman/util/ffunctional.h',
	'lib/libpacman/util/flog.cpp',
	'lib/libpacman/util/flog.h',
	'lib/libpacman/util/fptrlist.cpp',
	'lib/libpacman/util/fptrlist.h',
	'lib/libpacman/util/fstringlist.cpp',
	'lib/libpacman/util/fsyslogger.cpp',
	'lib/libpacman/util/fsyslogger.h',
	'lib/libpacman/util/log.c',
	'lib/libpacman/util/time.c',
	'lib/libpacman/fstring.c',
]

# libpacman source files
libpacman_sources = [
	'lib/libpacman/db/fakedb.cpp',
	'lib/libpacman/db/localdb.cpp',
	'lib/libpacman/db/localdb_files.cpp',
	'lib/libpacman/db/syncdb.cpp',
	'lib/libpacman/package/fpmpackage.cpp',
	'lib/libpacman/package/packagecache.cpp',
	'lib/libpacman/config_parser.cpp',
	'lib/libpacman/conflict.cpp',
	'lib/libpacman/db.cpp',
	'lib/libpacman/database_cache.cpp',
	'lib/libpacman/deps.cpp',
	'lib/libpacman/error.cpp',
	'lib/libpacman/group.cpp',
	'lib/libpacman/handle.cpp',
	'lib/libpacman/package.cpp',
	'lib/libpacman/packages_transaction.cpp',
	'lib/libpacman/pacman.cpp',
	'lib/libpacman/server.cpp',
	'lib/libpacman/sync.cpp',
	'lib/libpacman/timestamp.h',
	'lib/libpacman/trans.cpp',
	'lib/libpacman/trans_sysupgrade.cpp',
	'lib/libpacman/util.cpp',
	'lib/libpacman/versioncmp.cpp',
]

# Add flib sources to libpacman sources for now because they are linked
# into the same library
libpacman_sources.extend(flib_sources)
