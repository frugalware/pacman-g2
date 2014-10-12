env = Environment()

vars = Variables('build.conf')

vars.AddVariables(
	('debug', 'compile with debugging', 0),
	('static', 'compile static objects', 0),
	('shared', 'compile shared objects', 0),
)

vars.Update(env)

vars.Save('build.conf', env)

# -%- lang: python -%-
