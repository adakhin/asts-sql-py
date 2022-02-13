#ifndef ASTS_CONNECTION_H
#define ASTS_CONNECTION_H

#include <string>
#include <map>
#include <memory>

#include "mtesrl.h"
#include "mteerr.h"

#include "asts_interface.h"

namespace ad::asts {
typedef int MTEHandle;

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



template<typename storage_engine_t> class AstsConnection {
private:
  std::map<std::string, int> handles_ { {"TE", -1}, {"RE", -1}, {"RFS", -1}, {"ALGO", -1} };
  std::map<std::string, std::shared_ptr<AstsInterface> > interfaces_;
  std::map<std::string, AstsOpenedTable*> tables_;
  bool debug_ = true;
  storage_engine_t engine_;

  void NewTableInternal(const std::string& system, const std::string& tablename) {
    if(interfaces_[system]->tables.find(tablename) == interfaces_[system]->tables.end())
        throw std::runtime_error("Table "+tablename+" does not exist in interface "+interfaces_[system]->name_);
    // only one copy of each table may be opened
    if (tables_.find(tablename) != tables_.end())
        throw std::runtime_error("Table "+tablename+" has been already opened");
    engine_.OpenTable(interfaces_[system], tablename);
    tables_[tablename] = new AstsOpenedTable(interfaces_[system], tablename);
  }

  void LoadTableInternal(const std::string& system, AstsOpenedTable* tbl) {
    MTEMSG *TableData;
    //std::string params = tbl->ParamsToStr();
    std::string params = "";
    tbl->Table = MTEOpenTable(handles_[system], (char *)tbl->thistable_->name.c_str(), (char *)params.c_str(), 1, &TableData);
    if(tbl->Table >= 0)
    {
       // load actual data
       int32_t* ptr = (int32_t*)(TableData->Data);
    }
    else
        throw std::runtime_error("Unable to load table "+tbl->tablename_+": "+std::string(TableData->Data, TableData->DataLen));

  }
public:
  bool Connect(const std::string & system, const std::string & params, std::string & errmsg){
    if(handles_.find(system) == handles_.end())
      throw std::runtime_error("Invalid system "+system);
    if(handles_[system] >= 0)
      throw std::runtime_error("System "+system+" is already connected!");

    char ErrMsg[256];
    handles_[system] = MTEConnect((char*)params.c_str(), ErrMsg);

    if(handles_[system] < 0) {
      errmsg = "MTEConnect returned an error: "+std::to_string(handles_[system])+" "+std::string(ErrMsg);
      return false;
    }

    interfaces_[system] = std::make_shared<AstsInterface>();
    if(!interfaces_[system]->LoadInterface(handles_[system], errmsg, debug_)) {
  //    errmsg = "Unable to load interface!";
      return false;
    }
    engine_.AddInterface(interfaces_[system]);

    return true;

  }

  void Disconnect(const std::string & system){
    if(handles_.find(system) == handles_.end())
      return;
    if(handles_[system] >= 0) {
      MTEDisconnect(handles_[system]);
      handles_[system] = -1;
    }
  }

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

};

}
#endif // ASTS_CONNECTION_H
