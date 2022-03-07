#include <boost/python.hpp>

#include "asts_connection.h"
#include "storage/sqlite.h"

using namespace boost::python;
using asts_connection_t = ad::asts::AstsConnection<ad::asts::SQLiteStorage>;

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(asts_connection_t_overloads, OpenTable, 1, 2)

BOOST_PYTHON_MODULE(astslib)
{
    class_<asts_connection_t>("AstsConnectionProxy")
        .def("Connect", &asts_connection_t::Connect)
        .def("Disconnect", &asts_connection_t::Disconnect)
        .def("OpenTable", &asts_connection_t::OpenTable, asts_connection_t_overloads())
        .def("CloseTable", &asts_connection_t::CloseTable)
        .def("Query", &asts_connection_t::Query)
    ;
}
