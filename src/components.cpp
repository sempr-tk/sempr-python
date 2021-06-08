#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>

#include <sempr/component/AffineTransform.hpp>

namespace py = pybind11;
using namespace sempr;

void initComponents(py::module_& m)
{
    // AffineTransform
    py::class_<AffineTransform, std::shared_ptr<AffineTransform>, Component>(m, "AffineTransform")
        .def(py::init<>())
        .def(py::init(
            [](const Eigen::Matrix4d& mat)
            {
                Eigen::Affine3d affine(mat);
                return std::make_shared<AffineTransform>(affine);
            }
        ))
        .def_property("transform",
            [](const AffineTransform& a)
            {
                auto t = a.transform();
                return t.matrix();
            },
            [](AffineTransform& a, const Eigen::Matrix4d& mat)
            {
                Eigen::Affine3d affine(mat);
                a.setTransform(affine);
            }
        )
    ;
}