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

vars.Save('build.conf', env)

# -%- lang: python -%-
