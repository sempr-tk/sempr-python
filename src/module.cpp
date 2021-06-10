#include <pybind11/pybind11.h>

namespace py = pybind11;


void initCore(py::module_& m);
void initComponents(py::module_& m);
void initRete(py::module_& m);

PYBIND11_MODULE(semprpy, m)
{
    auto rete = m.def_submodule("rete");
    initRete(rete);

    initCore(m);
    initComponents(m);
}