import pylure

# construct context
ctx = pylure.Context().addInt('CITY_ID', 123).addDouble('PI', 3.14).addString('USER', 'alice').addCustom('APP_VERSION', 'v3.2.1', 'semver')
print('Context: {}'.format(ctx.toDict()))

# compile and eval
s = 'CITY_ID == 123'
pat = pylure.compile(s)
print("({}) => {}".format(s, pylure.eval(pat, ctx)))