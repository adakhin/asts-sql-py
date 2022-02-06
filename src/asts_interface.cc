#include "asts_interface.h"

#include "mtesrl.h"
#include "mteerr.h"

#include "util.h"

namespace ut = ad::util;
namespace ad::asts {

int* AstsGenericField::ReadFromBuf(int* pointer) {
  pointer = ut::ReadValueFromBuf(pointer, name);
  pointer = ut::SkipStringFromBuf(pointer); // caption
  pointer = ut::SkipStringFromBuf(pointer); // description
  int tmp;
  pointer = ut::ReadValueFromBuf(pointer, tmp);
  size = (fld_size_t)tmp;
  pointer = ut::ReadValueFromBuf(pointer, tmp);
  type = (AstsFieldType)tmp;
  pointer = ut::ReadValueFromBuf(pointer, decimals);
  pointer = ut::ReadValueFromBuf(pointer, tmp);
  attr = tmp;
  pointer = ut::SkipStringFromBuf(pointer); // enumname
  return pointer;
}

int* AstsInField::ReadFromBuf(int* pointer) {
  pointer = AstsGenericField::ReadFromBuf(pointer);
  pointer = ut::ReadValueFromBuf(pointer, defaultvalue);
  return pointer;
}

//----------------------------------------------------------------------------

int* AstsTable::ReadFromBuf(int* pointer) {
  pointer = ut::ReadValueFromBuf(pointer, name);
  pointer = ut::SkipStringFromBuf(pointer); // caption
  pointer = ut::SkipStringFromBuf(pointer); // description
  pointer = ut::ReadValueFromBuf(pointer, systemidx);
  int temp, infieldnum;
  pointer = ut::ReadValueFromBuf(pointer, temp);
  attr = temp;
  pointer = ut::ReadValueFromBuf(pointer, infieldnum);
  infields.reserve(infieldnum);
  for(int c=0; c<infieldnum; c++) {
    AstsInField tmp;
    pointer = tmp.ReadFromBuf(pointer);
    infields.push_back(tmp);
  }
  pointer = ut::ReadValueFromBuf(pointer, temp);
  outfield_count = temp;
  outfields.reserve(outfield_count);
  AstsOutField tmp;
  for(size_t c=0; c<outfield_count; c++) {
    pointer = tmp.ReadFromBuf(pointer);
    max_fld_len = max_fld_len < tmp.size ? tmp.size : max_fld_len;
    outfields.push_back(tmp);
    if((tmp.attr & mffKey) == mffKey) {
      keyfields.push_back({(int)c, tmp.name});
    }
  }
  return pointer;
}

//----------------------------------------------------------------------------

bool AstsInterface::LoadInterface(int handle, std::string & errmsg, bool debug) {
  MTEMSG *ifacedata = nullptr;
  int interface=0;
  interface = MTEStructureEx(handle, 3, &ifacedata);
  if (interface < 0) {
    errmsg = std::string("MTEStructureEx returned an error: ")+MTEErrorMsg(interface);
    return false;
  }
  int * pointer = (int *)ifacedata->Data;
  ReadFromBuf(pointer);
//if(debug)
//  Dump();
  return true;
}

void AstsInterface::ReadFromBuf(int * pointer) {
  pointer = ut::ReadValueFromBuf(pointer, name_);
  pointer = ut::SkipStringFromBuf(pointer); // caption
  pointer = ut::SkipStringFromBuf(pointer); // description
  // enums are useless at the moment, so not loaded
  int enumtypenum = 0, tmp;
  pointer = ut::ReadValueFromBuf(pointer, enumtypenum);
  for (int c=0; c<enumtypenum; c++) {
    pointer = ut::SkipStringFromBuf(pointer); // name
    pointer = ut::SkipStringFromBuf(pointer); // caption
    pointer = ut::SkipStringFromBuf(pointer); // description
    pointer = ut::SkipIntFromBuf(pointer); // size
    pointer = ut::SkipIntFromBuf(pointer); // enum kind
    pointer = ut::ReadValueFromBuf(pointer, tmp); // enum count
    for (int i=0; i<tmp; i++) {
        pointer = ut::SkipStringFromBuf(pointer); // value
        pointer = ut::SkipStringFromBuf(pointer); // long description
        pointer = ut::SkipStringFromBuf(pointer); // short description
    }
  }
  // load tables
  int tablenum = 0;
  pointer = ut::ReadValueFromBuf(pointer, tablenum);
  bool got_prefix = false;
  std::unordered_map<std::string, std::shared_ptr<AstsTable> > tables_temp;
  prefix_ = "RE$";
  for (int c=0; c<tablenum; c++) {
    std::shared_ptr<AstsTable> tmp = std::make_shared<AstsTable>();
    pointer = tmp->ReadFromBuf(pointer);
    tables_temp[tmp->name] = tmp;
    if(!got_prefix && tmp->name == "TESYSTIME") {
      prefix_ = "TE$";
      got_prefix = true;
    }
  }
  // add prefix to table name for each table
  for(auto & t_data: tables_temp)
    tables.insert({ prefix_+t_data.second->name, t_data.second});
}

}
