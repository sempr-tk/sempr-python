# sempr-python

This project provides a small python wrapper for sempr. Be aware that is not
feature-complete, and its usage is in many aspects different from the C++
version. What did you expect? This is python! :wink:

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

**But wait, there is more:** Out of the box, many python IDEs do not support code
completion and the like for binary modules. I'm using vscode with pylance and
struggled a bit to get it all working, so here is what worked for me in the end:

After having installed semprpy, you will need to create stubs for semprpy
anywhere on your system (does not need to happen inside the semprpy repository).
There are several different methods out there to do this, and what worked in the
end for me was using either `mypy` or `pybind11-stubgen`.

> _**Note**: I struggled a lot to get it all working, but as usual, it is quite
  simple if you know how. I first got it working with mypy, but noticed that
  it did not export any documentation, only the signatures. Hence, I gave
  pybind11-stubgen another try, and that works just fine now! (The only remaining
  problem being that I did not include much documentation in the python
  bindings, yet... but that is something else.)_

### With `pybind11-stubgen`

The preferred way right now is to use `pybind11-stubgen`, as it was specifically
made to generate stubs from C-extension-modules created with pybind11. It
properly includes documentation and signatures in the stubs.

> _**Note**: pybind11-stubgen is being moved into mypy, but at the time of
  writing this, it still outperforms mypy._

```
pip install pybind11-stubgen
pybind11-stubgen semprpy
```

This creates a folder `./stubs` which you can add to your IDE as a location for
stub files. In vscode with pylance, this looks like this for me:
![stubpath](img/pylance_stub_path.png)

### With `mypy`

As already mentioned, pybind11-stubgen should give better results. But if you
have any trouble with that, you can also use mypy:

```
pip install mypy
```

This includes a script called `stubgen` which we can use to create stubs for
any module, package or source file. In this case, we use it to create stubs for
the whole semprpy-_package_:

```
stubgen -p semprpy
```
This creates a folder `out/semprpy` with your stub files. You can use these in
vscode with pylance by adding the path to the `out` folder to the
`Python > Analysis: Extra Paths`. For me, this looks like this:
![extrapaths](img/pylance_extra_settings.png)


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

Also, take a look at `test/test.py`, where I test stuff during development.
