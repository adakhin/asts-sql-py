#include <boost/python.hpp>

#include "asts_connection.h"
#include "storage/sqlite.h"

namespace bpy = boost::python;

class AstsConnectionProxy: public ad::asts::AstsConnection<ad::asts::SQLiteStorage> {
public:
  bpy::list Query(const std::string& query) {
    bpy::list tmp;
    ad::asts::SqlResult result;
    AstsConnection::Query(query, result);
    size_t fldcount = result.fields.size();
    for(auto & row : result.data) {
      bpy::dict line;
      for(size_t i=0; i<fldcount; ++i) {
        line[result.fields[i].name] ;
        if(!row[i].has_value())
          line[result.fields[i].name] = bpy::object();
        else
          switch(result.fields[i].type) {
            case ad::asts::AstsFieldType::kInteger:
              line[result.fields[i].name] = bpy::long_(std::any_cast<int64_t>(row[i]));
              break;
            case ad::asts::AstsFieldType::kChar:
            case ad::asts::AstsFieldType::kFloat:
            case ad::asts::AstsFieldType::kDate:
            case ad::asts::AstsFieldType::kTime:
              line[result.fields[i].name] = bpy::str(std::any_cast<std::string>(row[i]));
              break;
            case ad::asts::AstsFieldType::kFixed:
            case ad::asts::AstsFieldType::kFloatPoint:
              line[result.fields[i].name] = bpy::long_(std::any_cast<double>(row[i]));
              break;
            default:
              break;
          }
      }
      tmp.append(line);
    }
    return tmp;
  }

  void OpenTable(const std::string tablename, bpy::dict in_dict = bpy::dict()) {
     std::map<std::string, std::string> inparams;
     bpy::list keys = in_dict.keys();
     for(bpy::ssize_t i=0; i<bpy::len(keys); ++i) {
       bpy::str k, v;
       k = bpy::str(keys[i]);
       v = bpy::str(in_dict[k]);
       inparams[std::string(bpy::extract<char const*>(k))] = std::string(bpy::extract<char const*>(v));
     }
     AstsConnection::OpenTable(tablename, inparams);
  }
};

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(AstsConnectionProxy_overloads, OpenTable, 1, 2)

BOOST_PYTHON_MODULE(astslib)
{
    bpy::class_<AstsConnectionProxy>("AstsConnectionProxy")
        .def("Connect", &AstsConnectionProxy::Connect)
        .def("Disconnect", &AstsConnectionProxy::Disconnect)
        .def("OpenTable", &AstsConnectionProxy::OpenTable, AstsConnectionProxy_overloads())
        .def("CloseTable", &AstsConnectionProxy::CloseTable)
        .def("RefreshTable", &AstsConnectionProxy::RefreshTable)
        .def("Query", &AstsConnectionProxy::Query)
        .def_readwrite("debug", &AstsConnectionProxy::debug);
    ;
}
