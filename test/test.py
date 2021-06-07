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
json = c.saveToJSON()
print(f'{c} : {json}')

# add it to the entity
entity.addComponent(c, 'foo')
print(entity.saveToJSON())


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