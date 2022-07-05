#ifndef GENERIC_ENGINE_H
#define GENERIC_ENGINE_H
#include <string_view>

#include "asts_interface.h"
#include "util.h"

namespace ad::asts {

class GenericStorage {
public:
  // add reflection data to MTE$STRUCTURE table on Connect
  virtual void AddInterface(std::shared_ptr<AstsInterface> iface)=0;
  // remove reflection data on Disconnect
  virtual void RemoveInterface(std::shared_ptr<AstsInterface> iface)=0;
  // prepare to read row data from MTESRL-managed buffer (e.g. start SQL transaction)
  virtual void StartReadingRows(AstsOpenedTable* table) =0;
  // read one data row from MTESRL-managed buffer
  virtual void ReadRowFromBuffer(AstsOpenedTable* table, ad::util::PointerHelper& buffer, fld_count_t* fldnums, fld_count_t* fldnums_prev, fld_count_t fldcount) =0;
  // erase data from table (e.g. if clear on update flag is set)
  virtual void EraseData(const std::string& tablename, const std::string& secboard="", const std::string& seccode="")=0;
  // finish reading row data (e.g. commit SQL transaction)
  virtual void StopReadingRows()=0;

  virtual void CreateTable(std::shared_ptr<AstsInterface> iface, const std::string& tablename) =0;
  virtual void CloseTable(const std::string& tablename) =0;
  virtual void Query(std::string_view query, SqlResult& result, std::map<std::string, std::shared_ptr<AstsInterface> >& interfaces) =0;
};

} // ad::asts
#endif // GENERIC_ENGINE_H
