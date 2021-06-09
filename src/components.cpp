#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/operators.h>

#include <sempr/component/AffineTransform.hpp>
#include <sempr/component/GeosGeometry.hpp>
#include <geos/geom/Point.h>
#include <sempr/component/TripleVector.hpp>
#include <sempr/component/TriplePropertyMap.hpp>

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
    py::class_<TripleVector, std::shared_ptr<TripleVector>, Component>(m, "TripleVector")
        .def(py::init<>())
        .def("triples",
            [](const TripleVector& tv)
            {
                std::vector<Triple> l;
                tv.getTriples(l);
                return l;
            }
        )
        .def("__getitem__", &TripleVector::getTripleAt)
        .def("__delitem__", &TripleVector::removeTripleAt)
        .def("__len__", &TripleVector::size)
        .def("__iter__",
            [](const TripleVector& v)
            {
                return py::make_iterator(v.begin(), v.end());
            },
            py::keep_alive<0, 1>()
        )
        .def("add", &TripleVector::addTriple)
        .def("remove", &TripleVector::removeTriple)
        .def("clear", &TripleVector::clear)
    ;


    // type enum for TriplePropertyMap
    py::enum_<TriplePropertyMapEntry::Type>(m, "ValueType")
        .value("INVALID", TriplePropertyMapEntry::INVALID)
        .value("RESOURCE", TriplePropertyMapEntry::RESOURCE)
        .value("STRING", TriplePropertyMapEntry::STRING)
        .value("FLOAT", TriplePropertyMapEntry::FLOAT)
        .value("INT", TriplePropertyMapEntry::INT)
        .export_values()
    ;

    // TriplePropertyMap
    py::class_<TriplePropertyMap, std::shared_ptr<TriplePropertyMap>, Component>(m, "TriplePropertyMap")
        .def(py::init<>())
        .def("__getitem__",
            [](TriplePropertyMap& m, const std::string& key) -> py::object
            {
                auto& entry = m.map_.at(key);

                switch (entry.type()) {
                    case TriplePropertyMapEntry::RESOURCE:
                    case TriplePropertyMapEntry::STRING:
                        return py::str(entry);
                    case TriplePropertyMapEntry::FLOAT:
                        return py::float_(entry);
                    case TriplePropertyMapEntry::INT:
                        return py::int_(static_cast<int>(entry));
                    case TriplePropertyMapEntry::INVALID:
                        break;
                }

                throw std::invalid_argument("no entry with key '" + key + "'");
            }
        )
        .def("__setitem__",
            [](TriplePropertyMap& m, const std::string& key, int val)
            {
                m.map_[key] = val;
            }
        )
        .def("__setitem__",
            [](TriplePropertyMap& m, const std::string& key, float val)
            {
                m.map_[key] = val;
            }
        )
        .def("__setitem__",
            [](TriplePropertyMap& m, const std::string& key, const std::pair<std::string, bool>& val)
            {
                m.map_[key] = { val.first, val.second };
            }
        )
        .def("__setitem__",
            [](TriplePropertyMap& m, const std::string& key, const std::string& val)
            {
                m.map_[key] = val;
            }
        )
        .def("__iter__",
            [](TriplePropertyMap& m)
            {
                return py::make_key_iterator(m.map_.begin(), m.map_.end());
            },
            py::keep_alive<0, 1>()
        )
        .def("iter_triples",
            [](TriplePropertyMap& m)
            {
                return py::make_iterator(m.begin(), m.end());
            },
            py::keep_alive<0, 1>()
        )
        .def("typeAt",
            [](TriplePropertyMap& m, const std::string& key)
            {
                return m.map_[key].type();
            }
        )
    ;
}