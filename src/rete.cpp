#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

#include <rete-core/WME.hpp>
#include <rete-core/Token.hpp>
#include <rete-rdf/Triple.hpp>

#include <rete-reasoner/Reasoner.hpp>
#include <rete-reasoner/RuleParser.hpp>
#include <rete-reasoner/ExplanationToDotVisitor.hpp>


namespace py = pybind11;
using namespace rete;


// helper stuff to properly expose ExplanationVisitors
// (and allow subclassing them _in python_!)
class PyExplanationVisitor : public ExplanationVisitor {
public:
    using ExplanationVisitor::ExplanationVisitor;

    void visit(WMESupportedBy& wmeSupport, size_t depth) override { PYBIND11_OVERRIDE_PURE(void, ExplanationVisitor, visit, wmeSupport, depth); }

    /** Process WMEs */
    void visit(WME::Ptr wme, size_t depth) override { PYBIND11_OVERRIDE_PURE(void, ExplanationVisitor, visit, wme, depth); }

    /** Process Evidences */
    void visit(Evidence::Ptr ev, size_t depth) override { PYBIND11_OVERRIDE_PURE(void, ExplanationVisitor, visit, ev, depth); }
    void visit(AssertedEvidence::Ptr ev, size_t depth) override { PYBIND11_OVERRIDE_PURE(void, ExplanationVisitor, visit, ev, depth); }
    void visit(InferredEvidence::Ptr ev, size_t depth) override { PYBIND11_OVERRIDE_PURE(void, ExplanationVisitor, visit, ev, depth); }
};



void initRete(py::module_& m)
{
    // WME
    py::class_<WME, std::shared_ptr<WME>>(m, "WME")
        .def_property_readonly("isComputed", &WME::isComputed)
        .def("__str__", &WME::toString)
        .def_property_readonly("type", &WME::type)
        .def("__lt__", [](const WME& self, const WME& other) { return self < other; })
        .def("__eq__", [](const WME& self, const WME& other) { return self == other; })
    ;

    // rete Triple (!= sempr Triple)
    py::enum_<Triple::Field>(m, "Field")
        .value("SUBJECT", Triple::Field::SUBJECT)
        .value("PREDICATE", Triple::Field::PREDICATE)
        .value("OBJECT", Triple::Field::OBJECT)
        .export_values()
    ;

    py::class_<Triple, std::shared_ptr<Triple>, WME>(m, "Triple")
        .def(py::init<const std::string&, const std::string&, const std::string&>())
        .def_readonly("subject", &Triple::subject)
        .def_readonly("predicate", &Triple::predicate)
        .def_readonly("object", &Triple::object)
        .def("getField", &Triple::getField)
        .def_static("fieldName", &Triple::fieldName)
        .def("__getitem__",
            [](const Triple& t, size_t index)
            {
                if (index == 0) return t.subject;
                if (index == 1) return t.predicate;
                if (index == 2) return t.object;
                throw py::index_error();
            }
        )
    ;

    // Token
    py::class_<Token, std::shared_ptr<Token>>(m, "Token")
        .def(py::init<>())
        .def_readwrite("parent", &Token::parent)
        .def_readwrite("wme", &Token::wme)
        .def("__str__", &Token::toString)
    ;

    // Production
    py::class_<Production, std::shared_ptr<Production>>(m, "Production")
        .def_property_readonly("priority", &Production::getPriority)
        .def_property("name", &Production::getName, &Production::setName)
        .def("__str__", &Production::toString);

    // Evidence
    py::class_<Evidence, std::shared_ptr<Evidence>>(m, "Evidence")
        .def("__eq__", [](const Evidence& self, const Evidence& other){ return self == other; })
        .def("__lt__", [](const Evidence& self, const Evidence& other){ return self < other; })
        .def("__str__", &Evidence::toString);
    py::class_<AssertedEvidence, std::shared_ptr<AssertedEvidence>, Evidence>(m, "AssertedEvidence")
        .def(py::init<const std::string&>());
    py::class_<InferredEvidence, std::shared_ptr<InferredEvidence>, Evidence>(m, "InferredEvidence")
        .def_property_readonly("token", &InferredEvidence::token)
        .def_property_readonly("production", &InferredEvidence::production);

    // Network
    // no, I don't want to expose all the node types etc... let the rule parser
    // handle everything! This is just for the dot representation...
    py::class_<Network, std::shared_ptr<Network>>(m, "Network")
        .def_property_readonly("dot", &Network::toDot);

    // avoided this in the inference state, but not sure how to avoid it at
    // the visitor level...
    py::class_<WMESupportedBy>(m, "WMESupportedBy")
        .def_readonly("evidences", &WMESupportedBy::evidences_)
        .def_readonly("wme", &WMESupportedBy::wme_)
    ;


    // ExplanationVisitor
    py::class_<ExplanationVisitor, PyExplanationVisitor>(m, "ExplanationVisitor")
        .def(py::init<>())
        .def("visit", py::overload_cast<WMESupportedBy&, size_t>(&ExplanationVisitor::visit))
        .def("visit", py::overload_cast<WME::Ptr, size_t>(&ExplanationVisitor::visit))
        .def("visit", py::overload_cast<AssertedEvidence::Ptr, size_t>(&ExplanationVisitor::visit))
        .def("visit", py::overload_cast<InferredEvidence::Ptr, size_t>(&ExplanationVisitor::visit))
        .def("visit", py::overload_cast<Evidence::Ptr, size_t>(&ExplanationVisitor::visit))
    ;

    // TODO: register overloads, and export VizSettings
    py::class_<ExplanationToDotVisitor, ExplanationVisitor>(m, "ExplanationToDotVisitor")
        .def(py::init<>())
        .def_property("maxDepth",
            &ExplanationToDotVisitor::getMaxDepth,
            &ExplanationToDotVisitor::setMaxDepth
        )
        .def("dot",
            [](const ExplanationToDotVisitor& self)
            {
                return self.str();
            }
        )
    ;

    // InferenceState
    py::class_<InferenceState>(m, "InferenceState")
        .def("explain",
            [](const InferenceState& state, WME::Ptr wme)
            {
                auto expl = state.explain(wme);
                return expl.evidences_;
            }
        )
        .def("explainedBy",
            [](const InferenceState& state, Evidence::Ptr evidence)
            {
                auto expl = state.explainedBy(evidence);
                return expl.wmes_;
            }
        )
        .def_property_readonly("numWMEs", &InferenceState::numWMEs)
        .def_property_readonly("numEvidences", &InferenceState::numEvidences)
        .def("getWMEs", &InferenceState::getWMEs)
        .def("traverseExplanation", &InferenceState::traverseExplanation)
    ;

    // Reasoner
    py::class_<Reasoner>(m, "Reasoner")
        .def(py::init<>())
        .def(py::init<size_t>())
        .def("performInference", &Reasoner::performInference)
        .def("addEvidence", &Reasoner::addEvidence)
        .def("removeEvidence", py::overload_cast<WME::Ptr, Evidence::Ptr>(&Reasoner::removeEvidence))
        .def("removeEvidence", py::overload_cast<Evidence::Ptr>(&Reasoner::removeEvidence))
        .def_property_readonly("network", py::overload_cast<>(&Reasoner::net))
        .def_property_readonly("inferenceState", &Reasoner::getCurrentState)
        .def("explainAsDOT",
            [](Reasoner& self, WME::Ptr toExplain)
            {
                ExplanationToDotVisitor visitor;
                self.getCurrentState().traverseExplanation(toExplain, visitor);
                return visitor.str();
            }
        )
    ;
}