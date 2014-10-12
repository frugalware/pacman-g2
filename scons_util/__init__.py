from SCons.Script import AddOption

# Adds an autoconf style enable/disable boolean option
def AddBooleanOption(env, name, msg):
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

# -%- lang: python -%-
