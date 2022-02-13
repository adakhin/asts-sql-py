#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "../src/asts_connection.h"

namespace pt=boost::property_tree;

int main() {
  std::string connect_str, errormsg;
  pt::ptree tree;
  pt::read_ini("config.ini", tree);
  for(auto v : tree.get_child("TSMR"))
    connect_str.append(v.first+"="+v.second.data()+"\r\n");
  ad::asts::AstsConnection asts;
  if(!asts.Connect("TE", connect_str, errormsg)) {
    std::cout<< errormsg << std::endl;
    return 1;
  }
  return 0;
}
