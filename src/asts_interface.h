#ifndef ASTS_INTERFACE_H
#define ASTS_INTERFACE_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#ifndef MTE_SQL_MAX_FIELDS
#define MTE_SQL_MAX_FIELDS 255
#endif

namespace ad::asts {

//-----------------------------------------------------------------------------------
typedef int MTEHandle;
typedef char fld_attr_t; // TFieldFlags {ffKey = 0x01, ffSecCode = 0x02, ffNotNull = 0x04, ffVarBlock = 0x08}
typedef unsigned fld_size_t;

// we can't use native MTESRL field types because we need to have NULL as a separate type
enum AstsFieldType { kChar, kInteger, kFixed, kFloat, kDate, kTime, kFloatPoint, kNull };
inline std::string FieldTypeToStr(AstsFieldType t)
{
  switch (t)
  {
  case AstsFieldType::kChar:       return "ftChar";
  case AstsFieldType::kInteger:    return "ftInteger";
  case AstsFieldType::kFixed:      return "ftFixed";
  case AstsFieldType::kFloat:      return "ftFloat";
  case AstsFieldType::kDate:       return "ftDate";
  case AstsFieldType::kTime:       return "ftTime";
  case AstsFieldType::kFloatPoint: return "ftFloatPoint";
  case AstsFieldType::kNull:       return "NULL";
  default:                         break;
  }
  return "Unknown";
}
//-----------------------------------------------------------------------------------

struct AstsGenericField {
  std::string name;
  AstsFieldType type;
  fld_size_t size;
  fld_attr_t attr; 
  int decimals;

  int * ReadFromBuf(int * pointer);
};

struct AstsOutField : AstsGenericField {};
struct AstsInField : AstsGenericField {
  std::string defaultvalue;
  int * ReadFromBuf(int * pointer);
};

struct AstsTable {
  std::string name;
  fld_attr_t attr; // TTableFlags {mmfUpdateable = 1, mmfClearOnUpdate = 2, mmfOrderbook = 4}
  std::vector<AstsInField> infields;
  std::vector<AstsOutField> outfields;
  size_t max_fld_len = 0;
  size_t outfield_count = 0;
  std::vector<std::pair<size_t, std::string> > keyfields;
  int systemidx;

  int * ReadFromBuf(int * pointer);
};

struct AstsInterface {
    std::string prefix_="RE$";
    std::string name_="";
    std::string caption_="";
    std::string description_="";
    std::unordered_map<std::string, std::shared_ptr<AstsTable> > tables;

    void ReadFromBuf(int * pointer);
    bool LoadInterface(int handle, std::string & errmsg, bool debug = false);
    void Dump(void);

    std::string GetSystemType();
};

struct AstsOpenedTable {
  // MTESRL API parameters
  MTEHandle Table = 0;
  int ref = 0;

  std::shared_ptr<AstsInterface> iface_;
  std::shared_ptr<AstsTable> thistable_ = nullptr;
  std::string tablename_;
  std::map<std::string, std::string> inparams;

  AstsOpenedTable(std::shared_ptr<AstsInterface> iface, std::string table) noexcept {
    thistable_ = iface->tables[table];
    iface_ = iface;
    tablename_ = table;
  }
    //std::string ParamsToStr(void);
};


std::ostream & operator<< (std::ostream & os, const AstsGenericField & fld);
std::ostream & operator<< (std::ostream & os, const AstsInField & fld);
std::ostream & operator<< (std::ostream & os, const AstsTable & tbl);

}
#endif // ASTS_INTERFACE_H
