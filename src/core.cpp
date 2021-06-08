#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sempr/Core.hpp>
#include <sempr/plugins/RDFPlugin.hpp>
#include <sempr/SeparateFileStorage.hpp>

namespace py = pybind11;
using namespace sempr;

PYBIND11_MODULE(semprpy, m)
{
    py::options options;
    //options.disable_function_signatures();
    options.enable_user_defined_docstrings();
    options.show_user_defined_docstrings();

    m.doc() = "Basic python bindings for sempr";

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
        .def("removeEntity", &Core::removeEntity);

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
        .def("loadFromJSON",
            [](Entity& e, const std::string& json)
            {
                std::stringstream ss(json);
                cereal::JSONInputArchive ar(ss);
                e.load(ar);
            })
        .def("saveToJSON",
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
        .def("loadFromJSON", 
            [](Component& c, const std::string& json)
            {
                std::stringstream ss(json);
                cereal::JSONInputArchive ar(ss);
                c.loadFromJSON(ar);
            })
        .def("saveToJSON",
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