#!/usr/bin/python

env = Environment()

def CheckPKGConfig(ctx, version):
	ctx.Message('Checking for pkg-config... ')
	rv = ctx.TryAction('pkg-config --atleast-pkgconfig-version=\'%s\'' % version)[0]
	ctx.Result(rv)
	return rv

def CheckPKG(ctx, name):
	ctx.Message('Checking for %s... ' % name)
	rv = ctx.TryAction('pkg-config --exists \'%s\'' % name)[0]
	ctx.Result(ret)
	return ret

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
