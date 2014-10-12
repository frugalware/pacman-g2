# Adds an autoconf style enable/disable boolean option
def AddBooleanOption(env, name, msg):
	from SCons.Script import AddOption
	# Add the enable version.
	AddOption(
		'--enable-%s' % name,
		help = 'enable %s' % msg,
		dest = name,
		action = 'callback',
		callback = (lambda a,b,c,d: env.__setitem__(name, True)),
		default=None,
	)
	# Add the disable version.
	AddOption(
		'--disable-%s' % name,
		help = 'disable %s' % msg,
		dest = name,
		action = 'callback',
		callback = (lambda a,b,c,d: env.__setitem__(name, False)),
		default=None,
	)

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

# -%- lang: python -%-
