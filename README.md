# sempr-python

This project provides a small python wrapper for sempr. Be aware that is not
feature-complete, and the usage might differ from the c++ version of sempr in
some (though minor) cases.

## Installation

This project uses `pybind11` to create the python extension module:
```
pip install pybind11
```
Although a `CMakeLists.txt` is present (and you are free to use it,
if you want -- personally, I'm using it during development with my `PYTHONPATH`
pointed to the build directory), the compilation and installation is handled by
the `setup.py`:
```
python setup.py build
python setup.py install
```
If it fails to find pybind11 in the process, you'll need to specify its
location. In my case, like this:
```
pybind11_DIR=~/pythonenv/lib/python3.6/site-packages/pybind11/share/cmake/pybind11 python setup.py build
python setup.py install
```

Now you are good to go!

## Usage

The created python extension is called `semprpy`.

```python
import semprpy as sempr

core = sempr.Core('my_database_folder') # or without parameter to skip persistence
core.loadPlugins()

core.addRules('[true() -> (<ex:foo> <ex:bar> <ex:baz>)]')
core.addRules('[EC<Component>(?e ?c) -> (?e <ex:bar> <ex:blub>)]')
for r in core.rules():
    print(f'{r.id:2d} : {r.name if r.name else "<unnamed rule>"}')

entity = sempr.Entity()
core.addEntity(entity)

component = sempr.Component() # base class without data, only for testing
entity.addComponent(component)

# as always, inference before querying
core.performInference()

# sparql queries are available, too - and simplified.
# No need to get the RDFPlugin first.
# But: This will throw if the plugin is not loaded!
results = core.query('SELECT * WHERE { ?a <ex:bar> ?b . }')
for r in results:
    # r is a dict, with variables used in the query as keys, and a tuple of
    # (ValueType, value) as value.
    print(f'a: {r["a"][1]:20s} b: {r["b"][1]:10s}')
```

Outputs something along the lines:

```
 1 : extractTriples
 2 : updateSoprano
 4 : inferRules
 5 : <unnamed rule>
 6 : <unnamed rule>   
a: sempr:Entity_0       b: ex:blub   
a: ex:foo               b: ex:baz    
```