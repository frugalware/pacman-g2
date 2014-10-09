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
