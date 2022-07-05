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

using inparams_t = std::map<std::string, std::string>;

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

  void LoadTableData(const std::string& system, AstsOpenedTable* tbl, int32_t* ptr) {
    if(tbl->Table < 0)
      return;
    engine_.StartReadingRows(tbl);
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
      if(tbl->thistable_->attr & mmfClearOnUpdate) {
          // mmfClearOnUpdate: row with datalen==0 means table is now empty
          if(!datalen) {
            engine_.EraseData(tbl->tablename_);
            continue;
          }
          // mmfClearOnUpdate: we must erase old contents first and then replace it with new data
          // but not for orderbooks, orderbooks are tricky
          if(!is_orderbook && (i == 0))
            engine_.EraseData(tbl->tablename_);
      }

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

  void LoadTableInternal(const std::string& system, AstsOpenedTable* tbl) { // TODO: delete this fn
    MTEMSG *TableData;
    std::string params = tbl->ParamsToStr();
    tbl->Table = MTEOpenTable(handles_[system], (char *)tbl->thistable_->name.c_str(), (char *)params.c_str(), 1, &TableData);
    if(tbl->Table < 0)
      throw std::runtime_error("Unable to load table "+tbl->tablename_+": "+std::string(TableData->Data, TableData->DataLen));
    // load actual data
    int32_t* ptr = (int32_t*)(TableData->Data);
    LoadTableData(system, tbl, ptr);
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

  void OpenTable(const std::string tablename, inparams_t inparams={}) {
    std::string system = GetSystemFromTableName(tablename);
    NewTableInternal(system, tablename);
    auto tbl = tables_[tablename];
    if(!inparams.empty())
        tbl->inparams = inparams;
    LoadTableInternal(system, tables_[tablename]);
  }

  void RefreshTable(const std::string tablename) {
    std::string system = GetSystemFromTableName(tablename);
    if(tables_.find(tablename) == tables_.end())
        throw std::runtime_error("Table "+tablename+" has not been opened");
    AstsOpenedTable * tbl = tables_[tablename];
    if((tbl->thistable_->attr & mmfUpdateable) == 0) {
      // delete table data and reload it
      inparams_t temp_inparams = tbl->inparams;
      CloseTable(tablename);
      OpenTable(tablename, temp_inparams);
    }
    else {
      // send refresh request and update data
      if(tbl->Table < 0)
        throw std::runtime_error("Table handle is invalid");
      MTEMSG *TableData;
      int res = MTEAddTable(handles_[system], tbl->Table, tbl->ref);
      if(res != MTE_OK)
        throw std::runtime_error(std::string("MTEAddTable returned an error: ")+MTEErrorMsg(res));
      res = MTERefresh(handles_[system], &TableData);
      if(res != MTE_OK)
        throw std::runtime_error(std::string("MTERefresh returned an error: ")+MTEErrorMsg(res));
      int32_t * ptr=(int32_t *)(TableData->Data);
      ad::util::PointerHelper pointer(ptr);
      int tablecount = pointer.ReadInt();
      switch(tablecount) {
      case 0:
          break;
      case 1:
          LoadTableData(system, tbl, pointer._ptr);
          break;
      default:
          throw std::runtime_error("Too many tables in MTERefresh()!");
      }
      return;

    }
  }

  void CloseTable(const std::string tablename) {
    std::string system = GetSystemFromTableName(tablename);
    if (tables_.find(tablename) == tables_.end())
      throw std::runtime_error("Table "+tablename+" has not been opened");
    if(handles_[system] >= 0)
      MTECloseTable(handles_[system], tables_[tablename]->ref);
    engine_.CloseTable(tablename);
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
