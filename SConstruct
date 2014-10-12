from scons_util import AddBooleanOption

env = Environment()

vars = Variables('build.conf')

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
env.Append(CPPPATH = ['#.'])

# Set flags according to the debugging settings.
if env['debug']:
	env.Append(CCFLAGS = ['-g'])
else:
	env.Append(CPPDEFINES = ['NDEBUG'])

vars.Save('build.conf', env)

env.Export(['env'])

env.SConscript(dirs = ['lib'])

# -%- lang: python -%-
