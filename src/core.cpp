#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sempr/Core.hpp>
#include <sempr/plugins/RDFPlugin.hpp>
#include <sempr/SeparateFileStorage.hpp>
#include <sempr/RDF.hpp>

namespace py = pybind11;
using namespace sempr;

void initCore(py::module_& m)
{
    py::options options;
    //options.disable_function_signatures();
    options.enable_user_defined_docstrings();
    options.show_user_defined_docstrings();

    m.doc() = "Basic python bindings for sempr";

    // URI definitions/constants
    m.def("baseURI", &sempr::baseURI);

    m.def_submodule("rdf")
        .def("baseURI", &rdf::baseURI)
        .def("type", &rdf::type);

    m.def_submodule("rdfs")
        .def("baseURI", &rdfs::baseURI)
        .def("Resource", &rdfs::Resource)
        .def("Class", &rdfs::Class)
        .def("subClassOf", &rdfs::subClassOf)
        .def("subPropertyOf", &rdfs::subPropertyOf)
        .def("domain", &rdfs::domain)
        .def("range", &rdfs::range);

    m.def_submodule("owl")
        .def("baseURI", &owl::baseURI)
        .def("Class", &owl::Class)
        .def("FunctionalProperty", &owl::FunctionalProperty)
        .def("Nothing", &owl::Nothing)
        .def("ObjectProperty", &owl::ObjectProperty)
        .def("Restriction", &owl::Restriction)
        .def("Thing", &owl::Thing)
        .def("allValuesFrom", &owl::allValuesFrom)
        .def("cardinality", &owl::cardinality)
        .def("differentFrom", &owl::differentFrom)
        .def("disjointWith", &owl::disjointWith)
        .def("distinctMembers", &owl::distinctMembers)
        .def("equivalentClass", &owl::equivalentClass)
        .def("equivalentProperty", &owl::equivalentProperty)
        .def("hasValue", &owl::hasValue)
        .def("intersectionOf", &owl::intersectionOf)
        .def("inverseOf", &owl::inverseOf)
        .def("maxCardinality", &owl::maxCardinality)
        .def("minCardinality", &owl::minCardinality)
        .def("onClass", &owl::onClass)
        .def("onDataRange", &owl::onDataRange)
        .def("onDatatype", &owl::onDatatype)
        .def("oneOf", &owl::oneOf)
        .def("onProperty", &owl::onProperty)
        .def("sameAs", &owl::sameAs)
        .def("someValuesFrom", &owl::someValuesFrom)
        .def("unionOf", &owl::unionOf);

    // rete::ParsedRule
    py::class_<rete::ParsedRule, std::shared_ptr<rete::ParsedRule>>(m, "ParsedRule")
        .def("disconnect", &rete::ParsedRule::disconnect, "Disconnects all effects of the rule from the rete network")
        .def_property_readonly("effectNodes", &rete::ParsedRule::effectNodes, "List of effect nodes of this rule")
        .def_property_readonly("id", &rete::ParsedRule::id, "Numeric id of the rule in the core")
        .def_property_readonly("name", &rete::ParsedRule::name, "(Optional) name of the rule")
        .def_property_readonly("ruleString", &rete::ParsedRule::ruleString, "The string from which the rule was parsed")
        .doc() = "Testdoc!";


    // SPARQLQuery result types
    py::class_<SPARQLQuery> sq(m, "SPARQLQuery");
    py::enum_<SPARQLQuery::ValueType>(sq, "ValueType")
        .value("BLANK", SPARQLQuery::ValueType::BLANK)
        .value("LITERAL", SPARQLQuery::ValueType::LITERAL)
        .value("RESOURCE", SPARQLQuery::ValueType::RESOURCE)
        .export_values();

    // for component query results
    py::class_<ComponentQueryResult<Component>>(m, "ComponentQueryResult")
        .def_readonly("entity", &ComponentQueryResult<Component>::entity)
        .def_readonly("component", &ComponentQueryResult<Component>::component)
        .def_readonly("tag", &ComponentQueryResult<Component>::tag)
        .def_property_readonly("isInferred", &ComponentQueryResult<Component>::isInferred);


    // sempr::Core
    py::class_<Core>(m, "Core")
        .def(py::init<>())
        .def(py::init(
            [](const std::string& path)
            {
                if (!fs::exists(path)) fs::create_directory(path);
                auto db = std::make_shared<SeparateFileStorage>(path);
                auto core = std::make_unique<Core>(db, db);
                auto saved = db->loadAll();
                for (auto e : saved)
                {
                    core->addEntity(e);
                }

                return core;
            }), "Initializes a sempr::Core with a SeparateFileStorage "
                "persistence module pointing to the given path."
        )
        .def("loadPlugins", py::overload_cast<>(&Core::loadPlugins))
        .def("query",
            [](Core& core, const std::string& query)
            {
                auto rdf = core.getPlugin<RDFPlugin>();
                if (!rdf) throw std::runtime_error("RDFPlugin not loaded");

                SPARQLQuery sq;
                sq.query = query;

                rdf->soprano().answer(sq);
                return sq.results;
            })
        .def("componentQuery",
            [](Core& core, const std::string& query, const std::string& var)
            {
                auto rdf = core.getPlugin<RDFPlugin>();
                if (!rdf) throw std::runtime_error("RDFPlugin not loaded");

                return rdf->componentQuery(query)
                    .with<Component>(var).includeInferred(true).aggregate()
                    .execute();
            }
        )
        .def("componentQuery",
            [](Core& core, const std::string& query, const std::string& var0, const std::string& var1)
            {
                auto rdf = core.getPlugin<RDFPlugin>();
                if (!rdf) throw std::runtime_error("RDFPlugin not loaded");

                return rdf->componentQuery(query)
                    .with<Component>(var0).includeInferred(true).aggregate()
                    .with<Component>(var1).includeInferred(true).aggregate()
                    .execute();
            }
        )
        .def("addRules", &Core::addRules)
        .def("removeRule", &Core::removeRule)
        .def("rules", &Core::rules)
        .def("performInference", &Core::performInference)
        .def("addEntity", &Core::addEntity)
        .def("removeEntity", &Core::removeEntity)
        .def_property_readonly("reasoner", &Core::reasoner)
        ;

    // sempr::Entity
    py::class_<Entity, std::shared_ptr<Entity>>(m, "Entity")
        //.def_static("create", &Entity::create)
        .def(py::init(&Entity::create))
        .def_property_readonly("id", &Entity::id)
        .def_property_readonly("idIsURI", &Entity::idIsURI)
        .def("setId", &Entity::setId)
        .def("setUri", &Entity::setURI)
        .def_property_readonly("components", &Entity::getComponentsWithTag<Component>)
        .def("addComponent", static_cast<void(Entity::*)(Component::Ptr)>(&Entity::addComponent))
        .def("addComponent", static_cast<void(Entity::*)(Component::Ptr, const std::string&)>(&Entity::addComponent))
        .def("removeComponent", &Entity::removeComponent)
        .def("fromJSON",
            [](Entity& e, const std::string& json)
            {
                std::stringstream ss(json);
                cereal::JSONInputArchive ar(ss);
                e.load(ar);
            })
        .def("toJSON",
            [](const Entity& e) -> std::string
            {
                std::stringstream ss;
                {
                    cereal::JSONOutputArchive ar(ss);
                    e.save(ar);
                }
                return ss.str();
            });

    // sempr::Component
    py::class_<Component, std::shared_ptr<Component>>(m, "Component")
        .def(py::init<>())
        .def("changed", &Component::changed)
        .def("fromJSON",
            [](Component& c, const std::string& json)
            {
                std::stringstream ss(json);
                cereal::JSONInputArchive ar(ss);
                c.loadFromJSON(ar);
            })
        .def("toJSON",
            [](Component& c) -> std::string
            {
                std::stringstream ss;
                {
                    cereal::JSONOutputArchive ar(ss);
                    c.saveToJSON(ar);
                }
                return ss.str();
            });
}