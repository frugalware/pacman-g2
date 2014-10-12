from scons_util import *

# Global project variables
package     = 'pacman-g2'
version     = '3.90.0'
description = 'package manager'
config_file = '/etc/pacman-g2.conf'
package_url = 'www.frugalware.org'
bugs_email  = 'frugalware-devel@frugalware.org'
config_hdr  = 'config.h'
build_cfg   = 'build.conf'
archs       = [ 'i686', 'x86_64' ]

env = Environment()

vars = Variables(build_cfg)

vars.AddVariables(
	('debug', 'compile with debugging', False),
	('static', 'compile static objects', False),
	('shared', 'compile shared objects', False),
)

vars.Update(env)

AddBooleanOption(env, 'debug', 'debugging compilation')
AddBooleanOption(env, 'static', 'static object compilation')
AddBooleanOption(env, 'shared', 'shared object compilation')

# Set flags used by all modules under all conditions.
env.Append(CFLAGS = ['-std=c99'])
env.Append(CXXFLAGS = ['-std=c++11', '-fpermissive'])
env.Append(CPPDEFINES = ['_GNU_SOURCE'])
env.Append(CPPPATH = ['#.', '#lib/libpacman'])

# Set flags according to the debugging settings.
if env['debug']:
	env.Append(CCFLAGS = ['-g'])
else:
	env.Append(CPPDEFINES = ['NDEBUG'])

cfg = Configure(
	env,
	custom_tests =
	{
		'CheckPKGConfig' : CheckPKGConfig,
		'CheckPKG'       : CheckPKG,
		'CheckPlatform'  : CheckPlatform,
	},
	config_h = config_hdr
)

if not cfg.CheckPKGConfig('0.15.0'):
	print('pkg-config >= 0.15.0 not found.')
	Exit(1)

if not cfg.CheckPKG('libarchive'):
	print('libarchive not found.')
	Exit(1)

cfg.Define('HAVE_ARCHIVE', 1)

if not cfg.CheckPKG('libcurl'):
	print('libcurl not found.')
	Exit(1)

cfg.Define('HAVE_CURL', 1)

if not cfg.CheckPlatform():
	print('Unsupported computer platform.')
	Exit(1)

if not cfg.env['HOST_ARCH'] in archs:
	print('Unsupported CPU architecture (%s).' % cfg.env['HOST_ARCH'])
	Exit(1)

if not cfg.CheckCC():
	print('No functional C compiler found.')
	Exit(1)

if not cfg.CheckCXX():
	print('No functional C++ compiler found.')
	Exit(1)

cfg.Define('PACKAGE', '"%s"' % package)
cfg.Define('VERSION', '"%s"' % version)
cfg.Define('PACKAGE_VERSION', '"%s"' % version)
cfg.Define('PACKAGE_BUGREPORT', '"%s"' % bugs_email)
cfg.Define('PACKAGE_NAME', '"%s %s"' % (package, description))
cfg.Define('PACKAGE_STRING', '"%s %s %s"' % (package, description, version))
cfg.Define('PACKAGE_TARNAME', '"%s"' % package)
cfg.Define('PACKAGE_URL', '"%s"' % package_url)
cfg.Define('PACCONF', '"%s"' % config_file)
cfg.Define('PM_VERSION', '"0.%s"' % version)
cfg.Define('PM_DEFAULT_BYTES_PER_BLOCK', 10240)

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

cfg.CheckFunc('gettext')
cfg.CheckFunc('dcgettext')
cfg.CheckFunc('iconv')
cfg.CheckFunc('strverscmp')

env = cfg.Finish()

vars.Save(build_cfg, env)

env.Export(['env'])

env.SConscript(dirs = ['lib', 'src'])

# -%- lang: python -%-
