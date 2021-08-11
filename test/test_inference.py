import semprpy as sempr
from semprpy import rdf, rdfs, rete
import json

core = sempr.Core()
core.loadPlugins()

def verbose(msg):
    print(msg)
    return msg

core.addRules(
    verbose(f"""
    @PREFIX sempr: <sempr:>
    @PREFIX rdfs: <{rdfs.baseURI()}>
    @PREFIX rdf: <{rdf.baseURI()}>

    [subclass:
        (?a rdfs:subClassOf ?b)
        [transitive:
            (?b rdfs:subClassOf ?c) -> (?a rdfs:subClassOf ?c)]
        [type:
            (?x rdf:type ?a) -> (?x rdf:type ?b)]
    ]

    [ontology:
        true()
        ->
        (sempr:C rdfs:subClassOf sempr:B),
        (sempr:B rdfs:subClassOf sempr:A)
    ]

    [print: (?a ?b ?c) -> print(?a ?b ?c)]
    """)
)

e1 = sempr.Entity()
c1 = sempr.TriplePropertyMap()
c1[rdf.baseURI() + 'type'] = 'sempr:C', True
# small bug/inconvenience: rdf.type() == '<' + rdf.baseURI() + 'type' + >'
# but the property map wraps it in '<' '>', too...
# TODO: change rdf.type() ... etc to only return rdf.baseURI() + 'type'
# but that is a breaking change on the C++ side, too :/

e1.addComponent(c1)
core.addEntity(e1)

for t in c1.iter_triples():
    print(t)


core.performInference()

q = f'SELECT * WHERE {{ sempr:{e1.id} rdf:type ?t. }}'
print(q)
result = core.query(q)
for r in result:
    l = [(k, *r[k]) for k in r]
    print(l)

# explain the first result!
#              <entity URI>      expanded rdf:type            <?t>
toExplain = sempr.Triple(f'<sempr:{e1.id}>',     rdf.type(),      f'<{result[0]["t"][1]}>')
print(toExplain)
# uses a shortcut, but internally also accesses the reasoners inference state
# with an ExplanationToDotVisitor, as shown below
print(core.explainAsDOT(toExplain))


# you can even subclass the ExplanationVisitor...
# be aware that this is usually not the case, and special preparations
# were necessary on the C++ side to enable this!
class MyVisitor(rete.ExplanationVisitor):
    def __init__(self):
        rete.ExplanationVisitor.__init__(self)
        print('init MyVisitor')
        self.log = []

    def visit(self, sth, depth):
        self.log.append(sth)

        if isinstance(sth, rete.WMESupportedBy):
            for e in sth.evidences:
                print(f'{sth.wme} supported by: ({e})')
        else:
            print(f'{depth}: {sth}')


state = core.reasoner.inferenceState
wmes = state.getWMEs()

v = MyVisitor()
state.traverseExplanation(wmes[2], v)

print(v.log)

v = rete.ExplanationToDotVisitor()
state.traverseExplanation(wmes[2], v)

#print(v.dot())
with open('test.dot', 'w+') as f:
    f.write(v.dot())


v = rete.ExplanationToJSONVisitor()
state.traverseExplanation(wmes[2], v)

with open('test.json', 'w+') as f:
    json.dump(v.json(), f, indent=4)


print(core.explainAsJSON(
    sempr.Triple(*wmes[2])
))