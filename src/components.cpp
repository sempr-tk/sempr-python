#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>

#include <sempr/component/AffineTransform.hpp>
#include <sempr/component/GeosGeometry.hpp>
#include <geos/geom/Point.h>

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
}