#include "asts_interface.h"

#include <fstream>

#include "mtesrl.h"
#include "mteerr.h"

namespace ad::asts {

void AstsGenericField::ReadFromBuf(ut::PointerHelper& pointer) {
  name = pointer.ReadString();
  pointer.RewindString(); // caption
  pointer.RewindString(); // description
  size = (fld_size_t)pointer.ReadInt();
  type = (AstsFieldType)pointer.ReadInt();
  decimals = pointer.ReadInt();
  attr = (fld_attr_t)pointer.ReadInt();
  pointer.RewindString(); // enumname
}

void AstsInField::ReadFromBuf(ut::PointerHelper& pointer) {
  AstsGenericField::ReadFromBuf(pointer);
  defaultvalue = pointer.ReadString();
}

//----------------------------------------------------------------------------

void AstsTable::ReadFromBuf(ut::PointerHelper& pointer) {
  name = pointer.ReadString();
  pointer.RewindString(); // caption
  pointer.RewindString(); // description
  systemidx = pointer.ReadInt();
  attr = (fld_attr_t)pointer.ReadInt();
  size_t infieldnum = (size_t)pointer.ReadInt();
  infields.reserve(infieldnum);
  for(size_t c=0; c<infieldnum; c++) {
    AstsInField tmp;
    tmp.ReadFromBuf(pointer);
    infields.push_back(tmp);
  }
  outfield_count = pointer.ReadInt();
  outfields.reserve(outfield_count);
  AstsOutField tmp;
  for(size_t c=0; c<outfield_count; c++) {
    tmp.ReadFromBuf(pointer);
    max_fld_len = max_fld_len < tmp.size ? tmp.size : max_fld_len;
    outfields.push_back(tmp);
    if((tmp.attr & mffKey) == mffKey) {
      keyfields.push_back({(int)c, tmp.name});
    }
  }
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
  if(debug)
    Dump();
  return true;
}

void AstsInterface::ReadFromBuf(int* raw_ptr) {
  ut::PointerHelper pointer(raw_ptr);
  name_ = pointer.ReadString();
  pointer.RewindString(); // caption
  pointer.RewindString(); // description
  // enums are useless at the moment, so not loaded
  size_t enumtypenum, enumcount;
  enumtypenum = (size_t)pointer.ReadInt();
  for (size_t c=0; c<enumtypenum; c++) {
    pointer.RewindString(); // name
    pointer.RewindString(); // caption
    pointer.RewindString(); // description
    pointer.RewindInt(); // size
    pointer.RewindInt(); // enum kind
    enumcount = (size_t)pointer.ReadInt();
    for (size_t i=0; i<enumcount; i++) {
      pointer.RewindString(); // value
      pointer.RewindString(); // long description
      pointer.RewindString(); // short description
    }
  }
  // load tables
  int tablenum = pointer.ReadInt();
  bool got_prefix = false;
  std::unordered_map<std::string, std::shared_ptr<AstsTable> > tables_temp;
  prefix_ = "RE$";
  for (int c=0; c<tablenum; c++) {
    std::shared_ptr<AstsTable> tmp = std::make_shared<AstsTable>();
    tmp->ReadFromBuf(pointer);
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

void AstsInterface::Dump(void) {
  std::ofstream fl;
  fl.open(name_+".txt", std::ios::out);
  fl << "INTERFACE " << name_ << std::endl;
  fl << "this interface has prefix " << prefix_ << std::endl << std::endl;
  fl << "Tables:" << std::endl << std::endl;
  for(auto & rec : tables)
    fl << *rec.second << std::endl;
  fl << std::endl;
  fl.close();
}

std::string AstsInterface::GetSystemType() {
  return prefix_.substr(0, 2);
}

//----------------------------------------------------------------------------

std::ostream & operator<< (std::ostream & os, const AstsGenericField & fld) {
  os << " ** FIELD " << fld.name << " Size=" << std::to_string(fld.size) << " Type=";
  os << FieldTypeToStr(fld.type) << " Decimals="+std::to_string(fld.decimals) << " Attr=" << std::to_string(fld.attr);
  return os;
}

std::ostream & operator<< (std::ostream & os, const AstsInField & fld) {
  os << (AstsGenericField)fld << " Default='"+fld.defaultvalue+"'" << std::endl;
  return os;
}

std::ostream& operator<< (std::ostream& os, const AstsTable& tbl) {
  os << "TABLE " << tbl.name << " Attr=" << std::to_string(tbl.attr) << std::endl;
  os << " * IN FIELDS:" <<std::endl;
  for (AstsInField i : tbl.infields)
    os << i << std::endl;
  os << " * OUT FIELDS:" <<std::endl;
  for (AstsOutField i : tbl.outfields)
    os << i << std::endl;
  return os;
}
}
