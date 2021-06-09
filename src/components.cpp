#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/operators.h>

#include <sempr/component/AffineTransform.hpp>
#include <sempr/component/GeosGeometry.hpp>
#include <geos/geom/Point.h>
#include <sempr/component/TripleVector.hpp>

#include <stdexcept>

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
        );

    // GeosGeometry
    py::class_<GeosGeometry, std::shared_ptr<GeosGeometry>, Component>(m, "GeosGeometry")
        .def(py::init(
            []() // empty point by default
            {
                auto factory = geos::geom::GeometryFactory::getDefaultInstance();
                return std::make_shared<GeosGeometry>(factory->createPoint());
            }
        ))
        .def(py::init(
            [](const std::string& wkt)
            {
                auto& factory = *geos::geom::GeometryFactory::getDefaultInstance();
                geos::io::WKTReader reader(factory);

                // parse the string
                std::unique_ptr<geos::geom::Geometry> g(reader.read(wkt));
                return std::make_shared<GeosGeometry>(std::move(g));
            }
        ))
        .def_property("geometry",
            [](const GeosGeometry& geo)
            {
                geos::io::WKTWriter writer;
                int dim = geo.geometry()->getCoordinateDimension();
                writer.setOutputDimension(dim);

                // create the wkt string
                return writer.writeFormatted(geo.geometry());
            },
            [](GeosGeometry& geo, const std::string& wkt)
            {
                auto& factory = *geos::geom::GeometryFactory::getDefaultInstance();
                geos::io::WKTReader reader(factory);

                // parse the string
                std::unique_ptr<geos::geom::Geometry> g(reader.read(wkt));
                geo.setGeometry(std::move(g));
            }
        )
    ;


    // Triple
    py::class_<Triple>(m, "Triple")
        .def(py::init(
            [](py::tuple t)
            {
                if (t.size() != 3)
                    throw std::invalid_argument("need subject, predicate, object");

                return std::make_unique<Triple>(py::str(t[0]), py::str(t[1]), py::str(t[2]));
            }
        ))
        .def(py::init<const std::string&, const std::string&, const std::string&>())
        .def("__str__", &Triple::toString)
        .def(py::self == py::self)
        .def_property("subject",
            [](const Triple& t){ return t.getField(Triple::Field::SUBJECT);},
            [](Triple& t, const std::string& str){ t.setField(Triple::Field::SUBJECT, str);}
        )
        .def_property("predicate",
            [](const Triple& t){ return t.getField(Triple::Field::PREDICATE);},
            [](Triple& t, const std::string& str){ t.setField(Triple::Field::PREDICATE, str);}
        )
        .def_property("object",
            [](const Triple& t){ return t.getField(Triple::Field::OBJECT);},
            [](Triple& t, const std::string& str){ t.setField(Triple::Field::OBJECT, str);}
        )
    ;

    py::implicitly_convertible<py::tuple, Triple>();

    // TripleVector
    //py::class_<TripleVector, std::shared_ptr<TripleVector>, Component>(m, "TripleVector")
}