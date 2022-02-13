#include "sqlite.h"

namespace ad::asts {

inline void SQLiteStorage::ExecOrThrow(std::string_view sql, std::string errormsg) {
  char *zErrMsg = nullptr;
  int error = sqlite3_exec(db_, sql.data(), NULL, 0, &zErrMsg);
  if(error)
    throw std::runtime_error(errormsg+std::string(zErrMsg));
}

SQLiteStorage::SQLiteStorage() {
  int error = sqlite3_open(":memory:", &db_);
  if(error) {
    sqlite3_close(db_);
    throw std::runtime_error("Unable to initialize SQLite storage");
  }
}

SQLiteStorage::~SQLiteStorage() {
  sqlite3_close(db_);
}

void SQLiteStorage::AddInterface(std::shared_ptr<AstsInterface> iface) {
  std::string sql = "create table if not exists MTE$STRUCTURE (system_type char(2), interface_name char(12), table_name char(12), orig_table_name char(12), field_name char(20), field_type integer, field_length integer, decimals integer);";
  std::string errmsg = std::string("SQLite error while create reflection for interface ")+iface->name_+": ";
  ExecOrThrow(sql, errmsg);
  auto add_fld = [&](std::string v) {
    sql.append("'"+v+"',");
  };
  auto add_int = [&](int v, bool last_val=false) {
    sql.append(std::to_string(v));
    if(!last_val)
      sql.append(",");
  };
  for(auto & rec : iface->tables)
    for(auto & fld : rec.second->outfields)
    {
      sql = "insert into MTE$STRUCTURE select ";
      add_fld(iface->GetSystemType());
      add_fld(iface->name_);
      add_fld(iface->prefix_+rec.second->name);
      add_fld(rec.second->name);
      add_fld(fld.name);
      add_int(fld.type);
      add_int(fld.size);
      add_int(fld.decimals, true);
      sql.append(";");
      ExecOrThrow(sql, errmsg);
    }
  sql.append("create index if not exists MTE$STRUCTURE_IDX on MTE$STRUCTURE (table_name, field_name);");
}

}
