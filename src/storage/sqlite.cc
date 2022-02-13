#include "sqlite.h"
#include "../util.h"

#include <mtesrl.h>

namespace ad::asts {

inline void SQLiteStorage::ExecOrThrow(std::string_view sql, std::string errormsg) {
  char *zErrMsg = nullptr;
  int error = sqlite3_exec(db_, sql.data(), NULL, 0, &zErrMsg);
  if(error)
    throw std::runtime_error(errormsg+": "+std::string(zErrMsg));
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
  std::string errmsg = std::string("SQLite error while create reflection for interface ")+iface->name_;
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
    for(auto & fld : rec.second->outfields) {
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

void SQLiteStorage::OpenTable(std::shared_ptr<AstsInterface> iface, const std::string& tablename) {
  // check if table exists
  std::string expr = "SELECT * FROM sqlite_master WHERE name ='"+tablename+"' and type='table' COLLATE NOCASE;";
  sqlite3_stmt *statement;
  int error = sqlite3_prepare_v2(db_, expr.c_str(), -1, &statement, 0);
  if(error)
      throw std::runtime_error("SQLite error occured while checking if table "+tablename+" already exists: "+std::string(sqlite3_errstr(error)));
  size_t rowcount = 0;
  while(true) {
      int res = sqlite3_step(statement);
      if(res == SQLITE_ROW)
          ++rowcount;
      if(res == SQLITE_DONE || res==SQLITE_ERROR)
          break;
  }
  sqlite3_finalize(statement);
  if(!rowcount) {
    // table doesn't exist. create it
    std::string create, pk, tmp;
    create = "create table "+tablename+" (";
    pk = ", PRIMARY KEY (";
    bool haskeyfields = false;
    std::vector<std::string> fields, pk_fields;
    fields.reserve(iface->tables[tablename]->outfields.size());
    for(auto & fld : iface->tables[tablename]->outfields) {
        tmp = "";
        switch(fld.type)
        {
        case AstsFieldType::kInteger:
          tmp = fld.name+" integer";
          break;
        case AstsFieldType::kFixed:
        case AstsFieldType::kFloatPoint:
          tmp = fld.name+" double";
          break;
        case AstsFieldType::kChar:
        case AstsFieldType::kDate:
        case AstsFieldType::kTime:
        case AstsFieldType::kFloat: // we will know how to format this only in runtime
        default:
          tmp = fld.name+" char("+std::to_string(fld.size)+")";
          break;
        }
        if( (fld.attr & mffKey) == mffKey) {
          haskeyfields = true;
          tmp.append(" NOT NULL");
          pk_fields.push_back(fld.name);
        }
        fields.push_back(tmp);
    }
    create.append(ad::util::join(fields,", "));
    if(haskeyfields)
      pk.append(ad::util::join(pk_fields, ", ").append(")"));
    else
      pk = "";
    create.append(pk.append(");"));
    ExecOrThrow(create, "SQLite error occured while creating table "+tablename);
  }
}

}
