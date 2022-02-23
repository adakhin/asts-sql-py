#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "../src/asts_connection.h"
#include "../src/storage/sqlite.h"

namespace pt=boost::property_tree;

int main() {
  std::string connect_str, errormsg;
  pt::ptree tree;
  pt::read_ini("config.ini", tree);
  for(auto v : tree.get_child("TSMR"))
    connect_str.append(v.first+"="+v.second.data()+"\r\n");
  ad::asts::AstsConnection<ad::asts::SQLiteStorage> asts;
  if(!asts.Connect("TE", connect_str, errormsg)) {
    std::cout<< errormsg << std::endl;
    return 1;
  }
  asts.OpenTable("TE$TESYSTIME");
  ad::asts::SqlResult result;
  const char* query = R"sql(
    select
      NULL as test_field_1,
      1 as test_field_2,
      tt.*
    from te$tesystime tt
    union
    select
      'TT' as test_field_1,
      3 as test_field_2,
      tt.*
    from te$tesystime tt
    order by test_field_2 asc
  )sql";
  asts.Query(query, result);
  size_t fldcount = result.fields.size();
  for(auto & row : result.data) {
    for(size_t i=0; i<fldcount; ++i) {
      std::cout << result.fields[i].name << "[" << ad::asts::FieldTypeToStr(result.fields[i].type)<<"]  =  ";
      if(!row[i].has_value())
        std::cout << "[] NULL";
      else
        switch(result.fields[i].type) {
          case ad::asts::AstsFieldType::kInteger:
            std::cout << "[int64_t] ";
            std::cout << std::any_cast<int64_t>(row[i]);
            break;
          case ad::asts::AstsFieldType::kChar:
          case ad::asts::AstsFieldType::kFloat:
          case ad::asts::AstsFieldType::kDate:
          case ad::asts::AstsFieldType::kTime:
            std::cout << "[std::string] ";
            std::cout << std::any_cast<std::string>(row[i]);
            break;
          case ad::asts::AstsFieldType::kFixed:
          case ad::asts::AstsFieldType::kFloatPoint:
            std::cout << "[double] ";
            std::cout << std::any_cast<double>(row[i]);
            break;
          default:
            break;
        }
      std::cout << std::endl;
    }
  std::cout << "----------------------------------" <<std::endl;
  }
  std::cout << "that's all, folks!" << std::endl;
  return 0;
}
