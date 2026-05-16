#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "hats/graph.hpp"
#include "hats/knowledge.hpp"
#include "solver/solver.hpp"

namespace py = pybind11;
using namespace hats;
using namespace solver;

PYBIND11_MODULE(hats_py, m) {
    m.doc() = "Hats DEL simulation Python bindings";

    py::class_<Graph>(m, "Graph")
        .def(py::init<std::size_t>(), py::arg("n"))
        .def("add_edge", &Graph::add_edge, py::arg("u"), py::arg("v"))
        .def("size", &Graph::size)
        .def("has_edge", &Graph::has_edge, py::arg("u"), py::arg("v"))
        .def("degree", &Graph::degree, py::arg("v"))
        .def("neighbors", &Graph::neighbors, py::arg("v"))
        .def_static("make_complete", &Graph::make_complete, py::arg("n"))
        .def_static("make_cycle", &Graph::make_cycle, py::arg("n"))
        .def_static("make_star", &Graph::make_star, py::arg("n"));

    py::class_<SimulationResult>(m, "SimulationResult")
        .def_readonly("success", &SimulationResult::success)
        .def_readonly("deadlock", &SimulationResult::deadlock)
        .def_readonly("rounds", &SimulationResult::rounds)
        .def("__repr__", [](const SimulationResult &r) {
            return "SimulationResult(success=" + std::string(r.success ? "True" : "False") +
                   ", deadlock=" + std::string(r.deadlock ? "True" : "False") +
                   ", rounds=" + std::to_string(r.rounds) + ")";
        });

    // --- optional: expose internals for step-by-step visualisation ---

    py::class_<KnowledgeState>(m, "KnowledgeState")
        .def_readonly("n", &KnowledgeState::n)
        .def_readonly("world_count", &KnowledgeState::world_count)
        .def_readonly("active_word_count", &KnowledgeState::active_word_count)
        .def_readonly("actual_views", &KnowledgeState::actual_views);

    py::class_<AgentPartition>(m, "AgentPartition")
        .def_readonly("neighbors_mask", &AgentPartition::neighbors_mask)
        .def_property_readonly("classes", [](const AgentPartition &ap) {
            py::dict d;
            for (const auto &[view, worlds] : ap.classes)
                d[py::int_(view)] = worlds;
            return d;
        });

    py::class_<RoundRecord>(m, "RoundRecord")
        .def_readonly("round",           &RoundRecord::round)
        .def_readonly("guessed_players", &RoundRecord::guessed_players)
        .def_readonly("guessed_colors",  &RoundRecord::guessed_colors)
        .def_readonly("valid_worlds",    &RoundRecord::valid_worlds)
        .def_readonly("silence",         &RoundRecord::silence)
        .def("__repr__", [](const RoundRecord &rec) {
            return "RoundRecord(round=" + std::to_string(rec.round) +
                   ", silence=" + (rec.silence ? "True" : "False") +
                   ", guessed=" + std::to_string(rec.guessed_players.size()) +
                   ", valid_worlds=" + std::to_string(rec.valid_worlds.size()) + ")";
        });

    // keep_alive<1,2>: Graph (arg 2) must outlive Solver (self = arg 1).
    py::class_<Solver>(m, "Solver")
        .def(py::init<const Graph &, WorldMask>(),
             py::arg("graph"), py::arg("actual_world"),
             py::keep_alive<1, 2>())
        .def("run",   &Solver::run)
        .def("trace", &Solver::trace);

    m.def("init_knowledge",     &init_knowledge,     py::arg("graph"), py::arg("actual_world"));
    m.def("compute_partitions", &compute_partitions, py::arg("graph"), py::arg("world_count"));
}
