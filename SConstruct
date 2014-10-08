#!/usr/bin/python

def CheckPKGConfig(ctx, version):
	ctx.Message('Checking for pkg-config... ')
	rv = ctx.TryAction('pkg-config --atleast-pkgconfig-version=\'%s\'' % version)[0]
	ctx.Result(rv)
	return rv

def CheckPKG(ctx, name):
	ctx.Message('Checking for %s... ' % name)
	rv = ctx.TryAction('pkg-config --exists \'%s\'' % name)[0]
	ctx.Result(rv)
	return rv

env = Environment()

cfg = Configure(
	env,
	{
		'CheckPKGConfig' : CheckPKGConfig,
		'CheckPKG'       : CheckPKG,
	}
)

if not cfg.CheckPKGConfig('0.15.0'):
	print('pkg-config >= 0.15.0 not found.')
	Exit(1)

if not cfg.CheckPKG('libarchive'):
	print('libarchive not found.')
	Exit(1)

if not cfg.CheckPKG('libcurl'):
	print('libcurl not found.')
	Exit(1)

env = cfg.Finish()

env.ParseConfig('pkg-config --cflags --libs libarchive libcurl')
env.Append(CFLAGS = '-std=c99')
env.Append(CXXFLAGS = '-std=c++11')

try:
	debug = int(ARGUMENTS.get('debug', 0))
except ValueError:
	print('debug is invalid.')
	Exit(1)

try:
	static = int(ARGUMENTS.get('static', 0))
except ValueError:
	print('static is invalid.')
	Exit(1)

try:
	shared = int(ARGUMENTS.get('shared', 1))
except ValueError:
	print('shared is invalid.')
	Exit(1)

if debug:
	env.Append(CCFLAGS = '-g')
