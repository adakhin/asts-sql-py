#ifndef ASTS_CONNECTION_H
#define ASTS_CONNECTION_H

#include <string>
#include <map>
#include <memory>
#include <string.h> // memset
#include <stdexcept>

#include "mtesrl.h"
#include "mteerr.h"

#include "asts_interface.h"
#include "util.h"

namespace ad::asts {

template<typename storage_engine_t> class AstsConnection {
private:
  std::map<std::string, int> handles_ { {"TE", -1}, {"RE", -1}, {"RFS", -1}, {"ALGO", -1} };
  std::map<std::string, std::shared_ptr<AstsInterface> > interfaces_;
  std::map<std::string, AstsOpenedTable*> tables_;
  bool debug_ = true;
  storage_engine_t engine_;

  std::string GetSystemFromTableName(const std::string& tablename)
  {
      size_t idx = tablename.find('$');
      if (idx == std::string::npos)
          throw std::runtime_error("Invalid table name: "+tablename+". Unable to deduce system type");
      std::string system = tablename.substr(0, idx);
      if (handles_.find(system) == handles_.end())
          throw std::runtime_error("Invalid system type: "+system);
      return system;
  }

  void NewTableInternal(const std::string& system, const std::string& tablename) {
    if(interfaces_[system]->tables.find(tablename) == interfaces_[system]->tables.end())
        throw std::runtime_error("Table "+tablename+" does not exist in interface "+interfaces_[system]->name_);
    // only one copy of each table may be opened
    if (tables_.find(tablename) != tables_.end())
        throw std::runtime_error("Table "+tablename+" has been already opened");
    engine_.CreateTable(interfaces_[system], tablename);
    tables_[tablename] = new AstsOpenedTable(interfaces_[system], tablename);
  }

  void LoadTableInternal(const std::string& system, AstsOpenedTable* tbl) {
    MTEMSG *TableData;
    std::string params = tbl->ParamsToStr();
    tbl->Table = MTEOpenTable(handles_[system], (char *)tbl->thistable_->name.c_str(), (char *)params.c_str(), 1, &TableData);
    engine_.StartReadingRows(tbl);
    if(tbl->Table >= 0) {
       // load actual data
       int32_t* ptr = (int32_t*)(TableData->Data);
       ad::util::PointerHelper buffer(ptr);
       tbl->ref = buffer.ReadInt();
       int row_count = buffer.ReadInt();
       if(!row_count)
         return;
       int datalen = 0;
       fld_count_t fldcount = 0;
       fld_count_t fldnums[MTE_SQL_MAX_FIELDS] = {0};
       fld_count_t fldnums_prev[MTE_SQL_MAX_FIELDS] = {0};
       bool is_orderbook = (tbl->thistable_->attr & mmfOrderBook);
       for(int i=0; i<row_count; i++) {
         // FieldCount Byte
         fldcount = buffer.ReadChar();
         // DataLength Integer
         datalen = buffer.ReadInt();
         // determine list of fields in table
         // WARNING: field order in interface and real data may differ!
         memset(fldnums, 0x00, sizeof(fldnums));
         if(fldcount != 0) {
           // explicit list of fields - copy it to our own buffer from MTESRL-managed one
           memcpy(fldnums, (fld_count_t*)buffer._ptr, fldcount);
           buffer.Rewind(fldcount);
         }
         else {
           // all fields - fill our buffer with consecutive numbers up to outfield_count for this table
           for (size_t f=0; f<tbl->thistable_->outfield_count; f++)
             fldnums[f] = f;
         }
         engine_.ReadRowFromBuffer(tbl, buffer, fldnums, fldnums_prev, fldcount ? fldcount : tbl->thistable_->outfield_count);
         memcpy(fldnums_prev, fldnums, sizeof(fldnums_prev));
      }
      engine_.StopReadingRows();
    }
    else
      throw std::runtime_error("Unable to load table "+tbl->tablename_+": "+std::string(TableData->Data, TableData->DataLen));
  }
public:
  void Connect(const std::string & system, const std::string & params){
    if(handles_.find(system) == handles_.end())
      throw std::runtime_error("Invalid system "+system);
    if(handles_[system] >= 0)
      throw std::runtime_error("System "+system+" is already connected!");

    char ErrMsg[256];
    handles_[system] = MTEConnect((char*)params.c_str(), ErrMsg);

    if(handles_[system] < 0)
      throw std::runtime_error("MTEConnect returned an error: "+std::to_string(handles_[system])+" "+std::string(ErrMsg));

    interfaces_[system] = std::make_shared<AstsInterface>();
    std::string errmsg;
    if(!interfaces_[system]->LoadInterface(handles_[system], errmsg, debug_))
      throw std::runtime_error("Unable to load interface! "+errmsg);
    engine_.AddInterface(interfaces_[system]);
  }

  void Disconnect(const std::string & system){
    if(handles_.find(system) == handles_.end())
      return;
    if(handles_[system] >= 0) {
      MTEDisconnect(handles_[system]);
      handles_[system] = -1;
    }
    interfaces_.erase(system);
  }

  void OpenTable(const std::string tablename, std::map<std::string, std::string> inparams={}) {
    std::string system = GetSystemFromTableName(tablename);
    NewTableInternal(system, tablename);
    auto tbl = tables_[tablename];
    if(!inparams.empty())
        tbl->inparams = inparams;
    LoadTableInternal(system, tables_[tablename]);
  }

  void CloseTable(const std::string tablename) {
    std::string system = GetSystemFromTableName(tablename);
    if (tables_.find(tablename) == tables_.end())
      throw std::runtime_error("Table "+tablename+" has not been opened");
    if(handles_[system] >= 0)
      MTECloseTable(handles_[system], tables_[tablename]->ref);
    delete tables_[tablename];
    tables_.erase(tablename);
  }

  void Query(const std::string& query, SqlResult& result) {
    if(query.empty())
      return;
    engine_.Query(query, result, interfaces_);
  }

};

}
#endif // ASTS_CONNECTION_H
