import semprpy as sempr
from typing import List

import pprint

my_printer = pprint.PrettyPrinter()
print = my_printer.pprint

# core, load plugins
core = sempr.Core('testdb')
core.loadPlugins()

# add a rule
rules = core.addRules("[true() -> (<a> <b> <c>)]")
print(rules)

# list rules
print('rules:')
rules : List[sempr.ParsedRule] = core.rules()
for rule in rules:
    print(f'{rule.id:2d} : {rule.ruleString}')

# remove one
core.removeRule(rules[0].id)

# list again
print('rules:')
rules : List[sempr.ParsedRule] = core.rules()
for rule in rules:
    print(f'{rule.id:2d} : {rule.ruleString}')


# create an entity
entity = sempr.Entity()
core.addEntity(entity)

print(f'{entity.id} - {entity.idIsURI}')

# create a component
c = sempr.Component()
json = c.toJSON()
print(f'{c} : {json}')

# add it to the entity
entity.addComponent(c, 'foo')
print(entity.toJSON())


# add rule for debugging existence of entities
core.addRules(
    '[printEC: EC<Component>(?e ?c ?t) -> print(?e "has comp with tag" ?t)]'
)
entity.addComponent(sempr.Component(), 'bar')
core.performInference()

# remove component
print('remove component')
entity.removeComponent(c)
core.performInference()


# add rule inferring triples
core.addRules(
    '[countComps: EC<Component>(?e ?c), sum(?one 0 1), GROUP BY (?e), count(?num ?one) -> (?e <ex:hasNumComps> ?num)]'
)
core.performInference()

e2 = sempr.Entity()
core.addEntity(e2)

for i in range(5):
    print(f'query {i+1}/5')
    e2.addComponent(sempr.Component(), f'tag_{i}')
    core.performInference()
    # sparql query!
    res = core.query('SELECT ?e ?num WHERE { ?e <ex:hasNumComps> ?num . }')
    print(res)


# remove the entity
print('remove entity')
core.removeEntity(entity)
core.performInference()

print(e2.components)
print([(c, t) for c, t in e2.components if isinstance(c, sempr.Component)])

res = core.componentQuery('SELECT ?e ?num WHERE { ?e <ex:hasNumComps> ?num . }', 'e')
print(res)

for r in res:
    sparql, clist = r
    #print(f'entity: {sparql["e"]}')
    print(sparql)
    for c in clist: # type: sempr.ComponentQueryResult
        print(f'-- {c.component} {c.isInferred} {c.tag}')


# AffineTransform
import numpy as np
a1 = sempr.AffineTransform() # default: identity
print(f'a1: {a1.transform}')

mat = np.identity(4)
mat[0:3, 3] = [3, 4, 5]
a1.transform = mat
print(f'a1: {a1.transform}')
print(a1.toJSON())

a2 = sempr.AffineTransform()
a2.fromJSON(a1.toJSON())
print(a2.transform)

# GeosGeometry
g1 = sempr.GeosGeometry()
print(g1.toJSON())

g2 = sempr.GeosGeometry("POINT (1 2 3)")
print(g2.toJSON())

print(g1.geometry)
g1.geometry = "POINT (4 5 6)"
print(g1.geometry)

try:
    g1.geometry = "PONT (7 8 9)" # intentional typo
except Exception as e:
    print(f'got an exception, as expected: {e}')

print(g1)


# Triples
t1 = sempr.Triple('<ex:foo>', '<ex:bar>', '<ex:bazzz>')
print(str(t1))

t1.object = '<ex:baz>'
print(str(t1))

# 3-tuples can be cast to a triple when needed
print(f"compare with py tuple:     {t1 == ('<ex:foo>', '<ex:bar>', '<ex:baz>')}")
print(f"compare with sempr triple: {t1 == sempr.Triple('<ex:foo>', '<ex:bar>', '<ex:baz>')}")


# TripleVector
tv = sempr.TripleVector()
for i in range(5):
    tv.add(('<ex:foo>', '<ex:bar>', f'<ex:baz_{i}>'))

def showVector(tv : sempr.TripleVector):
    print(len(tv))
    for t in tv:
        print(str(t))


showVector(tv)
print(f'1: {tv[1]}')

del tv[1]

showVector(tv)

tv.remove(('<ex:foo>', '<ex:bar>', '<ex:baz_3>'))
tv.add(('a', 'b', 'c'))

showVector(tv)



# TriplePropertyMap
m = sempr.TriplePropertyMap()
m['some_int'] = 42
m['some_float'] = 3.14159265
m['some_string'] = 'Hello, World!'
m['some_resource'] = 'ex:foo', True

for key in m:
    print(f'm[{key+"]":15s} ({str(m.typeAt(key))+")":20s} {m[key]}')

try:
    print(m["foo"])
except Exception as e:
    print(f'exception, as expected: {repr(e)}')

print('incomplete triples, as there is no entity assigned yet:')
for t in m.iter_triples():
    print(str(t))

# add it to an entity
entity.addComponent(m)

print('after adding it to an entity:')
for t in m.iter_triples():
    print(str(t))


# custom callbacks
def myFancyCallback(flag : sempr.rete.PropagationFlag, val1 : str):
    print(f"Woohoo! Python callback! {flag} -- {val1}")


core.registerCallbackEffect(myFancyCallback, "myCB")
core.addRules(
    '[testCB: EC<Component>(?e ?c), GROUP BY (?e) -> myCB(?e)]'
)
core.performInference()